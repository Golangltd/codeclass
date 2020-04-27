package MsgCenter_msg

import (
	"github.com/name5566/leaf/network/json"
	_ "github.com/name5566/leaf/network/protobuf"
)

var Processor = json.NewProcessor() // json
//var Processor = protobuf.NewProcessor()  // protobuf

/*
   所有的消息入口：
*/
func init() {
	Processor.Register(&C2G_Player_PiPeiGame{}) // 匹配协议
}
