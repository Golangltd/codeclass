package main

import (
	"fmt"
	"github.com/gamexg/proxyclient"
	"time"
)

func main() {
	p, err := proxyclient.NewProxyClient("http://127.0.0.1:7777")
	if err != nil {
		panic(err)
	}

	t := time.Now()
	c, err := p.Dial("tcp", "www.google.com:80")
	if err != nil {
		panic(err)
	}
	fmt.Println("连接耗时：", time.Now().Sub(t))

	if _, err := c.Write([]byte("GET / HTTP/1.0\r\nHOST:www.google.com\r\n\r\n")); err != nil {
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
