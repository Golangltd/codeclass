package internal

import (
	"fmt"
	"reflect"
	"youyugame/common/uitl"
	"youyugame/modules/MsgCenter/msg"
)

func init() {
	handler(&MsgCenter_msg.C2G_Player_PiPeiGame{}, C2G_Player_PiPeiGame) // 玩家进入游戏
}

func handler(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

/*
	客户端发送匹配的请求
	Protocol  int
	Protocol2 int
	OpenID    string // 玩家的唯一ID
*/
func C2G_Player_PiPeiGame(args []interface{}) {
	m := args[0].(*MsgCenter_msg.C2G_Player_PiPeiGame)
	fmt.Println(m.OpenID)

	/* Broadcast_PiPeiGame_Proto == 4   服务器返回 是否成功
	type Broadcast_PiPeiGame struct {
		Protocol  int
		Protocol2 int
		Bsucc     bool                 // true ：匹配成功， false：匹配失败
		RoomData  *playerdata.RoomData // 房间信息，房间的ID，匹配成功的玩家的数据，当前的牌桌上的数据（双方的信息，倒计时等）
	}
	*/

	data := &MsgCenter_msg.Broadcast_PiPeiGame{
		Protocol:  10,
		Protocol2: 4,
		Bsucc:     true,
		RoomData:  nil,
	}
	uitl_yy.PlayerSendMessage(args, data)
	return
}
