package internal

import (
	"LollipopGo/msg/protocolfile"
	"reflect"

	//	"github.com/go-redis/redis" // 内存数据库--用于测试

	"github.com/LollipopGo/lollipopgo/gate"
	"github.com/LollipopGo/lollipopgo/log"
)

func handleMsg(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

func init() {
	// 向当前模块（login 模块）注册 Protocol.UserLogin 消息的消息处理函数 handleTest
	handleMsg(&Protocol.UserLogin{}, handleTest)
}

// 消息处理
func handleTest(args []interface{}) {
	// 收到的 Test 消息
	m := args[0].(*Protocol.UserLogin)
	// 消息的发送者
	a := args[1].(gate.Agent)
	// 1 查询数据库--判断用户是不是合法
	// 2 如果数据库返回查询正确--保存到缓存或者内存

	// 输出收到的消息的内容
	log.Debug("Test login %v", m.LoginName)
	log.Debug("---------", Protocol.UserLogin{
		LoginName: "client",
	})

	dd := &Protocol.UserLogin{
		LoginName: "client",
		LoginPW:   "golang.ltd",
	}
	// a.PlaySendMessage(dd)
	a.WriteMsg(dd)
	//gamedata.Init1()
}
