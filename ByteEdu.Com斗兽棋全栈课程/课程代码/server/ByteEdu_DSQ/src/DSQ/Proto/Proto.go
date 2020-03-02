package Proto

import (
	"Player"
)

//说明：
//1 服务器单组，只有DSQ Server (登录服，DB服，大厅服)
//2 所有的消息，目前设计：走DSQ的子协议

// 主协议  G_GameDSQ_Proto == 10    斗兽棋的主协议
const (
	GameDSQINIT             = iota // GameDSQINIT == 0             初始化子协议
	C2DSQ_PlayerLogin_Proto        // C2DSQ_PlayerLogin_Proto == 1 玩家登录的协议
	DSQ2C_PlayerLogin_Proto        // DSQ2C_PlayerLogin_Proto == 2
)

//------------------------------------------------------------------------------
// DSQ2C_PlayerLogin_Proto
type DSQ2C_PlayerLogin struct {
	Protocol   int
	Protocol2  int
	PlayerData *player.PlayerST // player 玩家的结构信息  （UID，headurl(id),name ,LV ,）
}

// C2DSQ_PlayerLogin_Proto
type C2DSQ_PlayerLogin struct {
	Protocol  int
	Protocol2 int
	DeviceID  string // 设备ID
}

//------------------------------------------------------------------------------
