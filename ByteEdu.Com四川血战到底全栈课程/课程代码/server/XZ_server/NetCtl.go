package main

import (
	"encoding/json"
	"glog-master"

	"code.google.com/p/go.net/websocket"
)

func (this *DSQGame) PlayerSendMessage(senddata interface{}) int {

	b, err1 := json.Marshal(senddata)
	if err1 != nil {
		glog.Error("PlayerSendMessage json.Marshal data fail ! err:", err1.Error())
		glog.Flush()
		return 1
	}
	// 去除多余发送数据
	if len(string(b)) < 100 {
		glog.Info("json.Marshal(b) :", string(b))
		data := ""
		data = "data" + "=" + string(b[0:len(b)])
		glog.Info("json.Marshal(data) :", data)
	}
	if this.Connection == nil {
		glog.Info("链接信息为空:", 3)
		return 3
	}
	glog.Flush()
	err := websocket.JSON.Send(this.Connection, b)
	if err != nil {
		glog.Error("PlayerSendMessage send data fail ! err:", err.Error())
		glog.Flush()
		return 2
	}

	return 0
}

func (this *DSQGame) PlayerSendMessagePro(senddata interface{}) int {

	b, err1 := json.Marshal(senddata)
	if err1 != nil {
		glog.Error("PlayerSendMessage json.Marshal data fail ! err:", err1.Error())
		glog.Flush()
		return 1
	}

	err := websocket.JSON.Send(this.Connection, b)
	if err != nil {
		glog.Error("PlayerSendMessage send data fail ! err:", err.Error())
		glog.Flush()
		return 2
	}

	return 0
}
