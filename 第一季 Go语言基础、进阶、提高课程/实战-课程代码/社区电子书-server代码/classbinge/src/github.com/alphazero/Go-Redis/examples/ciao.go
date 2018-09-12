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
	"bufio"
	"flag"
	"fmt"
	"log"
	"os"
	"redis"
)

/*
	hello world, redis style.
*/

func main() {

	// Parse command-line flags; needed to let flags used by Go-Redis be parsed.
	flag.Parse()

	// create the client.  Here we are using a synchronous client.
	// Using the default ConnectionSpec, we are specifying the client to connect
	// to db 13 (e.g. SELECT 13), and a password of go-redis (e.g. AUTH go-redis)

	spec := redis.DefaultSpec().Db(13).Password("go-redis")
	client, e := redis.NewSynchClientWithSpec(spec)
	if e != nil {
		log.Println("failed to create the client", e)
		return
	}

	key := "examples/hello/user.name"
	value, e := client.Get(key)
	if e != nil {
		log.Println("error on Get", e)
		return
	}

	if value == nil {
		fmt.Printf("\nHello, don't believe we've met before!\nYour name? ")
		reader := bufio.NewReader(os.Stdin)
		user, _ := reader.ReadString(byte('\n'))
		if len(user) > 1 {
			user = user[0 : len(user)-1]
			value = []byte(user)
			client.Set(key, value)
		} else {
			fmt.Printf("vafanculo!\n")
			return
		}
	}
	fmt.Printf("Hey, ciao %s!\n", fmt.Sprintf("%s", value))
}
