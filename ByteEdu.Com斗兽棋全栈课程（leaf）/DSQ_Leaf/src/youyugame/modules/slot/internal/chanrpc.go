package internal

func init() {
	skeleton.RegisterChanRPC("slot_NewAgent", rpcNewAgent)
	skeleton.RegisterChanRPC("slot_CloseAgent", rpcCloseAgent)
}

func rpcNewAgent(args []interface{}) {
	// a := args[0].(gate.Agent)
	// _ = a
}

func rpcCloseAgent(args []interface{}) {
	// a := args[0].(gate.Agent)
	// _ = a
}
