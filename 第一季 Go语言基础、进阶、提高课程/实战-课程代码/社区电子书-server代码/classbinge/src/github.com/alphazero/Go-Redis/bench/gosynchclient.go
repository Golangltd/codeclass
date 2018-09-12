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

func main() {
	flag.Parse()
	fmt.Printf("\n\n=== Bench synchclient ================ %d Concurrent Clients -- %d opts each --- \n\n", *workers, *opcnt)
	for _, task := range tasks {
		benchTask(task, *opcnt, *workers, true)
	}
}

// ----------------------------------------------------------------------------
// types and props
// ----------------------------------------------------------------------------

// workers option.  default is equiv to -w=10 on command line
var workers = flag.Int("w", 10, "number of concurrent workers")

// opcnt option.  default is equiv to -n=2000 on command line
var opcnt = flag.Int("n", 2000, "number of task iterations per worker")

// array of Tasks to run in sequence
// Add a task to the list to bench to the runner.
// Tasks are run in sequence.
var tasks = []taskSpec{
	taskSpec{doPing, "PING"},
	taskSpec{doSet, "SET"},
	taskSpec{doGet, "GET"},
	taskSpec{doIncr, "INCR"},
	taskSpec{doDecr, "DECR"},
	taskSpec{doLpush, "LPUSH"},
	taskSpec{doLpop, "LPOP"},
	taskSpec{doRpush, "RPUSH"},
	taskSpec{doRpop, "RPOP"},
}

// ----------------------------------------------------------------------------
// benchmarker
// ----------------------------------------------------------------------------

// redis task function type def
type redisTask func(id string, signal chan int, client redis.Client, iterations int)

// task info
type taskSpec struct {
	task redisTask
	name string
}

func benchTask(taskspec taskSpec, iterations int, workers int, printReport bool) (delta time.Duration, err error) {
	signal := make(chan int, workers) // Buffering optional but sensible.
	clients, e := makeConcurrentClients(workers)
	if e != nil {
		return 0, e
	}
	t0 := time.Now()
	for i := 0; i < workers; i++ {
		id := fmt.Sprintf("%d", i)
		go taskspec.task(id, signal, clients[i], iterations)
	}
	for i := 0; i < workers; i++ {
		<-signal
	}
	delta = time.Now().Sub(t0)
	for i := 0; i < workers; i++ {
		clients[i].Quit() // will be deprecated soon
	}

	if printReport {
		report("concurrent "+taskspec.name, delta, iterations*workers)
	}

	return
}

func makeConcurrentClients(workers int) (clients []redis.Client, err error) {
	clients = make([]redis.Client, workers)
	for i := 0; i < workers; i++ {
		spec := redis.DefaultSpec().Db(13).Password("go-redis")
		client, e := redis.NewSynchClientWithSpec(spec)
		if e != nil {
			log.Println("Error creating client for worker: ", e)
			return nil, e
		}
		clients[i] = client
	}
	return
}

func report(cmd string, delta time.Duration, cnt int) {
	fmt.Printf("---\n")
	fmt.Printf("cmd: %s\n", cmd)
	fmt.Printf("%d iterations of %s in %d msecs\n", cnt, cmd, delta/time.Millisecond)
	fmt.Printf("---\n\n")
}

// ----------------------------------------------------------------------------
// redis tasks
// ----------------------------------------------------------------------------

func doPing(id string, signal chan int, client redis.Client, cnt int) {
	for i := 0; i < cnt; i++ {
		client.Ping()
	}
	signal <- 1
}
func doIncr(id string, signal chan int, client redis.Client, cnt int) {
	key := "ctr-" + id
	for i := 0; i < cnt; i++ {
		client.Incr(key)
	}
	signal <- 1
}
func doDecr(id string, signal chan int, client redis.Client, cnt int) {
	key := "ctr-" + id
	for i := 0; i < cnt; i++ {
		client.Decr(key)
	}
	signal <- 1
}
func doSet(id string, signal chan int, client redis.Client, cnt int) {
	key := "set-" + id
	value := []byte("foo")
	for i := 0; i < cnt; i++ {
		client.Set(key, value)
	}
	signal <- 1
}
func doGet(id string, signal chan int, client redis.Client, cnt int) {
	key := "set-" + id
	for i := 0; i < cnt; i++ {
		client.Get(key)
	}
	signal <- 1
}
func doLpush(id string, signal chan int, client redis.Client, cnt int) {
	key := "list-L-" + id
	value := []byte("foo")
	for i := 0; i < cnt; i++ {
		client.Lpush(key, value)
	}
	signal <- 1
}
func doLpop(id string, signal chan int, client redis.Client, cnt int) {
	key := "list-L-" + id
	for i := 0; i < cnt; i++ {
		client.Lpop(key)
	}
	signal <- 1
}

func doRpush(id string, signal chan int, client redis.Client, cnt int) {
	key := "list-R-" + id
	value := []byte("foo")
	for i := 0; i < cnt; i++ {
		client.Rpush(key, value)
	}
	signal <- 1
}
func doRpop(id string, signal chan int, client redis.Client, cnt int) {
	key := "list-R" + id
	for i := 0; i < cnt; i++ {
		client.Rpop(key)
	}
	signal <- 1
}
