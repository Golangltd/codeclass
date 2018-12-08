package proxyclient

import (
	"bytes"
	"encoding/binary"
	"io"
	"net"
	"testing"
	"time"
)

func testSocks5ProixyServer(t *testing.T, proxyAddr string, usernameAndPassword []byte, attypAddr []byte, port uint16, ci chan int) {
	b := make([]byte, 30)
	l, err := net.Listen("tcp", proxyAddr)
	if err != nil {
		t.Fatalf("错误,%v", err)
	}

	ci <- 1

	c, err := l.Accept()
	if err != nil {
		t.Fatal(err)
	}

	if len(usernameAndPassword) == 0 {
		if n, err := c.Read(b); err != nil || bytes.Equal(b[:n], []byte{0x05, 0x01, 0x00}) != true {
			t.Fatal("鉴定请求错误：", err)
		}

		if _, err := c.Write([]byte{0x05, 0x00}); err != nil {
			t.Fatalf("回应鉴定错误：%v", err)
		}
	} else {
		if n, err := c.Read(b); err != nil || bytes.Equal(b[:n], []byte{0x05, 0x01, 0x02}) != true {
			t.Fatalf("鉴定请求错误：%v", err)
		}

		if _, err := c.Write([]byte{0x05, 0x02}); err != nil {
			t.Fatalf("回应鉴定错误：%v", err)
		}

		if n, err := c.Read(b); err != nil || b[0] != 0x01 || bytes.Equal(b[1:n], usernameAndPassword) != true {
			t.Fatalf("用户名密码错误：%v", err)
		}

		if _, err := c.Write([]byte{0x01, 0x00}); err != nil {
			t.Fatalf("回应登陆错误：%v", err)
		}
	}

	// 构建应该受到的请求内容
	br := make([]byte, 5+len(attypAddr))
	n := copy(br, []byte{0x05, 0x01, 0x00})
	n = copy(br[n:], attypAddr)
	binary.BigEndian.PutUint16(br[n+3:], port)

	// 接收命令请求
	if n, err := c.Read(b); err != nil || bytes.Equal(b[:n], br) != true {
		t.Fatalf("请求命令错误：%v,%v!=%v", err, br, b[:n])
	}

	// 发出回应
	if _, err := c.Write([]byte{0x05, 0x00, 0x00, 0x01, 0x1, 0x2, 0x3, 0x4, 0x80, 0x80}); err != nil {
		t.Fatalf("请求回应错误：%v", err)
	}

	if n, err := c.Read(b); err != nil || bytes.Equal(b[:n], B1) != true {
		t.Fatalf("B1不正确。err=%v，B1=%v,b=%v", err, B1, b[:n])
	}

	// 发出B2
	if _, err := c.Write(B2); err != nil {
		t.Fatalf("B2 发送错误：%v", err)
	}

	if v, ok := c.(TCPConn); ok != true {
		t.Fatalf("类型不匹配错误。")
	} else {
		v.SetLinger(5)
	}
	c.Close()

}

func testSocks5ProxyClient(t *testing.T, proxyAddr string, addr string) {
	b := make([]byte, 30)
	p, err := NewProxyClient(proxyAddr)
	if err != nil {
		t.Fatal("启动代理错误:", err)
	}

	c, err := p.DialTimeout("tcp", addr, 5*time.Second)
	if err != nil {
		t.Fatal("通过代理建立连接错误：", err, "proxyAddr:", proxyAddr, "addr:", addr)
	}

	// 发出B1
	if _, err := c.Write(B1); err != nil {
		t.Fatal("B1 发送错误：", err)
	}

	//接收B2
	if n, err := c.Read(b); err != nil || bytes.Equal(b[:n], B2) != true {
		t.Fatalf("B2不正确。err=%v，B1=%v,b=%v", err, B2, b[:n])
	}

	if _, err := c.Read(b); err != io.EOF {
		t.Fatal("读EOF错误。err=", err)
	}
}

func TestSocksProxy(t *testing.T) {
	ci := make(chan int)
	b := make([]byte, 0, 30)

	// 测试域名
	addr := "www.163.com"

	b = append(b, 0x03, byte(len(addr)))
	b = append(b, []byte(addr)...)

	go testSocks5ProixyServer(t, "127.0.0.1:13337", nil, b, 80, ci)
	<-ci
	testSocks5ProxyClient(t, "socks5://127.0.0.1:13337", "www.163.com:80")

	// 测试 ipv4
	addr = "1.2.3.4"
	b = b[0:0]
	b = append(b, 0x01)
	b = append(b, []byte(net.ParseIP(addr).To4())...)

	go testSocks5ProixyServer(t, "127.0.0.1:13338", nil, b, 80, ci)
	<-ci
	testSocks5ProxyClient(t, "socks5://127.0.0.1:13338", "1.2.3.4:80")

	// 测试 ipv6
	addr = "1:2:3:4::5:6"
	b = b[0:0]
	b = append(b, 0x04)
	b = append(b, []byte(net.ParseIP(addr))...)

	go testSocks5ProixyServer(t, "127.0.0.1:13339", nil, b, 80, ci)
	<-ci
	testSocks5ProxyClient(t, "socks5://127.0.0.1:13339", "[1:2:3:4::5:6]:80")
}

func TestSocksProxyA(t *testing.T) {
	ci := make(chan int)
	b := make([]byte, 0, 30)

	userAndPass := []byte{0x04, 'u', 's', 'e', 'r', 0x04, 'p', 'a', 's', 's'}

	// 测试域名
	addr := "www.163.com"

	b = append(b, 0x03, byte(len(addr)))
	b = append(b, []byte(addr)...)

	go testSocks5ProixyServer(t, "127.0.0.1:13347", userAndPass, b, 80, ci)
	<-ci
	testSocks5ProxyClient(t, "socks5://user:pass@127.0.0.1:13347", "www.163.com:80")

	// 测试 ipv4
	addr = "1.2.3.4"
	b = b[0:0]
	b = append(b, 0x01)
	b = append(b, []byte(net.ParseIP(addr).To4())...)

	go testSocks5ProixyServer(t, "127.0.0.1:13348", userAndPass, b, 80, ci)
	<-ci
	testSocks5ProxyClient(t, "socks5://user:pass@127.0.0.1:13348", "1.2.3.4:80")

	// 测试 ipv6
	addr = "1:2:3:4::5:6"
	b = b[0:0]
	b = append(b, 0x04)
	b = append(b, []byte(net.ParseIP(addr))...)

	go testSocks5ProixyServer(t, "127.0.0.1:13349", userAndPass, b, 80, ci)
	<-ci
	testSocks5ProxyClient(t, "socks5://user:pass@127.0.0.1:13349", "[1:2:3:4::5:6]:80")
}
