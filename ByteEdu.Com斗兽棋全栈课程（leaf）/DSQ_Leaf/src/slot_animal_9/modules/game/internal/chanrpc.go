package internal

import (
	"github.com/name5566/leaf/gate"
)

// 定义个全局变量

func init() {
	// 注册事件
	skeleton.RegisterChanRPC("NewAgent", rpcNewAgent)
	skeleton.RegisterChanRPC("CloseAgent", rpcCloseAgent)
	// 注册消息
	// handler(&msg.AuthReq{}, EntryGame)
}

func rpcNewAgent(args []interface{}) {
	a := args[0].(gate.Agent)
	_ = a
}

func rpcCloseAgent(args []interface{}) {
	a := args[0].(gate.Agent)
	_ = a
}
