package main

import (
	"fmt"

	"code.google.com/p/go.net/websocket"
)

func wwwGolangLtd(ws *websocket.Conn) {
	fmt.Println("Golang语言社区 欢迎您！", ws)
	// data = json{}
	data := ws.Request().URL.Query().Get("data")
	fmt.Println("data:", data)

	// 网络信息
	NetDataConntmp := &NetDataConn{
		Connection: ws,
		StrMd5:     "",
	}
	// 指针接受者  处理消息
	NetDataConntmp.PullFromClient()
}
