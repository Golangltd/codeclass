package DSQ_ST

import (
	"DSQ_server/Player"
)

// 座位信息
type SeatDataST struct {
	SeatID     int // 默认的 0,1
	PlayerData *Player_DSQ.PlayerData
	LeftTime   int
	// 不需要发送初始化棋盘数据，
}

/*
   匹配成功后的房间信息
   1. roomid  例如：从1000开始
   2. 玩家的数据
   3. 牌桌的数据
*/
type RoomData struct {
	RoomID   int            // 房间if=d
	RoomName string         // 非必要的
	SeatData [2]*SeatDataST // 座位信息,默认规则 0号位置默认是 红方，1号位置默认是 蓝方
	TurnID   int            // 轮到谁，防止：玩家多操作
}
