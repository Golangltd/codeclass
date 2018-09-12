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
	"fmt"
	"log"
	"strconv"
)

// -----------------------------------------------------------------------------
// asyncClient
// -----------------------------------------------------------------------------

type asyncClient struct {
	conn AsyncConnection
}

// Create a new Client and connects to the Redis server using the
// default ConnectionSpec.
//
func NewAsynchClient() (c AsyncClient, err Error) {
	spec := DefaultSpec()
	return NewAsynchClientWithSpec(spec)
}

// Create a new asynClient and connects to the Redis server using the
// specified ConnectionSpec.
//
func NewAsynchClientWithSpec(spec *ConnectionSpec) (client AsyncClient, err Error) {
	c := new(asyncClient)
	c.conn, err = NewAsynchConnection(spec)
	if err != nil {
		if debug() {
			log.Println("NewAsyncConnection() raised error: ", err)
		}
		return nil, err
	}
	return c, nil
}

// -----------------------------------------------------------------------------
// interface redis.AsyncClient support
// -----------------------------------------------------------------------------

// Redis QUIT command.
func (c *asyncClient) Quit() (stat FutureBool, err Error) {
	//	log.Println("<BUG> Lazy programmer hasn't implemented Quit!")
	//	return nil, NewError(SYSTEM_ERR, "<BUG> Lazy programmer hasn't implemented Quit!")
	resp, err := c.conn.QueueRequest(&QUIT, [][]byte{})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis GET command.
func (c *asyncClient) Get(arg0 string) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&GET, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis TYPE command.
func (c *asyncClient) Type(arg0 string) (result FutureKeyType, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&TYPE, [][]byte{arg0bytes})
	if err == nil {
		result = newFutureKeyType(resp.future.(FutureString))
	}
	return result, err
}

// Redis SET command.
func (c *asyncClient) Set(arg0 string, arg1 []byte) (stat FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	resp, err := c.conn.QueueRequest(&SET, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis SAVE command.
func (c *asyncClient) Save() (stat FutureBool, err Error) {
	resp, err := c.conn.QueueRequest(&SAVE, [][]byte{})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return

}

// Redis KEYS command.
func (c *asyncClient) AllKeys() (result FutureKeys, err Error) {
	return c.Keys("*")
}

// Redis KEYS command.
func (c *asyncClient) Keys(arg0 string) (result FutureKeys, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&KEYS, [][]byte{arg0bytes})
	if err == nil {
		result = newFutureKeys(resp.future.(FutureBytes))
	}
	return result, err
}

/***
// Redis SORT command.
func (c *asyncClient) Sort (arg0 string) (result redis.Sort, err Error){
	arg0bytes := []byte (arg0);

	var resp *PendingResponse;
	resp, err = c.conn.QueueRequest(&SORT, [][]byte{arg0bytes});
	if err == nil {result = resp.GetMultiBulkData();}
	return result, err;

}
***/
// Redis EXISTS command.
func (c *asyncClient) Exists(arg0 string) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)

	resp, err := c.conn.QueueRequest(&EXISTS, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis RENAME command.
func (c *asyncClient) Rename(arg0 string, arg1 string) (stat FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	resp, err := c.conn.QueueRequest(&RENAME, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis INFO command.
func (c *asyncClient) Info() (result FutureInfo, err Error) {
	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&INFO, [][]byte{})
	if err == nil {
		result = newFutureInfo(resp.future.(FutureBytes))
	}
	return result, err
}

// Redis PING command.
func (c *asyncClient) Ping() (stat FutureBool, err Error) {
	resp, err := c.conn.QueueRequest(&PING, [][]byte{})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis SETNX command.
func (c *asyncClient) Setnx(arg0 string, arg1 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	resp, err := c.conn.QueueRequest(&SETNX, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis GETSET command.
func (c *asyncClient) Getset(arg0 string, arg1 []byte) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&GETSET, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis MGET command.
func (c *asyncClient) Mget(arg0 string, arg1 []string) (result FutureBytesArray, err Error) {
	//	arg0bytes := []byte(arg0)
	//	arg1bytes := concatAndGetBytes(arg1, " ")
	args := appendAndConvert(arg0, arg1...)

	var resp *PendingResponse
	//	resp, err = c.conn.QueueRequest(&MGET, [][]byte{arg0bytes, arg1bytes})
	resp, err = c.conn.QueueRequest(&MGET, args)
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis INCR command.
func (c *asyncClient) Incr(arg0 string) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&INCR, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis INCRBY command.
func (c *asyncClient) Incrby(arg0 string, arg1 int64) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&INCRBY, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis DECR command.
func (c *asyncClient) Decr(arg0 string) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&DECR, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis DECRBY command.
func (c *asyncClient) Decrby(arg0 string, arg1 int64) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&DECRBY, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis DEL command.
func (c *asyncClient) Del(arg0 string) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&DEL, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis RANDOMKEY command.
func (c *asyncClient) Randomkey() (result FutureString, err Error) {
	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&RANDOMKEY, [][]byte{})
	if err == nil {
		result = resp.future.(FutureString) // REVU - this is broken
	}
	return result, err

}

// Redis RENAMENX command.
func (c *asyncClient) Renamenx(arg0 string, arg1 string) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&RENAMENX, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis DBSIZE command.
func (c *asyncClient) Dbsize() (result FutureInt64, err Error) {
	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&DBSIZE, [][]byte{})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis EXPIRE command.
func (c *asyncClient) Expire(arg0 string, arg1 int64) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&EXPIRE, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis TTL command.
func (c *asyncClient) Ttl(arg0 string) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&TTL, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis RPUSH command.
func (c *asyncClient) Rpush(arg0 string, arg1 []byte) (stat FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	resp, err := c.conn.QueueRequest(&RPUSH, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis LPUSH command.
func (c *asyncClient) Lpush(arg0 string, arg1 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	resp, err := c.conn.QueueRequest(&LPUSH, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}

	return
}

// Redis LSET command.
func (c *asyncClient) Lset(arg0 string, arg1 int64, arg2 []byte) (stat FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(strconv.FormatInt(arg1, 10))
	arg2bytes := arg2

	resp, err := c.conn.QueueRequest(&LSET, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis LREM command.
func (c *asyncClient) Lrem(key string, value []byte, count int64) (result FutureInt64, err Error) {
	arg0bytes := []byte(key)
	arg1bytes := value
	arg2bytes := []byte(strconv.FormatInt(count, 10))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&LREM, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis LLEN command.
func (c *asyncClient) Llen(arg0 string) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&LLEN, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis LRANGE command.
func (c *asyncClient) Lrange(arg0 string, arg1 int64, arg2 int64) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(strconv.FormatInt(arg1, 10))
	arg2bytes := []byte(strconv.FormatInt(arg2, 10))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&LRANGE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis LTRIM command.
func (c *asyncClient) Ltrim(arg0 string, arg1 int64, arg2 int64) (stat FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))
	arg2bytes := []byte(fmt.Sprintf("%d", arg2))

	resp, err := c.conn.QueueRequest(&LTRIM, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis LINDEX command.
func (c *asyncClient) Lindex(arg0 string, arg1 int64) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&LINDEX, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis LPOP command.
func (c *asyncClient) Lpop(arg0 string) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&LPOP, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis BLPOP command.
func (c *asyncClient) Blpop(arg0 string, timeout int) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprint(timeout))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&BLPOP, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis RPOP command.
func (c *asyncClient) Rpop(arg0 string) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&RPOP, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis BRPOP command.
func (c *asyncClient) Brpop(arg0 string, timeout int) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprint(timeout))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&BRPOP, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis RPOPLPUSH command.
func (c *asyncClient) Rpoplpush(arg0 string, arg1 string) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&RPOPLPUSH, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis BRPOPLPUSH command.
func (c *asyncClient) Brpoplpush(arg0 string, arg1 string, timeout int) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)
	arg2bytes := []byte(fmt.Sprint(timeout))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&BRPOPLPUSH, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis SADD command.
func (c *asyncClient) Sadd(arg0 string, arg1 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&SADD, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis SREM command.
func (c *asyncClient) Srem(arg0 string, arg1 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&SREM, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis SISMEMBER command.
func (c *asyncClient) Sismember(arg0 string, arg1 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&SISMEMBER, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis SMOVE command.
func (c *asyncClient) Smove(arg0 string, arg1 string, arg2 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)
	arg2bytes := arg2

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&SMOVE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis SCARD command.
func (c *asyncClient) Scard(arg0 string) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&SCARD, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis SINTER command.
func (c *asyncClient) Sinter(arg0 string, arg1 []string) (result FutureBytesArray, err Error) {
	//	arg0bytes := []byte(arg0)
	//	arg1bytes := concatAndGetBytes(arg1, " ")
	args := appendAndConvert(arg0, arg1...)
	var resp *PendingResponse
	//	resp, err = c.conn.QueueRequest(&SINTER, [][]byte{arg0bytes, arg1bytes})
	resp, err = c.conn.QueueRequest(&SINTER, args)
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis SINTERSTORE command.
func (c *asyncClient) Sinterstore(arg0 string, arg1 []string) (stat FutureBool, err Error) {
	//	arg0bytes := []byte(arg0)
	//	arg1bytes := concatAndGetBytes(arg1, " ")
	args := appendAndConvert(arg0, arg1...)

	//	resp, err := c.conn.QueueRequest(&SINTERSTORE, [][]byte{arg0bytes, arg1bytes})
	resp, err := c.conn.QueueRequest(&SINTERSTORE, args)
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis SUNION command.
func (c *asyncClient) Sunion(arg0 string, arg1 []string) (result FutureBytesArray, err Error) {
	//	arg0bytes := []byte(arg0)
	//	arg1bytes := concatAndGetBytes(arg1, " ")
	args := appendAndConvert(arg0, arg1...)

	var resp *PendingResponse
	//	resp, err = c.conn.QueueRequest(&SUNION, [][]byte{arg0bytes, arg1bytes})
	resp, err = c.conn.QueueRequest(&SUNION, args)
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis SUNIONSTORE command.
func (c *asyncClient) Sunionstore(arg0 string, arg1 []string) (stat FutureBool, err Error) {
	//	arg0bytes := []byte(arg0)
	//	arg1bytes := concatAndGetBytes(arg1, " ")
	args := appendAndConvert(arg0, arg1...)

	//	resp, err := c.conn.QueueRequest(&SUNIONSTORE, [][]byte{arg0bytes, arg1bytes})
	resp, err := c.conn.QueueRequest(&SUNIONSTORE, args)
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis SDIFF command.
func (c *asyncClient) Sdiff(arg0 string, arg1 []string) (result FutureBytesArray, err Error) {
	//	arg0bytes := []byte(arg0)
	//	arg1bytes := concatAndGetBytes(arg1, " ")
	args := appendAndConvert(arg0, arg1...)

	var resp *PendingResponse
	//	resp, err = c.conn.QueueRequest(&SDIFF, [][]byte{arg0bytes, arg1bytes})
	resp, err = c.conn.QueueRequest(&SDIFF, args)
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis SDIFFSTORE command.
func (c *asyncClient) Sdiffstore(arg0 string, arg1 []string) (stat FutureBool, err Error) {
	//	arg0bytes := []byte(arg0)
	//	arg1bytes := concatAndGetBytes(arg1, " ")
	args := appendAndConvert(arg0, arg1...)

	//	resp, err := c.conn.QueueRequest(&SDIFFSTORE, [][]byte{arg0bytes, arg1bytes})
	resp, err := c.conn.QueueRequest(&SDIFFSTORE, args)
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis SMEMBERS command.
func (c *asyncClient) Smembers(arg0 string) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&SMEMBERS, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis SRANDMEMBER command.
func (c *asyncClient) Srandmember(arg0 string) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&SRANDMEMBER, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis ZADD command.
func (c *asyncClient) Zadd(arg0 string, arg1 float64, arg2 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%e", arg1))
	arg2bytes := arg2

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&ZADD, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis ZREM command.
func (c *asyncClient) Zrem(arg0 string, arg1 []byte) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&ZREM, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis ZCARD command.
func (c *asyncClient) Zcard(arg0 string) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&ZCARD, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

// Redis ZSCORE command.
func (c *asyncClient) Zscore(arg0 string, arg1 []byte) (result FutureFloat64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&ZSCORE, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = newFutureFloat64(resp.future.(FutureBytes))
	}
	return result, err

}

// Redis ZRANGE command.
func (c *asyncClient) Zrange(arg0 string, arg1 int64, arg2 int64) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))
	arg2bytes := []byte(fmt.Sprintf("%d", arg2))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&ZRANGE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis ZREVRANGE command.
func (c *asyncClient) Zrevrange(arg0 string, arg1 int64, arg2 int64) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))
	arg2bytes := []byte(fmt.Sprintf("%d", arg2))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&ZREVRANGE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis ZRANGEBYSCORE command.
func (c *asyncClient) Zrangebyscore(arg0 string, arg1 float64, arg2 float64) (result FutureBytesArray, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%e", arg1))
	arg2bytes := []byte(fmt.Sprintf("%e", arg2))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&ZRANGEBYSCORE, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		result = resp.future.(FutureBytesArray)
	}
	return result, err

}

// Redis HGET command.
func (c *asyncClient) Hget(arg0 string, arg1 string) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&HGET, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis HSET command.
func (c *asyncClient) Hset(arg0 string, arg1 string, arg2 []byte) (stat FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(arg1)
	arg2bytes := arg2

	resp, err := c.conn.QueueRequest(&HSET, [][]byte{arg0bytes, arg1bytes, arg2bytes})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis HGETALL command.
func (c *asyncClient) Hgetall(arg0 string) (result FutureBytes, err Error) {
	arg0bytes := []byte(arg0)

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&HGETALL, [][]byte{arg0bytes})
	if err == nil {
		result = resp.future.(FutureBytes)
	}
	return result, err

}

// Redis FLUSHDB command.
func (c *asyncClient) Flushdb() (stat FutureBool, err Error) {
	resp, err := c.conn.QueueRequest(&FLUSHDB, [][]byte{})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis FLUSHALL command.
func (c *asyncClient) Flushall() (stat FutureBool, err Error) {
	resp, err := c.conn.QueueRequest(&FLUSHALL, [][]byte{})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis MOVE command.
func (c *asyncClient) Move(arg0 string, arg1 int64) (result FutureBool, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := []byte(fmt.Sprintf("%d", arg1))

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&MOVE, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureBool)
	}
	return result, err

}

// Redis BGSAVE command.
func (c *asyncClient) Bgsave() (stat FutureBool, err Error) {
	resp, err := c.conn.QueueRequest(&BGSAVE, [][]byte{})
	if err == nil {
		stat = resp.future.(FutureBool)
	}

	return
}

// Redis LASTSAVE command.
func (c *asyncClient) Lastsave() (result FutureInt64, err Error) {
	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&LASTSAVE, [][]byte{})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err

}

func (c *asyncClient) Publish(arg0 string, arg1 []byte) (result FutureInt64, err Error) {
	arg0bytes := []byte(arg0)
	arg1bytes := arg1

	var resp *PendingResponse
	resp, err = c.conn.QueueRequest(&PUBLISH, [][]byte{arg0bytes, arg1bytes})
	if err == nil {
		result = resp.future.(FutureInt64)
	}
	return result, err
}
