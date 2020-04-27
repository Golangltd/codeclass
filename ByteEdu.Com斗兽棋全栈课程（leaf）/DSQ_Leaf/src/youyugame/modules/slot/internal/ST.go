package internal

import (
	"sync"
)

//  房间结构
type SlotsRoom struct {
	RoomId       string
	SlotsType    int32 // 线数
	PlayerNum    int
	MaxPlayerNum int
	RoomPlayers  map[int32]*SlotsPlayer
}

// 房间指针管理器
type SlotsRoomST struct {
	SlotRoomData map[string]*SlotsRoom
	SlotsLock    *sync.RWMutex
}

// slot 玩家
type SlotsPlayer struct {
}

// 所有线的管理器
type SlotAllLineTriple struct {
}

// 房间管理器
var GSlotRoomPtr *SlotsRoomST

//------------------------------------------------------------------------------
// 获取指针管理器
func NewGSlotRoomPtr() *SlotsRoomST {
	return &SlotsRoomST{
		SlotRoomData: make(map[string]*SlotsRoom),
		SlotsLock:    new(sync.RWMutex),
	}
}

// -----------------------------------------------------------------------------
