package proxyclient

import (
	"bytes"
	"io"
	"net"
	"testing"
)

const (
	CONNECT = "CONNECT"
)

// 伪装成为一个代理服务器。
func testHTTPProixyServer(t *testing.T, proxyAddr string, rAddr string, ci chan int) {

	l, err := net.Listen("tcp", proxyAddr)
	if err != nil {
		t.Fatalf("监听错误:%v", err)
	}

	ci <- 1
	c, err := l.Accept()
	if err != nil {
		t.Fatalf("接受连接错误:%v", err)
	}

	b := make([]byte, 1024)

	if _, err := c.Read(b); err != nil {
		t.Fatalf("读错误：%v", err)
	} /*else {
		b = b[:n]
		t.Log(string(b))
	}*/

	connect := CONNECT + " " + rAddr

	if bytes.Equal(b[:len(connect)], []byte(connect)) != true {
		t.Fatalf("命令不匹配！")
	}

	if bytes.Index(b, []byte("User-Agent")) < 0 {
		t.Fatalf("请求中未发现 User-Agent")
	}

	if _, err := c.Write([]byte("HTTP/1.0 200 ok\r\n00000:aaaaa\r\n\r\n")); err != nil {
		t.Fatalf("写数据错误")
	}

	if _, err := c.Read(b[:1024]); err != nil {
		t.Fatalf("读错误：%v", err)
	} /*else {
		b = b[:n]
		print(b)
	}*/

	if _, err := c.Write([]byte("HTTP/1.0 200 ok\r\nHead1:11111\r\n\r\nHello Word!")); err != nil {
		t.Fatalf("写数据错误")
	}

	c.Close()

}

func TestHttpProxy(t *testing.T) {
	ci := make(chan int)
	go testHTTPProixyServer(t, "127.0.0.1:1331", "www.google.com:80", ci)
	<-ci

	p, err := NewProxyClient("http://127.0.0.1:1331?standardheader=True")
	if err != nil {
		t.Fatalf("连接代理服务器错误：%v", err)
	}

	c, err := p.Dial("tcp", "www.google.com:80")
	if err != nil {
		t.Fatalf("通过代理服务器连接目标网站失败：%v", err)
	}

	if _, err := c.Write([]byte("GET / HTTP/1.0\r\nHOST:www.google.com\r\n\r\n")); err != nil {
		t.Fatalf("请求发送错误：%v", err)
	}

	b := make([]byte, 1024)
	if n, err := c.Read(b); err != nil {
		t.Fatalf("响应读取错误：%v", err)
	} else {
		b = b[:n]
	}

	if bytes.Equal(b, []byte("HTTP/1.0 200 ok\r\nHead1:11111\r\n\r\nHello Word!")) != true {
		t.Fatalf("返回内容不匹配：%v", string(b))
	}

	if _, err := c.Read(b[:1024]); err != io.EOF {
		t.Fatalf("非预期的结尾：%v", err)
	}

}
