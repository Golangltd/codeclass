package socks

import (
	"net"
	"net/http"
	"net/http/httputil"
	"testing"
)

const (
	TestSocks4ServerAddr   = "127.0.0.1:8000"
	TestSocks4TargetDomain = "www.baidu.com"
)

func TestSocks4Client(t *testing.T) {
	listener, err := net.Listen("tcp", TestSocks4ServerAddr)
	if err != nil {
		t.Fatal(err)
	}
	defer listener.Close()
	go func() {
		server, err := NewSocks4Server(Direct)
		if err != nil {
			t.Fatal(err)
		}
		server.Serve(listener)
	}()

	client, err := NewSocks4Client("tcp", TestSocks4ServerAddr, "", Direct)
	if err != nil {
		t.Fatal(err)
	}

	addrs, err := net.LookupHost(TestSocks4TargetDomain)
	if err != nil {
		t.Fatal(err)
	}
	if len(addrs) == 0 {
		t.Fatal("net.LookupHost with " + TestSocks4TargetDomain + " result invalid")
	}

	conn, err := client.Dial("tcp", addrs[0]+":80")
	if err != nil {
		t.Fatal("socks4 client.Dial failed, err: " + err.Error())
	}
	t.Log("client.Dial succeeded")
	httpClient := httputil.NewClientConn(conn, nil)
	if err != nil {
		conn.Close()
		t.Fatal(err)
	}
	defer httpClient.Close()

	request, err := http.NewRequest("GET", "/", nil)
	if err != nil {
		t.Fatal(err)
	}
	resp, err := httpClient.Do(request)
	if err != nil {
		t.Fatal(err)
	}
	data, err := httputil.DumpResponse(resp, true)
	if err != nil {
		t.Fatal(err)
	}
	t.Log("socks4 HTTP Get:", string(data))
}
