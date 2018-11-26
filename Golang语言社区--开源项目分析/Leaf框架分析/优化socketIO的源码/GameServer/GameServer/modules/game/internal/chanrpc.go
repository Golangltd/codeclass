package internal

import (
	"GameServer/leaf/gate"
)

func init() {
	skeleton.RegisterChanRPC("NewAgent", rpcNewAgent)
	skeleton.RegisterChanRPC("CloseAgent", rpcCloseAgent)
}

func rpcNewAgent(args []interface{}) {
	a := args[0].(gate.Agent)
	a.Emit("open", map[string]bool{"result": true})

}

func rpcCloseAgent(args []interface{}) {
	a := args[0].(gate.Agent)
	a.Emit("close", map[string]bool{"result": true})
}
