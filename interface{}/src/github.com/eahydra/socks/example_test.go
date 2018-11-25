package socks

import (
	"net"
	"net/http"
	"net/http/httputil"
)

func ExampleSocks5Client() {
	user := ""
	password := ""
	client, err := NewSocks5Client("tcp", "127.0.0.1:1080", user, password, Direct)
	if err != nil {
		return
	}
	conn, err := client.Dial("tcp", "www.google.com:80")
	if err != nil {
		return
	}
	httpClient := httputil.NewClientConn(conn, nil)
	defer httpClient.Close()

	request, err := http.NewRequest("GET", "/", nil)
	if err != nil {
		return
	}
	resp, err := httpClient.Do(request)
	if err != nil {
		return
	}
	dump, err := httputil.DumpResponse(resp, true)
	if err != nil {
		return
	}
	println(string(dump))
	return
}

func ExampleSocks5Server() {
	listener, err := net.Listen("tcp", ":1080")
	if err != nil {
		return
	}
	defer listener.Close()

	if server, err := NewSocks5Server(Direct); err == nil {
		server.Serve(listener)
	}
}

func ExampleSocks4Client() {
	user := ""
	client, err := NewSocks4Client("tcp", "127.0.0.1:1080", user, Direct)
	if err != nil {
		return
	}
	addrs, err := net.LookupHost("www.google.com")
	if err != nil {
		return
	}
	if len(addrs) == 0 {
		return
	}
	conn, err := client.Dial("tcp", addrs[0]+":80")
	if err != nil {
		return
	}
	httpClient := httputil.NewClientConn(conn, nil)
	defer httpClient.Close()

	request, err := http.NewRequest("GET", "/", nil)
	if err != nil {
		return
	}
	resp, err := httpClient.Do(request)
	if err != nil {
		return
	}
	dump, err := httputil.DumpResponse(resp, true)
	if err != nil {
		return
	}
	println(string(dump))
	return
}

func ExampleSocks4Server() {
	listener, err := net.Listen("tcp", ":1080")
	if err != nil {
		return
	}
	defer listener.Close()

	if server, err := NewSocks4Server(Direct); err == nil {
		server.Serve(listener)
	}
}
