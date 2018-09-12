//   Copyright 2009 Joubin Houshyar
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
//
package main

import (
	"flag"
	"fmt"
	"log"
	"redis"
	"time"
)

func onError(msg string, e error) error {
	log.Println(msg, "", e)
	return e
}
func failedTest(msg string) error {
	log.Println(msg)
	return nil
}
func main() {
	flag.Parse()
	cnt := 20000

	doOne(cnt)
}

func doOne(cnt int) error {

	var delta time.Duration
	spec := redis.DefaultSpec().Db(13).Password("go-redis")

	fmt.Printf("\n\n=== Bench synchclient ================ 1 Client -- %d opts --- \n", cnt)
	fmt.Println()

	client, e := redis.NewSynchClientWithSpec(spec)
	if e != nil {
		return onError("on NewSynchClient call: ", e)
	}
	if client == nil {
		return failedTest("NewSynchClient returned nil!")
	}
	//	defer client.Quit()   // will be deprecated soon
	defer client.Quit()

	client.Flushdb()

	delta = doPing(client, cnt)
	report("PING", delta, cnt)

	delta = doIncr(client, cnt)
	report("INCR", delta, cnt)

	delta = doSet(client, cnt)
	report("SET", delta, cnt)

	delta = doGet(client, cnt)
	report("GET", delta, cnt)

	delta = doSadd(client, cnt)
	report("SADD", delta, cnt)

	delta = doLpush(client, cnt)
	report("LPUSH", delta, cnt)

	delta = doRpush(client, cnt)
	report("RPUSH", delta, cnt)

	delta = doLpop(client, cnt)
	report("LPOP", delta, cnt)

	delta = doRpop(client, cnt)
	report("RPOP", delta, cnt)

	return nil
}

func report(cmd string, delta time.Duration, cnt int) {
	fmt.Printf("---\n")
	fmt.Printf("cmd: %s\n", cmd)
	fmt.Printf("%d iterations of %s in %d msecs\n", cnt, cmd, delta/time.Millisecond)
	fmt.Printf("---\n")
}

func doPing(client redis.Client, cnt int) (delta time.Duration) {
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Ping()
	}
	delta = time.Now().Sub(t0)
	client.Flushdb()
	return
}
func doIncr(client redis.Client, cnt int) (delta time.Duration) {
	key := "ctr"
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Incr(key)
	}
	delta = time.Now().Sub(t0)
	client.Flushdb()
	return
}
func doSet(client redis.Client, cnt int) (delta time.Duration) {
	key := "ctr"
	value := []byte("foo")
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Set(key, value)
	}
	delta = time.Now().Sub(t0)
	client.Flushdb()
	return
}
func doGet(client redis.Client, cnt int) (delta time.Duration) {
	key := "ctr"
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Get(key)
	}
	delta = time.Now().Sub(t0)
	client.Flushdb()
	return
}
func doSadd(client redis.Client, cnt int) (delta time.Duration) {
	key := "set"
	value := []byte("one")
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Sadd(key, value)
	}
	delta = time.Now().Sub(t0)
	client.Flushdb()
	return
}
func doLpush(client redis.Client, cnt int) (delta time.Duration) {
	key := "list-L"
	value := []byte("foo")
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Lpush(key, value)
	}
	delta = time.Now().Sub(t0)
	return
}
func doRpush(client redis.Client, cnt int) (delta time.Duration) {
	key := "list-R"
	value := []byte("foo")
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Lpush(key, value)
	}
	delta = time.Now().Sub(t0)
	return
}
func doLpop(client redis.Client, cnt int) (delta time.Duration) {
	key := "list-L"
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Lpop(key)
	}
	delta = time.Now().Sub(t0)
	return
}
func doRpop(client redis.Client, cnt int) (delta time.Duration) {
	key := "list-R"
	t0 := time.Now()
	for i := 0; i < cnt; i++ {
		client.Lpop(key)
	}
	delta = time.Now().Sub(t0)
	return
}
