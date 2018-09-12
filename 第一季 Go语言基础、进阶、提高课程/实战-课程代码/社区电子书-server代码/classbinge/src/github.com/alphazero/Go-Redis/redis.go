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

// Package redis provides both clients and connectors for the Redis
// server.  Both synchronous and asynchronous interaction modes are
// supported.  Asynchronous clients (using the asynchronous connection)
// use pipelining.
//
// Synchronous semantics are defined by redis.Client interface type
//
//
// Usage example:
//
//  func usingRedisSync () Error {
//      spec := DefaultConnectionSpec();
//      pipeline := NewAsynchClient(spec);
//
//      value, reqErr := pipeline.Get("my-key");
//      if reqErr != nil { return withError (reqErr); }
//  }
//
// Asynchronous semantics are defined by redis.AsyncClient interface type.
// Note that these clients use a pipelining connector and a single instance
// can be used by multiple go routines.  Use of pipelining increases throughput
// at the cost of latency.  If low latency is more important to you than
// throughput, and you require async call semantics, then you should use only
// 1 go routine per AsyncClient connection.
//
// Usage example without timeouts (caller blocks on Get until the response
// from Redis has been processed.)
//
//  func usingRedisAsync () Error {
//      spec := DefaultConnectionSpec();
//      pipeline := NewRedisPipeline(spec);
//
//      // async invoke of GET my-key
//      // futureBytes is a FutureBytes that will have the result of the
//      // Redis GET operation.
//      futureBytes, reqErr := pipline.Get("my-key");
//      if reqErr != nil {
//          return withError (reqErr);
//      }
//
//      // ... note that you could issue additional redis commands here ...
//
//      []byte, execErr := futureBytes.Get();
//      if execErr != nil {
//          return withError (execErr);
//      }
//  }
//
// Usage example with timeouts - same Redis op as above but here we use
// TryGet on the Future result with a timeout of 1 msecs:
//
//  func usingRedisAsync () Error {
//      spec := DefaultConnectionSpec();
//      pipeline := NewRedisPipeline(spec);
//
//      // futureBytes is a FutureBytes
//      futureBytes, reqErr := pipline.Get("my-key");
//      if reqErr != nil { return withError (reqErr); }
//
//      // ... note that you could issue additional redis commands here ...
//
//      []byte, execErr := futureBytes.Get();
//      if execErr != nil {
//          return withError (execErr);
//      }
//
//      timeout := 1000000; // wait 1 msec for result
//      []byte, execErr, ok := futureBytes.TryGet (timeout);
//      if !ok {
//          .. handle timeout here
//      }
//      else {
//          if execErr != nil {
//              return withError (execErr);
//          }
//      }
//  }
//
package redis

import (
	"flag"
)

// The synchronous call semantics Client interface.
//
// Method names map one to one to the Redis command set.
// All methods may return an redis.Error, which is either
// a system error (runtime issue or bug) or Redis error (i.e. user error)
// See Error in this package for details of its interface.
//
// The synchronous client interface provides blocking call semantics supported by
// a distinct request/reply sequence at the connector level.
//
// Method names map one to one to the Redis command set.
//
// All methods may return an redis.Error, which is either a Redis error (from
// the server), or a system error indicating a runtime issue (or bug).
// See Error in this package for details of its interface.
type Client interface {

	// Redis QUIT command.
	Quit() (err Error)

	// Redis GET command.
	Get(key string) (result []byte, err Error)

	// Redis TYPE command.
	Type(key string) (result KeyType, err Error)

	// Redis SET command.
	Set(key string, arg1 []byte) Error

	// Redis SAVE command.
	Save() Error

	// Redis KEYS command using "*" wildcard 
	AllKeys() (result []string, err Error)

	// Redis KEYS command.
	Keys(key string) (result []string, err Error)

	// Redis EXISTS command.
	Exists(key string) (result bool, err Error)

	// Redis RENAME command.
	Rename(key, arg1 string) Error

	// Redis INFO command.
	Info() (result map[string]string, err Error)

	// Redis PING command.
	Ping() Error

	// Redis SETNX command.
	Setnx(key string, arg1 []byte) (result bool, err Error)

	// Redis GETSET command.
	Getset(key string, arg1 []byte) (result []byte, err Error)

	// Redis MGET command.
	Mget(key string, arg1 []string) (result [][]byte, err Error)

	// Redis INCR command.
	Incr(key string) (result int64, err Error)

	// Redis INCRBY command.
	Incrby(key string, arg1 int64) (result int64, err Error)

	// Redis DECR command.
	Decr(key string) (result int64, err Error)

	// Redis DECRBY command.
	Decrby(key string, arg1 int64) (result int64, err Error)

	// Redis DEL command.
	Del(key string) (result bool, err Error)

	// Redis RANDOMKEY command.
	Randomkey() (result string, err Error)

	// Redis RENAMENX command.
	Renamenx(key string, arg1 string) (result bool, err Error)

	// Redis DBSIZE command.
	Dbsize() (result int64, err Error)

	// Redis EXPIRE command.
	Expire(key string, arg1 int64) (result bool, err Error)

	// Redis TTL command.
	Ttl(key string) (result int64, err Error)

	// Redis RPUSH command.
	Rpush(key string, arg1 []byte) Error

	// Redis LPUSH command.
	Lpush(key string, arg1 []byte) Error

	// Redis LSET command.
	Lset(key string, arg1 int64, arg2 []byte) Error

	// Redis LREM command.
	Lrem(key string, arg1 []byte, arg2 int64) (result int64, err Error)

	// Redis LLEN command.
	Llen(key string) (result int64, err Error)

	// Redis LRANGE command.
	Lrange(key string, arg1 int64, arg2 int64) (result [][]byte, err Error)

	// Redis LTRIM command.
	Ltrim(key string, arg1 int64, arg2 int64) Error

	// Redis LINDEX command.
	Lindex(key string, arg1 int64) (result []byte, err Error)

	// Redis LPOP command.
	Lpop(key string) (result []byte, err Error)

	// Redis BLPOP command.
	Blpop(key string, timeout int) (result [][]byte, err Error)

	// Redis RPOP command.
	Rpop(key string) (result []byte, err Error)

	// Redis BRPOP command.
	Brpop(key string, timeout int) (result [][]byte, err Error)

	// Redis RPOPLPUSH command.
	Rpoplpush(key string, arg1 string) (result []byte, err Error)

	// Redis BRPOPLPUSH command.
	Brpoplpush(key string, arg1 string, timeout int) (result [][]byte, err Error)

	// Redis SADD command.
	Sadd(key string, arg1 []byte) (result bool, err Error)

	// Redis SREM command.
	Srem(key string, arg1 []byte) (result bool, err Error)

	// Redis SISMEMBER command.
	Sismember(key string, arg1 []byte) (result bool, err Error)

	// Redis SMOVE command.
	Smove(key string, arg1 string, arg2 []byte) (result bool, err Error)

	// Redis SCARD command.
	Scard(key string) (result int64, err Error)

	// Redis SINTER command.
	Sinter(key string, arg1 []string) (result [][]byte, err Error)

	// Redis SINTERSTORE command.
	Sinterstore(key string, arg1 []string) Error

	// Redis SUNION command.
	Sunion(key string, arg1 []string) (result [][]byte, err Error)

	// Redis SUNIONSTORE command.
	Sunionstore(key string, arg1 []string) Error

	// Redis SDIFF command.
	Sdiff(key string, arg1 []string) (result [][]byte, err Error)

	// Redis SDIFFSTORE command.
	Sdiffstore(key string, arg1 []string) Error

	// Redis SMEMBERS command.
	Smembers(key string) (result [][]byte, err Error)

	// Redis SRANDMEMBER command.
	Srandmember(key string) (result []byte, err Error)

	// Redis ZADD command.
	Zadd(key string, arg1 float64, arg2 []byte) (result bool, err Error)

	// Redis ZREM command.
	Zrem(key string, arg1 []byte) (result bool, err Error)

	// Redis ZCARD command.
	Zcard(key string) (result int64, err Error)

	// Redis ZSCORE command.
	Zscore(key string, arg1 []byte) (result float64, err Error)

	// Redis ZRANGE command.
	Zrange(key string, arg1 int64, arg2 int64) (result [][]byte, err Error)

	// Redis ZREVRANGE command.
	Zrevrange(key string, arg1 int64, arg2 int64) (result [][]byte, err Error)

	// Redis ZRANGEBYSCORE command.
	Zrangebyscore(key string, arg1 float64, arg2 float64) (result [][]byte, err Error)

	// Redis HGET command.
	Hget(key string, hashkey string) (result []byte, err Error)

	// Redis HSET command.
	Hset(key string, hashkey string, arg1 []byte) Error

	// Redis HGETALL command.
	Hgetall(key string) (result [][]byte, err Error)

	// Redis FLUSHDB command.
	Flushdb() Error

	// Redis FLUSHALL command.
	Flushall() Error

	// Redis MOVE command.
	Move(key string, arg1 int64) (result bool, err Error)

	// Redis BGSAVE command.
	Bgsave() Error

	// Redis LASTSAVE command.
	Lastsave() (result int64, err Error)

	// Redis PUBLISH command.
	// Publishes a message to the named channels.  This is a blocking call.
	//
	// Returns the number of PubSub subscribers that received the message.
	// OR error if any.
	Publish(channel string, message []byte) (recieverCout int64, err Error)
}

// The asynchronous client interface provides asynchronous call semantics with
// future results supporting both blocking and try-and-timeout result accessors.
//
// Each method provides a type-safe future result return value, in addition to
// any (system) errors encountered in queuing the request.
//
// The returned value may be ignored by clients that are not interested in the
// future response (for example on SET("foo", data)).  Alternatively, the caller
// may retain the future result referenced and perform blocking and/or timed wait
// gets on the expected response.
//
// Get() or TryGet() on the future result will return any Redis errors that were sent by
// the server, or, Go-Redis (system) errors encountered in processing the response.
type AsyncClient interface {

	// Redis QUIT command.
	Quit() (status FutureBool, err Error)

	// Redis GET command.
	Get(key string) (result FutureBytes, err Error)

	// Redis TYPE command.
	Type(key string) (result FutureKeyType, err Error)

	// Redis SET command.
	Set(key string, arg1 []byte) (status FutureBool, err Error)

	// Redis SAVE command.
	Save() (status FutureBool, err Error)

	// Redis KEYS command using "*" wildcard 
	AllKeys() (result FutureKeys, err Error)

	// Redis KEYS command.
	Keys(key string) (result FutureKeys, err Error)

	// Redis EXISTS command.
	Exists(key string) (result FutureBool, err Error)

	// Redis RENAME command.
	Rename(key, arg1 string) (status FutureBool, err Error)

	// Redis INFO command.
	Info() (result FutureInfo, err Error)

	// Redis PING command.
	Ping() (status FutureBool, err Error)

	// Redis SETNX command.
	Setnx(key string, arg1 []byte) (result FutureBool, err Error)

	// Redis GETSET command.
	Getset(key string, arg1 []byte) (result FutureBytes, err Error)

	// Redis MGET command.
	Mget(key string, arg1 []string) (result FutureBytesArray, err Error)

	// Redis INCR command.
	Incr(key string) (result FutureInt64, err Error)

	// Redis INCRBY command.
	Incrby(key string, arg1 int64) (result FutureInt64, err Error)

	// Redis DECR command.
	Decr(key string) (result FutureInt64, err Error)

	// Redis DECRBY command.
	Decrby(key string, arg1 int64) (result FutureInt64, err Error)

	// Redis DEL command.
	Del(key string) (result FutureBool, err Error)

	// Redis RANDOMKEY command.
	Randomkey() (result FutureString, err Error)

	// Redis RENAMENX command.
	Renamenx(key string, arg1 string) (result FutureBool, err Error)

	// Redis DBSIZE command.
	Dbsize() (result FutureInt64, err Error)

	// Redis EXPIRE command.
	Expire(key string, arg1 int64) (result FutureBool, err Error)

	// Redis TTL command.
	Ttl(key string) (result FutureInt64, err Error)

	// Redis RPUSH command.
	Rpush(key string, arg1 []byte) (status FutureBool, err Error)

	// Redis LPUSH command.
	Lpush(key string, arg1 []byte) (status FutureBool, err Error)

	// Redis LSET command.
	Lset(key string, arg1 int64, arg2 []byte) (status FutureBool, err Error)

	// Redis LREM command.
	Lrem(key string, arg1 []byte, arg2 int64) (result FutureInt64, err Error)

	// Redis LLEN command.
	Llen(key string) (result FutureInt64, err Error)

	// Redis LRANGE command.
	Lrange(key string, arg1 int64, arg2 int64) (result FutureBytesArray, err Error)

	// Redis LTRIM command.
	Ltrim(key string, arg1 int64, arg2 int64) (status FutureBool, err Error)

	// Redis LINDEX command.
	Lindex(key string, arg1 int64) (result FutureBytes, err Error)

	// Redis LPOP command.
	Lpop(key string) (result FutureBytes, err Error)

	// Redis RPOP command.
	Rpop(key string) (result FutureBytes, err Error)

	// Redis RPOPLPUSH command.
	Rpoplpush(key string, arg1 string) (result FutureBytes, err Error)

	// Redis SADD command.
	Sadd(key string, arg1 []byte) (result FutureBool, err Error)

	// Redis SREM command.
	Srem(key string, arg1 []byte) (result FutureBool, err Error)

	// Redis SISMEMBER command.
	Sismember(key string, arg1 []byte) (result FutureBool, err Error)

	// Redis SMOVE command.
	Smove(key string, arg1 string, arg2 []byte) (result FutureBool, err Error)

	// Redis SCARD command.
	Scard(key string) (result FutureInt64, err Error)

	// Redis SINTER command.
	Sinter(key string, arg1 []string) (result FutureBytesArray, err Error)

	// Redis SINTERSTORE command.
	Sinterstore(key string, arg1 []string) (status FutureBool, err Error)

	// Redis SUNION command.
	Sunion(key string, arg1 []string) (result FutureBytesArray, err Error)

	// Redis SUNIONSTORE command.
	Sunionstore(key string, arg1 []string) (status FutureBool, err Error)

	// Redis SDIFF command.
	Sdiff(key string, arg1 []string) (result FutureBytesArray, err Error)

	// Redis SDIFFSTORE command.
	Sdiffstore(key string, arg1 []string) (status FutureBool, err Error)

	// Redis SMEMBERS command.
	Smembers(key string) (result FutureBytesArray, err Error)

	// Redis SRANDMEMBER command.
	Srandmember(key string) (result FutureBytes, err Error)

	// Redis ZADD command.
	Zadd(key string, arg1 float64, arg2 []byte) (result FutureBool, err Error)

	// Redis ZREM command.
	Zrem(key string, arg1 []byte) (result FutureBool, err Error)

	// Redis ZCARD command.
	Zcard(key string) (result FutureInt64, err Error)

	// Redis ZSCORE command.
	Zscore(key string, arg1 []byte) (result FutureFloat64, err Error)

	// Redis ZRANGE command.
	Zrange(key string, arg1 int64, arg2 int64) (result FutureBytesArray, err Error)

	// Redis ZREVRANGE command.
	Zrevrange(key string, arg1 int64, arg2 int64) (result FutureBytesArray, err Error)

	// Redis ZRANGEBYSCORE command.
	Zrangebyscore(key string, arg1 float64, arg2 float64) (result FutureBytesArray, err Error)

	// Redis FLUSHDB command.
	Flushdb() (status FutureBool, err Error)

	// Redis FLUSHALL command.
	Flushall() (status FutureBool, err Error)

	// Redis MOVE command.
	Move(key string, arg1 int64) (result FutureBool, err Error)

	// Redis BGSAVE command.
	Bgsave() (status FutureBool, err Error)

	// Redis LASTSAVE command.
	Lastsave() (result FutureInt64, err Error)

	// Redis PUBLISH command.
	// Publishes a message to the named channels.
	//
	// Returns the future for number of PubSub subscribers that received the message.
	// OR error if any.
	Publish(channel string, message []byte) (recieverCountFuture FutureInt64, err Error)
}

// REVU - ALL THE COMMENS NEEDS REVIEW AND REVISION
// Most critical is the depth of channel for message publication
// Demux is problematic on many levels.  Simply provide an example
// such as
/*
	demux := make(PubSubChannel, depth)

	__, __, serr :=  pubsub.Subscribe(subid); if serr != nil {
	 // handle error
	}
	schan := pubsub.Mesages(subid)

	go func() {
       select
	}()

*/

// PubSub Client
//
// Strictly speaking, this client can only subscribe, receive messages, and
// unsubscribe.  Publishing to Redis PubSub channels is done via the standard
// clients (either sync or async); see the Publish() method on Client and AsyncClient.
//
// Once created, the PubSub client has a message channel (of type <-chan []byte)
// that the end-user can select, dequeue, etc.
//
// This client (very) slightly
// modifies the native pubsub client's semantics in that it does NOT post the
// 'subscribe' or 'unsubscribe' ACKs of Redis server on the exposed chan.  These
// ACKs are effectively captured and returned via the returned results of the
// PubSubClient's Subscribe() and Unsubscribe() methods, respectively.
//
// The subscribe and unsubscribe methods are both blocking (synchronous).  The
// messages published via the incoming chan are naturally asynchronous.
//
// Given the fact that Redis PSUBSCRIBE to channel names that do NOT end in *
// is identical to SUBSCRIBE to the same, PubSubClient only exposes Subscribe and
// Unsubscribe methods and supporting implementation are expected to always use
// the Redis PSUBSCRIBE and PUNSUBSCRIBE commands.  For example, if one issues
// PSUBSCRIBE foo/* (via telnet) to Redis, and then UNSUBSCRIBE or PUNSUBSCRIBE foo/bar,
// messages published to foo/bar will still be received in the (telnet) client.  So
// given that Redis does NOT filter subscriptions and it merely has a 1-1 mapping
// to subscribed and unsubscribed patterns, PSUBSCRIBE foo is equivalent to SUBSCRIBE foo.
// These facts inform the design decision to keep the API of PubSubClient simple and
// not expose explicit pattern or explicit (un)subscription.
//
// Also note that (per Redis semantics) ALL subscribed channels will publish to the
// single chan exposed by this client.  For practical applications, you will minimally
// want to use one PubSubClient per PubSub channel priority category.  For example,
// if your system has general priority application level and high priority critical system
// level PubSub channels, you should at least create 2 clients, one per priority category.
//
// Like all Go-Redis clients, you can (and should) Quit() once you are done with
// the client.
//
type PubSubClient interface {

	// returns the incoming messages channel for this client, or nil
	// if no such subscription is active.
	// In event of Unsubscribing from a Redis channel, the
	// client will close this channel.
	Messages(topic string) PubSubChannel

	// return the subscribed channel ids, whether specificly named, or
	// pattern based.
	Subscriptions() []string

	// Redis PSUBSCRIBE command.
	// Subscribes to one or more pubsub channels.
	// This is a blocking call.
	// Channel names can be explicit or pattern based (ending in '*')
	//
	// Returns the number of currently subscribed channels OR error (if any)
	//	Subscribe(channel string, otherChannels ...string) (messages PubSubChannel, subscriptionCount int, err Error)
	Subscribe(topic string, otherTopics ...string) (err Error)

	// Redis PUNSUBSCRIBE command.
	// unsubscribe from 1 or more pubsub channels.  If arg is nil,
	// client unsubcribes from ALL subscribed channels.
	// This is a blocking call.
	// Channel names can be explicit or pattern based (ending in '*')
	//
	// Returns the number of currently subscribed channels OR error (if any)
	Unsubscribe(channels ...string) (err Error)

	// Quit closes the client and client reference can be disposed.
	// This is a blocking call.
	// Returns error, if any, e.g. network issues.
	Quit() Error
}

// PubSubChannels are used by clients to forward received PubSub messages from Redis
// See PubSubClient interface for details.
type PubSubChannel <-chan []byte

// ----------------------------------------------------------------------------
// package initiatization and internal ops and flags
// ----------------------------------------------------------------------------

// ----------------
// flags
//
// go-redis will make use of command line flags where available.  flag names
// for this package are all prefixed by "redis:" to prevent possible name collisions.
//
// Note that because flag.Parse() can only be called once, add all flags must have
// been defined by the time it is called, we CAN NOT call flag.Parse() in our init()
// function, as that will prevent any invokers from defining their own flags.
//
// It is your responsibility to call flag.Parse() at the start of your main().

// redis:d
//
// global debug flag for redis package components.
// 
var _debug *bool = flag.Bool("redis:d", false, "debug flag for go-redis") // TEMP: should default to false
func debug() bool {
	return *_debug
}
