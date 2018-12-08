package proxyclient

import (
	"bytes"
	ss "github.com/shadowsocks/shadowsocks-go/shadowsocks"
	"net"
	"testing"
	"time"
)

func TestSsProxy(t *testing.T) {

	// 测试域名
	testAddr := "www.test123.com:80"
	testData := []byte("dsgbhdfhgsq36jhrawdxghucn46ggetst")
	testServerAddr := "127.0.0.1:1458"
	testMethod := "aes-256-cfb"
	testPassword := "dfh67fdtjhj8esgtk87ehhdg"
	testRawAddr, err := ss.RawAddr(testAddr)
	if err != nil {
		t.Fatal(err)
	}

	// 简单模拟服务器
	cipher, err := ss.NewCipher(testMethod, testPassword)
	if err != nil {
		t.Fatal(err)
	}

	l, err := net.Listen("tcp", testServerAddr)
	if err != nil {
		t.Fatal(err)
	}
	defer l.Close()

	go func() {
		c, err := l.Accept()
		if err != nil {
			t.Fatal(err)
		}
		c.SetDeadline(time.Now().Add(5 * time.Second))

		sc := ss.NewConn(c, cipher)
		defer sc.Close()

		// 读内容并返回
		for i := 0; i < 2; i++ {
			buf := make([]byte, 1024)
			if n, err := sc.Read(buf); err != nil {
				t.Fatal("i=", i, "服务器读内容错误：", err)
			} else {
				if _, err := sc.Write(buf[:n]); err != nil {
					t.Fatal(err)
				}
			}
		}
	}()

	// 发出请求，然后解密。
	p, err := newSsProxyClient(testServerAddr, testMethod, testPassword, nil, nil)
	if err != nil {
		t.Fatal(err)
	}

	c, err := p.DialTimeout("tcp", testAddr, 1*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()

	// 比较地址是否正确
	buf := make([]byte, 1024)
	if n, err := c.Read(buf); err != nil {
		t.Fatal("读地址错误：", err)
	} else {
		if bytes.Compare(buf[:n], testRawAddr) != 0 {
			t.Fatal("地址未正确发送")
		}
	}

	// 发送测试数据，并读取比较
	if _, err := c.Write(testData); err != nil {
		t.Fatal(err)
	}

	if n, err := c.Read(buf); err != nil {
		t.Fatal(err)
	} else {
		if bytes.Compare(buf[:n], testData) != 0 {
			t.Fatal("数据未正确发送")
		}
	}
}
