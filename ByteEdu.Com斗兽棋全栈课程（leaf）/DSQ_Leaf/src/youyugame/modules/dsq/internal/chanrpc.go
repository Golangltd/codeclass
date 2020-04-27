package internal

func init() {
	skeleton.RegisterChanRPC("dsq_NewAgent", rpcNewAgent)
	skeleton.RegisterChanRPC("dsq_CloseAgent", rpcCloseAgent)
}

func rpcNewAgent(args []interface{}) {
	// a := args[0].(gate.Agent)
	// _ = a
}

func rpcCloseAgent(args []interface{}) {
	// a := args[0].(gate.Agent)
	// _ = a
}
