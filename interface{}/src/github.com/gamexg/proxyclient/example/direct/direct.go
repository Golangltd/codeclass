package main

import (
	"fmt"
	"github.com/gamexg/proxyclient"
	"time"
)

func main() {
	// 启用 http 协议分包(将 http 协议拆分到多个包)
	// 建立连接后第一次发送数据等待 5000 毫秒
	p, err := proxyclient.NewProxyClient("direct://0.0.0.0:0000?SplitHttp=true&sleep=5000")
	if err != nil {
		panic(err)
	}

	t := time.Now()
	c, err := p.Dial("tcp", "www.baidu.com:80")
	if err != nil {
		panic(err)
	}
	fmt.Println("连接耗时：", time.Now().Sub(t))

	if _, err := c.Write([]byte("GET / HTTP/1.0\r\nHOST:www.baidu.com\r\n\r\n")); err != nil {
		panic(err)
	}

	b := make([]byte, 2048)

	if n, err := c.Read(b); err != nil {
		panic(err)
	} else {
		fmt.Print(string(b[:n]))
	}

	c.Close()

}
