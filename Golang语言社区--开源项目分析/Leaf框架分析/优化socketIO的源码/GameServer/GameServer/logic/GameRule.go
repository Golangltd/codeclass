package logic

type GameRule interface {
	StartLogicThinker()
	Start()
	OnMessage(message RoomMessage)
	SetRoom(room *Room)
}
