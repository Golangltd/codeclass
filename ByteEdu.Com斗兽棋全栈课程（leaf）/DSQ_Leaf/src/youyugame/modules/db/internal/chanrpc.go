package internal

import (
	"github.com/name5566/leaf/gate"
)

func init() {
	skeleton.RegisterChanRPC("db_NewAgent", rpcNewAgent)
	skeleton.RegisterChanRPC("db_CloseAgent", rpcCloseAgent)
}

// 建立链接
func rpcNewAgent(args []interface{}) {
	a := args[0].(gate.Agent)
	_ = a
}

// 清除链接
func rpcCloseAgent(args []interface{}) {
	a := args[0].(gate.Agent)
	_ = a
}
