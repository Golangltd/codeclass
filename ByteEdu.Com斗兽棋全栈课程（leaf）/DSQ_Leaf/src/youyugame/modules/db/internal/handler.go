package internal

import (
	_ "fmt"
	"reflect"
	_ "youyugame/modules/MsgCenter/msg"

	_ "github.com/name5566/leaf/gate"
)

func init() {
	//handler(&MsgCenter_msg.UserTockenReq{}, UserTockenReq)
}

func handler(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

// 玩家进入游戏验证
func UserTockenReq(args []interface{}) {
	//	// 用户处理
	//	m := args[0].(*MsgCenter_msg.UserTockenReq)
	//	a := args[1].(gate.Agent)
	//	fmt.Println("m:", m.UserID)
	//	// 发送数据
	//	a.WriteMsg(&MsgCenter_msg.UserTockenResp{
	//		Bsucc: true,
	//	})
	//	return
}
