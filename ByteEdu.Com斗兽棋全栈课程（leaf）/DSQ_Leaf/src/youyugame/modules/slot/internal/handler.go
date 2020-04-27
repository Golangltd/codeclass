package internal

import (
	_ "fmt"
	"reflect"
	_ "youyugame/common/uitl"
	_ "youyugame/modules/MsgCenter/msg"
)

func init() {
	//	handler(&MsgCenter_msg.UserEntryGameReq{}, UserEntryGameReq) // 玩家进入游戏
	//	handler(&MsgCenter_msg.UserYaZhuReq{}, UserEntryGameReq)     // 玩家开始游戏
}

func handler(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

//func UserEntryGameReq(args []interface{}) {
//	// 解析数据
//	m := args[0].(*MsgCenter_msg.UserEntryGameReq)
//	fmt.Println("m:", m.UserID)
//	// 去数据库模块验证
//	// call0/1/n  是属于同步调用
//	// asynCall， 提供回调函数
//	// Go模式是不需要等待结果，立即返回
//	// 发送数据到真正的数据处理
//	SyncSendOtherModules("db_NewAgent", args)
//	// ChanRPC.Call1("db_NewAgent", args)
//	// 公用发送数据
//	uitl_yy.PlayerSendMessage(args, &MsgCenter_msg.UserEntryGameResp{
//		Bsucc: true,
//	})
//	return
//}

//// 开始游戏,玩家点击开始
//func UserYaZhuReq(args []interface{}) {
//	// m := args[0].(*msg.UserYaZhuReq)
//	return
//}
