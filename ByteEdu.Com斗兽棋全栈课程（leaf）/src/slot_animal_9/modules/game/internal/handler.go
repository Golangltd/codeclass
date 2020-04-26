package internal

func init() {
	// 向当前模块（game 模块）注册 Hello 消息的消息处理函数 handleHello
	handler(&msg.Hello{}, handleHello)
	handler(&msg.UserEntryGameReq{}, UserEntryGameReq) // 玩家进入游戏
	handler(&msg.UserYaZhuReq{}, UserEntryGameReq)     // 玩家开始游戏
}

// 底层处理
func handler(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

// 消息处理
func handleHello(args []interface{}) {
	// 收到的 Hello 消息
	m := args[0].(*msg.Hello)
	// 消息的发送者
	a := args[1].(gate.Agent)

	// 输出收到的消息的内容
	log.Debug("hello %v", m.Name)

	// 给发送者回应一个 Hello 消息
	a.WriteMsg(&msg.Hello{
		Name: "client",
	})
}

// 玩家进入游戏
// 保存玩家数据到，内存
func UserEntryGameReq(args []interface{}) {
	// 解析数据
	// m := args[0].(*msg.UserEntryGameReq)
	return
}

// 开始游戏,玩家点击开始
func UserYaZhuReq(args []interface{}) {
	// m := args[0].(*msg.UserYaZhuReq)
	// 返回数据给前端
	return
}
