package main

import (
	"flag"
	"fmt"
	"log"
	"redis"
	"runtime"
	"time"
)

// ----------------------------------------------------------------------------
// types and props
// ----------------------------------------------------------------------------

// redis task function type def
type redisTask func(id string, signal chan int, client redis.AsyncClient, iterations int)

// task info
type taskSpec struct {
	task redisTask
	name string
}

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

// workers option.  default is equiv to -w=10000 on command line
var workers = flag.Int("w", 10000, "number of concurrent workers")

// opcnt option.  default is equiv to -n=20 on command line
var opcnt = flag.Int("n", 20, "number of task iterations per worker")

// ----------------------------------------------------------------------------
// benchmarker
// ----------------------------------------------------------------------------

func main() {
	// DEBUG
	runtime.GOMAXPROCS(2)

	log.SetPrefix("[go-redis|bench] ")
	flag.Parse()

	fmt.Printf("\n\n== Bench synchclient == %d goroutines 1 AsyncClient  -- %d opts each --- \n\n", *workers, *opcnt)

	for _, task := range tasks {
		// REVU: creates a new client for each task run
		// wondering if using the same conn throughout will be more realistic?
		// regardless flushdb per task is OK
		benchTask(task, *opcnt, *workers, true)
	}

}

// Use a single redis.AsyncClient with specified number
// of workers to bench concurrent load on the async client
func benchTask(taskspec taskSpec, iterations int, workers int, printReport bool) (delta time.Duration, err error) {
	// channel to signal completion
	signal := make(chan int, workers)

	// spec and initialize an AsyncClient
	// will flush your db13 as noted in README ..
	spec := redis.DefaultSpec().Db(13).Password("go-redis")
	client, e := redis.NewAsynchClientWithSpec(spec)
	if e != nil {
		log.Println("Error creating client for worker: ", e)
		return -1, e
	}

	defer client.Quit()

	// panics
	setup(client)

	t0 := time.Now()
	for i := 0; i < workers; i++ {
		id := fmt.Sprintf("%d", i)
		go taskspec.task(id, signal, client, iterations)
	}

	// wait for completion
	for i := 0; i < workers; i++ {
		<-signal
	}
	delta = time.Now().Sub(t0)

	if printReport {
		report(taskspec.name, workers, delta, iterations*workers)
	}

	return
}

func setup(client redis.AsyncClient) {
	fr, e := client.Flushdb()
	if e != nil {
		log.Println("Error creating client for worker: ", e)
		log.Println("fr: ", fr)
		panic(e)
	}
	frr, e2 := fr.Get()
	if e2 != nil {
		log.Println("Error creating client for worker: ", e2)
		log.Println("frr: ", frr)
		panic(e)
	}
}

func report(cmd string, workers int, delta time.Duration, cnt int) {
	log.Printf("---\n")
	log.Printf("cmd: %s\n", cmd)
	log.Printf("%d goroutines 1 asyncClient %d iterations of %s in %d msecs\n", workers, cnt, cmd, delta/time.Millisecond)
	log.Printf("---\n\n")
}

// ----------------------------------------------------------------------------
// redis tasks
// ----------------------------------------------------------------------------

func doPing(id string, signal chan int, client redis.AsyncClient, cnt int) {
	var fr redis.FutureBool
	for i := 0; i < cnt; i++ {
		fr, _ = client.Ping()
	}
	fr.Get()
	signal <- 1
}
func doIncr(id string, signal chan int, client redis.AsyncClient, cnt int) {
	key := "ctr-" + id
	var fr redis.FutureInt64
	for i := 0; i < cnt; i++ {
		fr, _ = client.Incr(key)
	}
	v, _ := fr.Get()
	if v != int64(cnt) {
		log.Fatalf("BUG: expecting counter %s to be %d but it is %d\n", key, cnt, v)
		panic(1)
	}
	// debug sanity check
	//	log.Printf("worker[%s] - last INCR result %s=%d\n", id, key, v)
	signal <- 1
}
func doDecr(id string, signal chan int, client redis.AsyncClient, cnt int) {
	var fr redis.FutureInt64
	key := "ctr-" + id
	for i := 0; i < cnt; i++ {
		fr, _ = client.Decr(key)
	}
	fr.Get()
	signal <- 1
}
func doSet(id string, signal chan int, client redis.AsyncClient, cnt int) {
	key := "set-" + id
	value := []byte("foo")
	for i := 0; i < cnt; i++ {
		client.Set(key, value)
	}
	signal <- 1
}
func doGet(id string, signal chan int, client redis.AsyncClient, cnt int) {
	key := "set-" + id
	for i := 0; i < cnt; i++ {
		client.Get(key)
	}
	signal <- 1
}
func doLpush(id string, signal chan int, client redis.AsyncClient, cnt int) {
	key := "list-L-" + id
	value := []byte("foo")
	for i := 0; i < cnt; i++ {
		client.Lpush(key, value)
	}
	signal <- 1
}
func doLpop(id string, signal chan int, client redis.AsyncClient, cnt int) {
	key := "list-L-" + id
	for i := 0; i < cnt; i++ {
		client.Lpop(key)
	}
	signal <- 1
}

func doRpush(id string, signal chan int, client redis.AsyncClient, cnt int) {
	key := "list-R-" + id
	value := []byte("foo")
	for i := 0; i < cnt; i++ {
		client.Rpush(key, value)
	}
	signal <- 1
}
func doRpop(id string, signal chan int, client redis.AsyncClient, cnt int) {
	key := "list-R" + id
	for i := 0; i < cnt; i++ {
		client.Rpop(key)
	}
	signal <- 1
}
