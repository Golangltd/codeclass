package gate

import (
	"GameServer/modules/game"
	"GameServer/msg"
)

func init() {
	msg.Processor.SetRouter(&msg.RoomCreate{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.RoomEnter{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.ClientClose{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.RoomExit{}, game.ChanRPC)
	//打牌
	msg.Processor.SetRouter(&msg.ChuPai{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.Gang{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.Peng{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.AnGang{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.ChiPai{}, game.ChanRPC)
	msg.Processor.SetRouter(&msg.HuPai{}, game.ChanRPC)
}
