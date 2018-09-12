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
//
package main

import (
	"flag"
	"fmt"
	"log"
	"redis"
)

// PubSub - PubSub message publisher example demonstrates  publishing messages
// via Redis PubSub's PUBLISH command.  Both sync and async client examples are
// provided.
//
// Messages are published to a Redis channel named "example/pubsub/channel".
// Ideally, you will want to telnet to the spec'd redis server and subscribe
// to the same channel.  Each client type will send 100 messages.
//
// This example works whether or not there are subscribers to the channel, but
// naturally the receiver count from Publish() method will depend on the number
// of subscribers.

func main() {

	// Parse command-line flags; needed to let flags used by Go-Redis be parsed.
	flag.Parse()

	// create the client.  Here we are using a synchronous client.
	// Using the default ConnectionSpec, we are specifying the client to connect
	// to db 13 (e.g. SELECT 13), and a password of go-redis (e.g. AUTH go-redis)

	spec := redis.DefaultSpec().Password("go-redis")
	channel := "example/pubsub/channel"

	// publish using sync client
	syncPublish(spec, channel)

	// publish using async client
	asyncPublish(spec, channel)

}

func syncPublish(spec *redis.ConnectionSpec, channel string) {

	client, e := redis.NewSynchClientWithSpec(spec)
	if e != nil {
		log.Println("failed to create the sync client", e)
		return
	}

	for i := 0; i < 100; i++ {
		msg := []byte(fmt.Sprintf("this is message # %d (using sync client)!", i))
		rcvCnt, err := client.Publish(channel, msg)
		if err != nil {
			fmt.Printf("Error on Publish - %s", err)
		} else {
			fmt.Printf("Message sent to %d subscribers\n", rcvCnt)
		}
	}

	client.Quit()
}

func asyncPublish(spec *redis.ConnectionSpec, channel string) {

	client, e := redis.NewAsynchClientWithSpec(spec)
	if e != nil {
		log.Println("failed to create the async client", e)
		return
	}

	// ref will ultimately point to the last future returned from Publish()
	var rcvCntFuture redis.FutureInt64
	for i := 0; i < 100; i++ {
		msg := []byte(fmt.Sprintf("this is message # %d (using async client)!", i))
		var err redis.Error
		// publish the message and don't wait for the future
		// we only care if an error was raised on publish
		rcvCntFuture, err = client.Publish(channel, msg)
		if err != nil {
			fmt.Printf("Error on Publish - %s", err)
		}
	}

	// ok, now let's wait until the last publish's future is done
	// before quiting.
	rcvCnt, fe := rcvCntFuture.Get()
	if fe != nil {
		fmt.Printf("Error on future get - %s\n", fe)
		return
	} else {
		fmt.Printf("(LAST) Message sent to %d subscribers\n", rcvCnt)
	}

	client.Quit()
}
