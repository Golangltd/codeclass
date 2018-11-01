package main

import (
	"fmt"

	"flag"

	"code.google.com/p/go.net/websocket"
)

/*
robot
1 模拟玩家的正常的“操作”，例如 行走 跳跃 开枪等等
2 做服务器的性能的测试，例如 并发量  内存 CPU 等等
3 压力测试

注意点：
1  模拟 ---> 多线程模拟  goroutine  --- server ！！！

首先：
1 net 网络使用websocket 进行连接
2  send  如何发送 ？？
*/
var addr = flag.String("addr", "127.0.0.1:8888", "http service address")

func main() {
	fmt.Println("Robot 客户端模拟！")
	url := "ws://" + *addr + "/GolangLtd"
	ws, err := websocket.Dial(url, "", "test://golang/")
	if err != nil {
		fmt.Println("err:", err.Error())
		return
	}
	_ = ws
}
