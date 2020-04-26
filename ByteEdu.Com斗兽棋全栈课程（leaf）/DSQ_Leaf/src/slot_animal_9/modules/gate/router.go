package gate

// 消息处理流程
func init() {
	// 执行不同的模块开发
	msg.Processor.SetRouter(&msg.Hello{}, game.ChanRPC)
}
