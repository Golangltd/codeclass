package main

import (
	"fmt"

	"LollipopGo/LollipopGo/util"

	"Proto"
	"Proto/Proto2"
	"encoding/json"

	"code.google.com/p/go.net/websocket"
)

// 匹配服务器
// 发消息给服务器
// 否则就一直等待匹配
func initMatch(conn *websocket.Conn) {
	// 1 组装
	data := &Proto2.C2S_PlayerLogin{
		Protocol:  Proto.GameData_Proto,
		Protocol2: Proto2.C2S_PlayerLoginProto2,
		Code:      util.CreateTime(), // 随机生产的数据，时间戳
	}
	// 2 发送数据到服务器
	PlayerSendToServer(conn, data)

	return
}

// 公用的send函数
func PlayerSendToServer(conn *websocket.Conn, data interface{}) {

	// 2 结构体转换成json数据
	jsons, err := json.Marshal(data)
	if err != nil {
		fmt.Println("err:", err.Error())
		return
	}

	errq := websocket.Message.Send(conn, jsons)
	if errq != nil {
		fmt.Println(errq)
	}
	return
}
