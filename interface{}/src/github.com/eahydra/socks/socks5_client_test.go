package socks

import (
	"net"
	"net/http"
	"net/http/httputil"
	"testing"
)

const (
	TestSocks5ServerAddress = "127.0.0.1:8001"
	TestSocks5TargetDomain  = "www.baidu.com"
)

func TestSocks5Client(t *testing.T) {
	listener, err := net.Listen("tcp", TestSocks5ServerAddress)
	if err != nil {
		t.Fatal(err)
	}
	defer listener.Close()

	go func() {
		server, err := NewSocks5Server(Direct)
		if err != nil {
			t.Fatal(err)
		}
		server.Serve(listener)
	}()

	client, err := NewSocks5Client("tcp", TestSocks5ServerAddress, "", "", Direct)
	if err != nil {
		t.Fatal(err)
	}
	conn, err := client.Dial("tcp", TestSocks5TargetDomain+":80")
	if err != nil {
		t.Fatal(err)
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
	t.Log("socks5 HTTP Get:", string(data))

}
