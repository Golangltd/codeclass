package msg

import (
	"GameServer/leaf/network/json"
	pb "GameServer/modules/CalRPC/CalCall"
)

var Processor = json.NewProcessor()

func init() {
	Processor.Register(&RoomCreate{})
	Processor.Register(&RoomEnter{})
	Processor.Register(&ClientClose{})
	Processor.Register(&RoomExit{})
	Processor.Register(&ChuPai{})
	Processor.Register(&Gang{})
	Processor.Register(&AnGang{})
	Processor.Register(&ChiPai{})
	Processor.Register(&HuPai{})
	Processor.Register(&Peng{})
}

//===============================房间消息=========================================
type RoomCreate struct {
	UID     string `json:"pid"`
	Message string `json:"message"`
	RoomID  string `json:"roomid"`
	Ext1    string `json:"ext1"`
	Ext2    string `json:"ext2"`
	Ext3    string `json:"ext3"`
	Ext4    string `json:"ext4"`
	Ext5    string `json:"ext5"`
}
type RoomEnter struct {
	UID    string `json:"pid"`
	RoomID string `json:"roomid"`
	Name   string `json:"name"`
	Sex    string `json:"sex"`
	Icon   string `json:"icon"`
	IP     string `json:"ip"`
}
type ClientClose struct {
	UID string `json:"pid"`
}
type RoomExit struct {
	UID    string `json:"pid"`
	Dir    string `json:"position"`
	RoomID string `json:"roomid"`
}
type TaskMessage struct {
	UID   int
	Event string
	Data  interface{}
}

//===============================房间指令=========================================
//出牌
type ChuPai struct {
	UID     string `json:"pid"`
	Dir     string `json:"position"`
	RoomID  string `json:"roomid"`
	Message string `json:"message"`
	Ext1    string `json:"ext1"`
}

//碰牌
type Peng struct {
	UID      string `json:"pid"`
	Position string `json:"position"`
	Message  string `json:"message"`
	RoomID   string `json:"roomid"`
}

//杠牌
type Gang struct {
	UID      string `json:"pid"`
	Position string `json:"position"`
	Message  string `json:"message"`
	RoomID   string `json:"roomid"`
	Order    string `json:"order"`
}

//暗杠
type AnGang struct {
	UID      string `json:"pid"`
	Position string `json:"position"`
	Message  string `json:"message"`
	RoomID   string `json:"roomid"`
	Order    string `json:"order"`
}

//吃牌
type ChiPai struct {
	UID      string `json:"pid"`
	Position string `json:"position"`
	Message  string `json:"message"`
	RoomID   string `json:"roomid"`
}

//胡牌
type HuPai struct {
	UID      string `json:"pid"`
	Position string `json:"position"`
	Message  string `json:"message"`
	RoomID   string `json:"roomid"`
}

//===============================RPC=============================================
//调用吃碰胡计算 request
type RpcCall struct {
	pb.MajongCal
}
