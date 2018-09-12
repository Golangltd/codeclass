//   Copyright 2009-2012 Joubin Houshyar
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

// REVU notes for protocol.go
// - all exported funcs that can raise error must return Error.
// - If error is via panic, then it must be a SystemError
// - If SystemError and with cause, the cause must be std.lib or 3rd party

package redis

import (
	"bufio"
	"bytes"
	"errors"
	"fmt"
	"io"
	"strconv"
)

// ----------------------------------------------------------------------------
// Wire
// ----------------------------------------------------------------------------

// protocol's special bytes
const (
	cr_byte    byte = byte('\r')
	lf_byte         = byte('\n')
	space_byte      = byte(' ')
	err_byte        = byte('-')
	ok_byte         = byte('+')
	count_byte      = byte('*')
	size_byte       = byte('$')
	num_byte        = byte(':')
	true_byte       = byte('1')
)

type ctlbytes []byte

var crlf_bytes ctlbytes = ctlbytes{cr_byte, lf_byte}

// ----------------------------------------------------------------------------
// Services
// ----------------------------------------------------------------------------

// Creates the byte buffer that corresponds to the specified Command and
// provided command arguments.
//
// panics on error (with redis.Error)
func CreateRequestBytes(cmd *Command, args [][]byte) []byte {

	defer func() {
		if e := recover(); e != nil {
			panic(newSystemErrorf("CreateRequestBytes(%s) - failed to create request buffer", cmd.Code))
		}
	}()
	cmd_bytes := []byte(cmd.Code)

	buffer := bytes.NewBufferString("")
	buffer.WriteByte(count_byte)
	buffer.Write([]byte(strconv.Itoa(len(args) + 1)))
	buffer.Write(crlf_bytes)
	buffer.WriteByte(size_byte)
	buffer.Write([]byte(strconv.Itoa(len(cmd_bytes))))
	buffer.Write(crlf_bytes)
	buffer.Write(cmd_bytes)
	buffer.Write(crlf_bytes)

	for _, s := range args {
		buffer.WriteByte(size_byte)
		buffer.Write([]byte(strconv.Itoa(len(s))))
		buffer.Write(crlf_bytes)
		buffer.Write(s)
		buffer.Write(crlf_bytes)
	}

	return buffer.Bytes()
}

// Creates a specific Future type for the given Redis command
// and returns it as a generic reference.
func CreateFuture(cmd *Command) (future interface{}) {
	switch cmd.RespType {
	case BOOLEAN:
		future = newFutureBool()
	case BULK:
		future = newFutureBytes()
	case MULTI_BULK:
		future = newFutureBytesArray()
	case NUMBER:
		future = newFutureInt64()
	case STATUS:
		future = newFutureBool()
	case STRING:
		future = newFutureString()
	case VIRTUAL:
		// REVU - treating virtual futures as FutureBools (always true)
		future = newFutureBool()
	}
	return
}

// Sets the type specific result value from the response for the future reference
// based on the command type.
func SetFutureResult(future interface{}, cmd *Command, r Response) {
	if r.IsError() {
		future.(FutureResult).onError(newRedisError(r.GetMessage()))
	} else {
		switch cmd.RespType {
		case BOOLEAN:
			future.(FutureBool).set(r.GetBooleanValue())
		case BULK:
			future.(FutureBytes).set(r.GetBulkData())
		case MULTI_BULK:
			future.(FutureBytesArray).set(r.GetMultiBulkData())
		case NUMBER:
			future.(FutureInt64).set(r.GetNumberValue())
		case STATUS:
			future.(FutureBool).set(true)
		case STRING:
			future.(FutureString).set(r.GetStringValue())
		case VIRTUAL:
			// REVU - OK to treat virtual commands as FutureBool
			future.(FutureBool).set(true)
		}
	}
}

// ----------------------------------------------------------------------------
// request processing
// ----------------------------------------------------------------------------

// Either writes all the bytes or it fails and returns an error
//
// panics on error (with redis.Error)
func sendRequest(w io.Writer, data []byte) {
	loginfo := "sendRequest"
	if w == nil {
		panic(newSystemErrorf("<BUG> %s() - nil Writer", loginfo))
	}

	n, e := w.Write(data)
	if e != nil {
		panic(newSystemErrorf("%s() - connection Write wrote %d bytes only.", loginfo, n))
	}

	// doc isn't too clear but the underlying netFD may return n<len(data) AND
	// e == nil, but that's precisely what we're checking.
	// presumably we can try sending the remaining bytes but that is precisely
	// what netFD.Write is doing (and it couldn't) so ...
	if n < len(data) {
		panic(newSystemErrorf("%s() - connection Write wrote %d bytes only.", loginfo, n))
	}
}

// ----------------------------------------------------------------------------
// Response
// ----------------------------------------------------------------------------

type Response interface {
	IsError() bool
	GetMessage() string
	GetBooleanValue() bool
	GetNumberValue() int64
	GetStringValue() string
	GetBulkData() []byte
	GetMultiBulkData() [][]byte
}
type _response struct {
	isError       bool
	msg           string
	boolval       bool
	numval        int64
	stringval     string
	bulkdata      []byte
	multibulkdata [][]byte
}

func (r *_response) IsError() bool          { return r.isError }
func (r *_response) GetMessage() string     { return r.msg }
func (r *_response) GetBooleanValue() bool  { return r.boolval }
func (r *_response) GetNumberValue() int64  { return r.numval }
func (r *_response) GetStringValue() string { return r.stringval }
func (r *_response) GetBulkData() []byte    { return r.bulkdata }
func (r *_response) GetMultiBulkData() [][]byte {
	return r.multibulkdata
}

// ----------------------------------------------------------------------------
// response processing
// ----------------------------------------------------------------------------

// Gets the response to the command.
//
// The returned response (regardless of flavor) may have (application level)
// errors as sent from Redis server.  (Note err will be nil in that case)
//
// Any errors (whether runtime or bugs) are returned as redis.Error.
func GetResponse(reader *bufio.Reader, cmd *Command) (resp Response, err Error) {

	defer func() {
		err = onRecover(recover(), "GetResponse")
	}()

	buf := readToCRLF(reader)

	// Redis error
	if buf[0] == err_byte {
		resp = &_response{msg: string(buf[1:]), isError: true}
		return
	}

	switch cmd.RespType {
	case STATUS:
		resp = &_response{msg: string(buf[1:])}
		return
	case STRING:
		assertCtlByte(buf, ok_byte, "STRING")
		resp = &_response{stringval: string(buf[1:])}
		return
	case BOOLEAN:
		assertCtlByte(buf, num_byte, "BOOLEAN")
		resp = &_response{boolval: buf[1] == true_byte}
		return
	case NUMBER:
		assertCtlByte(buf, num_byte, "NUMBER")
		n, e := strconv.ParseInt(string(buf[1:]), 10, 64)
		assertNotError(e, "in GetResponse - parse error in NUMBER response")
		resp = &_response{numval: n}
		return
	case VIRTUAL:
		resp = &_response{boolval: true}
		return
	case BULK:
		assertCtlByte(buf, size_byte, "BULK")
		size, e := strconv.Atoi(string(buf[1:]))
		assertNotError(e, "in GetResponse - parse error in BULK size")
		resp = &_response{bulkdata: readBulkData(reader, size)}
		return
	case MULTI_BULK:
		assertCtlByte(buf, count_byte, "MULTI_BULK")
		cnt, e := strconv.Atoi(string(buf[1:]))
		assertNotError(e, "in GetResponse - parse error in MULTIBULK cnt")
		resp = &_response{multibulkdata: readMultiBulkData(reader, cnt)}
		return
	}

	panic(fmt.Errorf("BUG - GetResponse - this should not have been reached"))
}

// panics on error (with redis.Error)
func assertCtlByte(buf []byte, b byte, info string) {
	if buf[0] != b {
		panic(newSystemErrorf("control byte for %s is not '%s' as expected - got '%s'", info, string(b), string(buf[0])))
	}
}

// panics on error (with redis.Error)
func assertNotError(e error, info string) {
	if e != nil {
		panic(newSystemErrorWithCause(info, e))
	}
}

// ----------------------------------------------------------------------------
// PubSub message
// ----------------------------------------------------------------------------

type PubSubMType int

const (
	SUBSCRIBE_ACK PubSubMType = iota
	UNSUBSCRIBE_ACK
	MESSAGE
)

func (t PubSubMType) String() string {
	switch t {
	case SUBSCRIBE_ACK:
		return "SUBSCRIBE_ACK"
	case UNSUBSCRIBE_ACK:
		return "UNSUBSCRIBE_ACK"
	case MESSAGE:
		return "MESSAGE"
	}
	panic(newSystemErrorf("BUG - unknown PubSubMType %d", t))
}

// Conforms to the payload as received from wire.
// If Type is MESSAGE, then Body will contain a message, and
// SubscriptionCnt will be -1.
// otherwise, it is expected that SubscriptionCnt will contain subscription-info,
// e.g. number of subscribed channels, and data will be nil.
type Message struct {
	Type            PubSubMType
	Topic           string
	Body            []byte
	SubscriptionCnt int
}

func (m Message) String() string {
	return fmt.Sprintf("Message [type:%s topic:%s body:<%s> subcnt:%d]",
		m.Type,
		m.Topic,
		m.Body,
		m.SubscriptionCnt,
	)
}

func newMessage(topic string, Body []byte) *Message {
	m := Message{}
	m.Type = MESSAGE
	m.Topic = topic
	m.Body = Body
	return &m
}

func newPubSubAck(Type PubSubMType, topic string, scnt int) *Message {
	m := Message{}
	m.Type = Type
	m.Topic = topic
	m.SubscriptionCnt = scnt
	return &m
}

func newSubcribeAck(topic string, scnt int) *Message {
	return newPubSubAck(SUBSCRIBE_ACK, topic, scnt)
}

func newUnsubcribeAck(topic string, scnt int) *Message {
	return newPubSubAck(UNSUBSCRIBE_ACK, topic, scnt)
}

// ----------------------------------------------------------------------------
// PubSub message processing
// ----------------------------------------------------------------------------

// Fully reads and processes an expected Redis pubsub message byte sequence.
func GetPubSubResponse(r *bufio.Reader) (msg *Message, err Error) {
	defer func() {
		err = onRecover(recover(), "GetPubSubResponse")
	}()

	buf := readToCRLF(r)
	assertCtlByte(buf, count_byte, "PubSub Sequence")

	num, e := strconv.ParseInt(string(buf[1:len(buf)]), 10, 64)
	assertNotError(e, "in getPubSubResponse - ParseInt")
	if num != 3 {
		panic(fmt.Errorf("<BUG> Expecting *3 for len in response - got %d - buf: %s", num, buf))
	}

	header := readMultiBulkData(r, 2)

	msgtype := string(header[0])
	subid := string(header[1])

	buf = readToCRLF(r)

	n, e := strconv.Atoi(string(buf[1:]))
	assertNotError(e, "in getPubSubResponse - pubsub msg seq 3 line - number parse error")

	// TODO - REVU decisiont to conflate P/SUB and P/UNSUB
	switch msgtype {
	case "subscribe":
		assertCtlByte(buf, num_byte, "subscribe")
		msg = newSubcribeAck(subid, n)
	case "unsubscribe":
		assertCtlByte(buf, num_byte, "unsubscribe")
		msg = newUnsubcribeAck(subid, n)
	case "message":
		assertCtlByte(buf, size_byte, "MESSAGE")
		msg = newMessage(subid, readBulkData(r, int(n)))
	// TODO
	case "psubscribe", "punsubscribe", "pmessage":
		panic(fmt.Errorf("<BUG> - pattern-based message type %s not implemented", msgtype))
	}

	return
}

// ----------------------------------------------------------------------------
// protocol i/o
// ----------------------------------------------------------------------------

// reads all bytes upto CR-LF.  (Will eat those last two bytes)
// return the line []byte up to CR-LF
// error returned is NOT ("-ERR ...").  If there is a Redis error
// that is in the line buffer returned
//
// panics on errors (with redis.Error)
func readToCRLF(r *bufio.Reader) []byte {
	//	var buf []byte
	buf, e := r.ReadBytes(cr_byte)
	if e != nil {
		panic(newSystemErrorWithCause("readToCRLF - ReadBytes", e))
	}

	var b byte
	b, e = r.ReadByte()
	if e != nil {
		panic(newSystemErrorWithCause("readToCRLF - ReadByte", e))
	}
	if b != lf_byte {
		e = errors.New("<BUG> Expecting a Linefeed byte here!")
	}
	return buf[0 : len(buf)-1]
}

// Reads a multibulk response of given expected elements.
//
// panics on errors (with redis.Error)
func readBulkData(r *bufio.Reader, n int) (data []byte) {
	if n >= 0 {
		buffsize := n + 2
		data = make([]byte, buffsize)
		if _, e := io.ReadFull(r, data); e != nil {
			panic(newSystemErrorWithCause("readBulkData - ReadFull", e))
		} else {
			if data[n] != cr_byte || data[n+1] != lf_byte {
				panic(newSystemErrorf("terminal was not crlf_bytes as expected - data[n:n+1]:%s", data[n:n+1]))
			}
			data = data[:n]
		}
	}
	return
}

// Reads a multibulk response of given expected elements.
// The initial *num\r\n is assumed to have been consumed.
//
// panics on errors (with redis.Error)
func readMultiBulkData(conn *bufio.Reader, num int) [][]byte {
	data := make([][]byte, num)
	for i := 0; i < num; i++ {
		buf := readToCRLF(conn)
		if buf[0] != size_byte {
			panic(newSystemErrorf("readMultiBulkData - expected: size_byte got: %d", buf[0]))
		}

		size, e := strconv.Atoi(string(buf[1:]))
		if e != nil {
			panic(newSystemErrorWithCause("readMultiBulkData - Atoi parse error", e))
		}
		data[i] = readBulkData(conn, size)
	}
	return data
}
