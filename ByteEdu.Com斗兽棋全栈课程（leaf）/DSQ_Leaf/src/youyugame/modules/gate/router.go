package gate

import (
	"youyugame/modules/MsgCenter/msg"
	_ "youyugame/modules/db"
	"youyugame/modules/dsq"
)

/*
  所有消息的入口：
  1. 不同模块间的转发等
  2. 消息的定义
*/
func init() {
	MsgCenter_msg.Processor.SetRouter(&MsgCenter_msg.C2G_Player_PiPeiGame{}, dsq.ChanRPC)
}
