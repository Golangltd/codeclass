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

package redis

import (
	"log"
	"time"
)

// ----------------------------------------------------------------------------
// synchronization utilities.
// ----------------------------------------------------------------------------

// result and result channel
//
// result defines a basic struct that either holds a generic (interface{}) reference
// or an Error reference. It is used to generically send and receive future results
// through channels.

type result struct {
	v interface{}
	e Error
}

// creates a result struct using the provided references
// and sends it on the specified channel.

func send(c chan result, v interface{}, e Error) {
	c <- result{v, e}
}

// blocks on the channel until a result is received.
// If the received result reference's e (error) field is not null,
// it will return it.

func receive(c chan result) (v interface{}, error Error) {
	fv := <-c
	if fv.e != nil {
		error = fv.e
	} else {
		v = fv.v
	}
	return
}

// using a timer blocks on the channel until a result is received.
// or timeout period expires.
// if timedout, returns timedout==true.
// otherwise,
// If the received result reference's e (error) field is not null,
// it will return it.
//
// For now presumably a nil result is OK and if error is nil, the return
// value v is the intended result, even if nil.

func tryReceive(c chan result, ns time.Duration) (v interface{}, error Error, timedout bool) {
	//	timer := NewTimer(ns)
	select {
	case fv := <-c:
		if fv.e != nil {
			error = fv.e
		} else {
			v = fv.v
		}
		//	case to := <-timer:
	case to := <-time.After(ns):
		timedout = true
		if debug() {
			log.Println("resultchan.TryGet() -- timedout waiting for futurevaluechan | timeout: ", to)
		}
	}
	return
}

// Future? interfaces very much in line with the Future<?> of Java.
// These variants all expose the same set of semantics in a type-safe manner.
//
// We're only exposing the getters on these future objects as references to
// these interfaces are returned to redis users.  Same considerations also
// inform the decision to limit the exposure of the newFuture? methods to the
// package.
//
// Also note that while the current implementation does not enforce this, the
// Future? references can only be used until a value, or an error is obtained.
// If a value is obtained from a Future? reference, any further calls to Get()
// will block indefinitely.  It is OK, of course, to use TryGet(..) repeatedly
// until it returns with a false 'timedout' return out param.

// FutureResult
//
// A generic future.  All type-safe Futures support this interface
//
type FutureResult interface {
	onError(Error)
}

// FutureBytes (for []byte)
//
type FutureBytes interface {
	//	onError (Error);
	set([]byte)
	Get() (vale []byte, error Error)
	TryGet(timeoutnano time.Duration) (value []byte, error Error, timedout bool)
}
type _byteslicefuture chan result

func newFutureBytes() FutureBytes            { return make(_byteslicefuture, 1) }
func (fvc _byteslicefuture) onError(e Error) { send(fvc, nil, e) }
func (fvc _byteslicefuture) set(v []byte)    { send(fvc, v, nil) }
func (fvc _byteslicefuture) Get() ([]byte, Error) {
	gv, err := receive(fvc)
	if err != nil {
		return nil, err
	}
	return gv.([]byte), err
}
func (fvc _byteslicefuture) TryGet(ns time.Duration) ([]byte, Error, bool) {
	gv, err, timedout := tryReceive(fvc, ns)
	if timedout || err != nil {
		return nil, err, timedout
	}
	return gv.([]byte), err, timedout
}

// FutureBytesArray (for [][]byte)
//
type FutureBytesArray interface {
	//	onError (Error);
	set([][]byte)
	Get() (vale [][]byte, error Error)
	TryGet(timeoutnano time.Duration) (value [][]byte, error Error, timedout bool)
}
type _bytearrayslicefuture chan result

func newFutureBytesArray() FutureBytesArray { return make(_bytearrayslicefuture, 1) }
func (fvc _bytearrayslicefuture) onError(e Error) {
	send(fvc, nil, e)
}
func (fvc _bytearrayslicefuture) set(v [][]byte) {
	send(fvc, v, nil)
}
func (fvc _bytearrayslicefuture) Get() (v [][]byte, error Error) {
	gv, err := receive(fvc)
	if err != nil {
		return nil, err
	}
	return gv.([][]byte), err
}
func (fvc _bytearrayslicefuture) TryGet(ns time.Duration) ([][]byte, Error, bool) {
	gv, err, timedout := tryReceive(fvc, ns)
	if timedout || err != nil {
		return nil, err, timedout
	}
	return gv.([][]byte), err, timedout
}

// FutureBool
//
type FutureBool interface {
	//	onError (Error);
	set(bool)
	Get() (val bool, error Error)
	TryGet(timeoutnano time.Duration) (value bool, error Error, timedout bool)
}
type _boolfuture chan result

func newFutureBool() FutureBool         { return make(_boolfuture, 1) }
func (fvc _boolfuture) onError(e Error) { send(fvc, nil, e) }
func (fvc _boolfuture) set(v bool)      { send(fvc, v, nil) }
func (fvc _boolfuture) Get() (v bool, error Error) {
	gv, err := receive(fvc)
	if err != nil {
		return false, err
	}
	return gv.(bool), err
}
func (fvc _boolfuture) TryGet(ns time.Duration) (bool, Error, bool) {
	gv, err, timedout := tryReceive(fvc, ns)
	if timedout || err != nil {
		return false, err, timedout
	}
	return gv.(bool), err, timedout
}

// FutureString
//
type FutureString interface {
	//	onError (execErr Error);
	set(v string)
	Get() (string, Error)
	TryGet(timeoutnano time.Duration) (value string, error Error, timedout bool)
}
type _futurestring chan result

func newFutureString() FutureString       { return make(_futurestring, 1) }
func (fvc _futurestring) onError(e Error) { send(fvc, nil, e) }
func (fvc _futurestring) set(v string)    { send(fvc, v, nil) }
func (fvc _futurestring) Get() (v string, error Error) {
	gv, err := receive(fvc)
	if err != nil {
		return "", err
	}
	return gv.(string), err
}
func (fvc _futurestring) TryGet(ns time.Duration) (string, Error, bool) {
	gv, err, timedout := tryReceive(fvc, ns)
	if timedout || err != nil {
		return "", err, timedout
	}
	return gv.(string), err, timedout
}

// FutureInt64
//
type FutureInt64 interface {
	//	onError (execErr Error);
	set(v int64)
	Get() (int64, Error)
	TryGet(timeoutnano time.Duration) (value int64, error Error, timedout bool)
}
type _futureint64 chan result

func newFutureInt64() FutureInt64        { return make(_futureint64, 1) }
func (fvc _futureint64) onError(e Error) { send(fvc, nil, e) }
func (fvc _futureint64) set(v int64)     { send(fvc, v, nil) }
func (fvc _futureint64) Get() (v int64, error Error) {
	gv, err := receive(fvc)
	if err != nil {
		return -1, err
	}
	return gv.(int64), err
}
func (fvc _futureint64) TryGet(ns time.Duration) (int64, Error, bool) {
	gv, err, timedout := tryReceive(fvc, ns)
	if timedout || err != nil {
		return 0, err, timedout
	}
	return gv.(int64), err, timedout
}

// FutureFloat64
//
type FutureFloat64 interface {
	Get() (float64, Error)
	TryGet(timeoutnano time.Duration) (v float64, error Error, timedout bool)
}
type _futurefloat64 struct {
	future FutureBytes
}

func newFutureFloat64(future FutureBytes) FutureFloat64 {
	return _futurefloat64{future}
}
func (fvc _futurefloat64) Get() (v float64, error Error) {
	gv, err := fvc.future.Get()
	if err != nil {
		return 0, err
	}
	v, err = Btof64(gv)
	return v, nil
}
func (fvc _futurefloat64) TryGet(ns time.Duration) (float64, Error, bool) {
	gv, err, timedout := fvc.future.TryGet(ns)
	if timedout || err != nil {
		return float64(0), err, timedout
	}
	v, err := Btof64(gv)
	return v, nil, timedout
}
