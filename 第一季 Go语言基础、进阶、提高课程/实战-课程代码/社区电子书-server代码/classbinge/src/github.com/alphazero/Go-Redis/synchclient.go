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
	"bytes"
	"fmt"
	"log"
	"strconv"
	"strings"
)

// -----------------------------------------------------------------------------
// syncClient
// -----------------------------------------------------------------------------

type syncClient struct {
	conn SyncConnection
}

// Create a new syncClient and connects to the Redis server using the
// default ConnectionSpec.
//
func NewSynchClient() (c Client, err Error) {
	spec := DefaultSpec()
	c, err = NewSynchClientWithSpec(spec)
	if err != nil {
		log.Println("NewSynchClientWithSpec raised error: ", err)
	}
	if c == nil {
		log.Println("NewSynchClientWithSpec returned nil Client.")
		err = newSystemError("NewSynchClientWithSpec returned nil Client")
	}
	return
}

// Create a new syncClient and connects to the Redis server using the
// specified ConnectionSpec.
//
func NewSynchClientWithSpec(spec *ConnectionSpec) (c Client, err Error) {
	_c := new(syncClient)
	_c.conn, err = NewSyncConnection(spec)
	if err != nil {
		return nil, withError(err)
	}
	//	_c.conn = conn
	return _c, nil
}

// -----------------------------------------------------------------------------
// interface redis.Client support
// -----------------------------------------------------------------------------

// Redis QUIT command.
func (c *syncClient) Quit() (err Error) {
	_, err = c.conn.ServiceRequest(&QUIT, [][]byte{})
	return
}

// Redis GET command.
func (c *syncClient) Get(arg0 string) (result []byte, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&GET, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis TYPE command.
func (c *syncClient) Type(arg0 string) (result KeyType, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&TYPE, [][]byte{arg0bytes})
	if err == nil {
		result = GetKeyType(resp.GetStringValue())
	}
	return result, err
}

// Redis SET command.
func (c *syncClient) Set(arg0 string, arg1 []byte) (err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	_, err = c.conn.ServiceRequest(&SET, [][]byte{arg0bytes, arg1bytes})
	return
}

// Redis SAVE command.
func (c *syncClient) Save() (err Error) {
	_, err = c.conn.ServiceRequest(&SAVE, [][]byte{})
	return

}

// Redis KEYS command.
func (c *syncClient) AllKeys() (result []string, err Error) {
	return c.Keys("*")
}

// Redis KEYS command.
func (c *syncClient) Keys(arg0 string) (result []string, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&KEYS, [][]byte{arg0bytes})
	if err == nil {
		//		result = strings.Split(bytes.NewBuffer(resp.GetBulkData()).String(), " ", 0);
		res := resp.GetMultiBulkData()
		result = make([]string, len(res))
		for i, key_bytes := range res {
			result[i] = string(key_bytes)
		}
	}
	return result, err

}

func convAndSplit(buff []byte) []string {
	return strings.SplitN(bytes.NewBuffer(buff).String(), " ", 0)
}

/***
// Redis SORT command.
func (c *syncClient) Sort (arg0 string) (result redis.Sort, err Error){
	arg0bytes := []byte (arg0);

	var resp Response;
	resp, err = c.conn.ServiceRequest(&SORT, [][]byte{arg0bytes});
	if err == nil {result = resp.GetMultiBulkData();}
	return result, err;

}
***/
// Redis EXISTS command.
func (c *syncClient) Exists(arg0 string) (result bool, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&EXISTS, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis RENAME command.
func (c *syncClient) Rename(arg0 string, arg1 string) (err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	_, err = c.conn.ServiceRequest(&RENAME, [][]byte{arg0bytes, arg1bytes})
	return
}

// Redis INFO command.
func (c *syncClient) Info() (result map[string]string, err Error) {
	var resp Response
	resp, err = c.conn.ServiceRequest(&INFO, [][]byte{})
	if err == nil {
		result = parseInfo(resp.GetBulkData())
		//		infoStr := bytes.NewBuffer(resp.GetBulkData()).String();
		//		infoItems := strings.Split(infoStr, "\r\n", 0);
		//		result = make(map[string] string);
		//		for _, entry := range infoItems  {
		//			etuple := strings.Split(entry, ":", 2);
		//			result[etuple[0]] = etuple[1];
		//		}
	}
	return result, err
}

func parseInfo(buff []byte) map[string]string {
	infoStr := bytes.NewBuffer(buff).String()
	infoItems := strings.SplitN(infoStr, "\r\n", 0)
	result := make(map[string]string)
	for _, entry := range infoItems {
		etuple := strings.SplitN(entry, ":", 2)
		result[etuple[0]] = etuple[1]
	}
	return result
}

// Redis PING command.
func (c *syncClient) Ping() (err Error) {
	if c == nil {
		log.Println("FAULT in synchclient.Ping(): why is c nil?")
		return newSystemError("c *syncClient is NIL!")
	} else if c.conn == nil {
		log.Println("FAULT in synchclient.Ping(): why is c.conn nil?")
		return newSystemError("c.conn *SynchConnection is NIL!")
	}
	_, err = c.conn.ServiceRequest(&PING, [][]byte{})
	return
}

// Redis SETNX command.
func (c *syncClient) Setnx(arg0 string, arg1 []byte) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&SETNX, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis GETSET command.
func (c *syncClient) Getset(arg0 string, arg1 []byte) (result []byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&GETSET, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis MGET command.
func (c *syncClient) Mget(arg0 string, arg1 []string) (result [][]byte, err Error) {
	args := appendAndConvert(arg0, arg1...)
	var resp Response
	resp, err = c.conn.ServiceRequest(&MGET, args)
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err
}

// Redis INCR command.
func (c *syncClient) Incr(arg0 string) (result int64, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&INCR, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis INCRBY command.
func (c *syncClient) Incrby(arg0 string, arg1 int64) (result int64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp Response
	resp, err = c.conn.ServiceRequest(&INCRBY, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis DECR command.
func (c *syncClient) Decr(arg0 string) (result int64, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&DECR, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis DECRBY command.
func (c *syncClient) Decrby(arg0 string, arg1 int64) (result int64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp Response
	resp, err = c.conn.ServiceRequest(&DECRBY, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis DEL command.
func (c *syncClient) Del(arg0 string) (result bool, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&DEL, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis RANDOMKEY command.
func (c *syncClient) Randomkey() (result string, err Error) {
	var resp Response
	resp, err = c.conn.ServiceRequest(&RANDOMKEY, [][]byte{})
	if err == nil {
		result = string(resp.GetBulkData())
	}
	return result, err

}

// Redis RENAMENX command.
func (c *syncClient) Renamenx(arg0 string, arg1 string) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	var resp Response
	resp, err = c.conn.ServiceRequest(&RENAMENX, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis DBSIZE command.
func (c *syncClient) Dbsize() (result int64, err Error) {
	var resp Response
	resp, err = c.conn.ServiceRequest(&DBSIZE, [][]byte{})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis EXPIRE command.
func (c *syncClient) Expire(arg0 string, arg1 int64) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp Response
	resp, err = c.conn.ServiceRequest(&EXPIRE, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis TTL command.
func (c *syncClient) Ttl(arg0 string) (result int64, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&TTL, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis RPUSH command.
func (c *syncClient) Rpush(arg0 string, arg1 []byte) (err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	_, err = c.conn.ServiceRequest(&RPUSH, [][]byte{arg0bytes, arg1bytes})
	return
}

// Redis LPUSH command.
func (c *syncClient) Lpush(arg0 string, arg1 []byte) (err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	_, err = c.conn.ServiceRequest(&LPUSH, [][]byte{arg0bytes, arg1bytes})
	return
}

// Redis LSET command.
func (c *syncClient) Lset(arg0 string, arg1 int64, arg2 []byte) (err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))
	arg2bytes := arg2

	_, err = c.conn.ServiceRequest(&LSET, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	return
}

// Redis LREM command.
func (c *syncClient) Lrem(key string, value []byte, count int64) (result int64, err Error) {
	arg0bytes := []byte(key)
	arg1bytes := value
	arg2bytes := []byte(strconv.FormatInt(count, 10))

	var resp Response
	resp, err = c.conn.ServiceRequest(&LREM, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis LLEN command.
func (c *syncClient) Llen(arg0 string) (result int64, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&LLEN, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis LRANGE command.
func (c *syncClient) Lrange(arg0 string, arg1 int64, arg2 int64) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(strconv.FormatInt(arg1, 10))
	arg2bytes := []byte(strconv.FormatInt(arg2, 10))

	var resp Response
	resp, err = c.conn.ServiceRequest(&LRANGE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis LTRIM command.
func (c *syncClient) Ltrim(arg0 string, arg1 int64, arg2 int64) (err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))
	arg2bytes := []byte(fmt.Sprintf("%d", arg2))

	_, err = c.conn.ServiceRequest(&LTRIM, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	return
}

// Redis LINDEX command.
func (c *syncClient) Lindex(arg0 string, arg1 int64) (result []byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp Response
	resp, err = c.conn.ServiceRequest(&LINDEX, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis LPOP command.
func (c *syncClient) Lpop(arg0 string) (result []byte, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&LPOP, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis BLPOP command.
func (c *syncClient) Blpop(arg0 string, timeout int) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprint(timeout))

	var resp Response
	resp, err = c.conn.ServiceRequest(&BLPOP, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis RPOP command.
func (c *syncClient) Rpop(arg0 string) (result []byte, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&RPOP, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis BRPOP command.
func (c *syncClient) Brpop(arg0 string, timeout int) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprint(timeout))

	var resp Response
	resp, err = c.conn.ServiceRequest(&BRPOP, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis RPOPLPUSH command.
func (c *syncClient) Rpoplpush(arg0 string, arg1 string) (result []byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	var resp Response
	resp, err = c.conn.ServiceRequest(&RPOPLPUSH, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis BRPOPLPUSH command.
func (c *syncClient) Brpoplpush(arg0 string, arg1 string, timeout int) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)
	arg2bytes := []byte(fmt.Sprint(timeout))

	var resp Response
	resp, err = c.conn.ServiceRequest(&BRPOPLPUSH, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis SADD command.
func (c *syncClient) Sadd(arg0 string, arg1 []byte) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&SADD, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis SREM command.
func (c *syncClient) Srem(arg0 string, arg1 []byte) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&SREM, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis SISMEMBER command.
func (c *syncClient) Sismember(arg0 string, arg1 []byte) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&SISMEMBER, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis SMOVE command.
func (c *syncClient) Smove(arg0 string, arg1 string, arg2 []byte) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)
	arg2bytes := arg2

	var resp Response
	resp, err = c.conn.ServiceRequest(&SMOVE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis SCARD command.
func (c *syncClient) Scard(arg0 string) (result int64, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&SCARD, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// REVU - this is buggy in conjunction with callsite usage (when arr is nil)
// deprecated - TODO convert asynchclient.go to use appendAndConvert
func concatAndGetBytes(arr []string, delim string) []byte {
	cstr := ""
	for _, s := range arr {
		cstr += s
		cstr += delim
	}
	return []byte(cstr)
}

// REVU - use this instead of concatAndGetBytes TODO - for asynch
func appendAndConvert(a0 string, arr ...string) [][]byte {
	sarr := make([][]byte, 1+len(arr))
	sarr[0] = []byte(a0)
	for i, v := range arr {
		sarr[i+1] = []byte(v)
	}
	return sarr
}
func packArrays(a0 []byte, arr ...[]byte) [][]byte {
	sarr := make([][]byte, 1+len(arr))
	sarr[0] = a0
	for i, v := range arr {
		sarr[i+1] = v
	}
	return sarr
}

// Redis SINTER command.
func (c *syncClient) Sinter(arg0 string, arg1 []string) (result [][]byte, err Error) {
	var resp Response
	resp, err = c.conn.ServiceRequest(&SINTER, appendAndConvert(arg0, arg1...))
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis SINTERSTORE command.
func (c *syncClient) Sinterstore(arg0 string, arg1 []string) (err Error) {
	_, err = c.conn.ServiceRequest(&SINTERSTORE, appendAndConvert(arg0, arg1...))
	return
}

// Redis SUNION command.
func (c *syncClient) Sunion(arg0 string, arg1 []string) (result [][]byte, err Error) {
	var resp Response
	//		resp, err = c.conn.ServiceRequest(&SUNION, [][]byte{arg0bytes, arg1bytes})
	resp, err = c.conn.ServiceRequest(&SUNION, appendAndConvert(arg0, arg1...))
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis SUNIONSTORE command.
func (c *syncClient) Sunionstore(arg0 string, arg1 []string) (err Error) {
	_, err = c.conn.ServiceRequest(&SUNIONSTORE, appendAndConvert(arg0, arg1...))
	return
}

// Redis SDIFF command.
func (c *syncClient) Sdiff(arg0 string, arg1 []string) (result [][]byte, err Error) {
	var resp Response
	resp, err = c.conn.ServiceRequest(&SDIFF, appendAndConvert(arg0, arg1...))
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis SDIFFSTORE command.
func (c *syncClient) Sdiffstore(arg0 string, arg1 []string) (err Error) {
	_, err = c.conn.ServiceRequest(&SDIFFSTORE, appendAndConvert(arg0, arg1...))
	return
}

// Redis SMEMBERS command.
func (c *syncClient) Smembers(arg0 string) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&SMEMBERS, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis SRANDMEMBER command.
func (c *syncClient) Srandmember(arg0 string) (result []byte, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&SRANDMEMBER, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis ZADD command.
func (c *syncClient) Zadd(arg0 string, arg1 float64, arg2 []byte) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%e", arg1))
	arg2bytes := arg2

	var resp Response
	resp, err = c.conn.ServiceRequest(&ZADD, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis ZREM command.
func (c *syncClient) Zrem(arg0 string, arg1 []byte) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&ZREM, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis ZCARD command.
func (c *syncClient) Zcard(arg0 string) (result int64, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&ZCARD, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis ZSCORE command.
func (c *syncClient) Zscore(arg0 string, arg1 []byte) (result float64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&ZSCORE, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		buff := resp.GetBulkData()
		//		fnum, oserr := strconv.Atof64(bytes.NewBuffer(buff).String());
		//		if oserr != nil {
		//			err = newSystemErrorWithCause("Expected a parsable byte representation of a float64 in Zscore!", oserr);
		//		}
		//		result = fnum;
		result, err = Btof64(buff)
	}
	return result, err

}

func Btof64(buff []byte) (num float64, e Error) {
	num, ce := strconv.ParseFloat(bytes.NewBuffer(buff).String(), 64)
	if ce != nil {
		e = newSystemErrorWithCause("Expected a parsable byte representation of a float64", ce)
	}
	return
}

// Redis ZRANGE command.
func (c *syncClient) Zrange(arg0 string, arg1 int64, arg2 int64) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))
	arg2bytes := []byte(fmt.Sprintf("%d", arg2))

	var resp Response
	resp, err = c.conn.ServiceRequest(&ZRANGE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis ZREVRANGE command.
func (c *syncClient) Zrevrange(arg0 string, arg1 int64, arg2 int64) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))
	arg2bytes := []byte(fmt.Sprintf("%d", arg2))

	var resp Response
	resp, err = c.conn.ServiceRequest(&ZREVRANGE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis ZRANGEBYSCORE command.
func (c *syncClient) Zrangebyscore(arg0 string, arg1 float64, arg2 float64) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%e", arg1))
	arg2bytes := []byte(fmt.Sprintf("%e", arg2))

	var resp Response
	resp, err = c.conn.ServiceRequest(&ZRANGEBYSCORE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis HGET command.
func (c *syncClient) Hget(arg0 string, arg1 string) (result []byte, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	var resp Response
	resp, err = c.conn.ServiceRequest(&HGET, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBulkData()
	}
	return result, err

}

// Redis HSET command.
func (c *syncClient) Hset(arg0 string, arg1 string, arg2 []byte) (err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)
	arg2bytes := arg2

	_, err = c.conn.ServiceRequest(&HSET, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	return
}

// Redis HGETALL command.
func (c *syncClient) Hgetall(arg0 string) (result [][]byte, err Error) {
	arg0bytes := []byte(arg0)

	var resp Response
	resp, err = c.conn.ServiceRequest(&HGETALL, [][]byte{arg0bytes})
	if err == nil {
		result = resp.GetMultiBulkData()
	}
	return result, err

}

// Redis FLUSHDB command.
func (c *syncClient) Flushdb() (err Error) {
	_, err = c.conn.ServiceRequest(&FLUSHDB, [][]byte{})
	return
}

// Redis FLUSHALL command.
func (c *syncClient) Flushall() (err Error) {
	_, err = c.conn.ServiceRequest(&FLUSHALL, [][]byte{})
	return
}

// Redis MOVE command.
func (c *syncClient) Move(arg0 string, arg1 int64) (result bool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp Response
	resp, err = c.conn.ServiceRequest(&MOVE, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.GetBooleanValue()
	}
	return result, err

}

// Redis BGSAVE command.
func (c *syncClient) Bgsave() (err Error) {
	_, err = c.conn.ServiceRequest(&BGSAVE, [][]byte{})
	return
}

// Redis LASTSAVE command.
func (c *syncClient) Lastsave() (result int64, err Error) {
	var resp Response
	resp, err = c.conn.ServiceRequest(&LASTSAVE, [][]byte{})
	if err == nil {
		result = resp.GetNumberValue()
	}
	return result, err

}

// Redis PUBLISH command.
func (c *syncClient) Publish(arg0 string, arg1 []byte) (rcvCnt int64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp Response
	resp, err = c.conn.ServiceRequest(&PUBLISH, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		rcvCnt = resp.GetNumberValue()
	}
	return rcvCnt, err
}
