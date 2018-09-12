# Go-Redis

[Go][Go] Clients and Connectors for [Redis][Redis].  

The initial release provides the interface and implementation supporting the (~) full set of current Redis commands using synchrnous call semantics.  (Pipelines and asychronous goodness using the goroutines and channels is next.)

Hope to add rigorous tests as soon as I have a better understanding of the Go language tools.  Same applies to the makefile.  
Also am not sure regarding the efficiency of the implementation (for the obvious reasons), but definitely a goal is to make this a high performance connector.

## structure

The code is consolidated into a single 'redis' package and various elements of it are usable independently (for example if you wish to roll your own API but want to use the raw bytes protocol handling aspects).

# Compliance

Both Go and Redis are dynamic projects and present a challenge in fully covering the possible combinations in the wild.  Given the release of Go 1, this project will focus on Go 1 based Redis compatibility; we'll deal with the far off prospect of renewed major weekly changes in Go if and when that arises.   

Current status is compatible with Redis 2.4.n (2.4.9 tested)  and Go 1.  Redis feature set is not fully covered and is WIP.

(Always refer to compliance_note.txt for current (accurate) status for specific branches.)
 

# Getting started:

Get, build, and startup [Redis][Redis]:

	make
	chmod +x redis-server
	./redis-server redis.conf

Confirm:

    alphazero[13]:entrevous$ telnet localhost 6379
    Trying ::1...
    telnet: connect to address ::1: Connection refused
    Trying fe80::1...
    telnet: connect to address fe80::1: Connection refused
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    
    PING
    +PONG
    
    INFO
    $282
    redis_version:1.07
    arch_bits:32
    uptime_in_seconds:72
    uptime_in_days:0
    connected_clients:1
    connected_slaves:0
    used_memory:295232
    changes_since_last_save:1
    bgsave_in_progress:0
    last_save_time:1259873372
    total_connections_received:4
    total_commands_processed:5
    role:master
    
    QUIT
    Connection closed by foreign host.
    alphazero[14]:entrevous$ 


## get, and install, Go-Redis

Go-Redis is built using the Go tool. The tool assumes the code will be in a folder $GOPATH/src/redis .  


	cd $GOPATH/src
	git clone git://github.com/alphazero/Go-Redis.git redis
	cd redis
	go install

Confirm the install has created redis.a in your $GOPATH/pkg/<arch> folder:

	ls -l $GOPATH/pkg/"$GOOS"_"$GOARCH"/redis.a

e.g. on my Mac OS X (64b) 

	ls -l <my-gopath>/pkg/darwin_amd64

## run the benchmarks
	
Basic benchmarks are in ~/bench.  Use Go tool (go run <bench>) to run the individual bench apps.

	cd bench
	# run the asyncbench.go 
	go run asyncbench.go

## examples

[Ciao.go][ciao] is a sort of hello world for redis and should get you started for the barebones necessities of getting a client and issuing commands.

	cd examples
	go run ciao.go

[Go]: http://golang.org/
[Redis]: http://github.com/antirez/redis
[ciao]: http://github.com/alphazero/Go-Redis/blob/master/examples/ciao.go
