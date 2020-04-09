package Proto_DSQGame

import (
	"DSQ_server/Player"
	"DSQ_server/ST"
)

// 主协议 10号
const (
	DSQGameINIT                = iota // 初始化操作
	C2G_Player_Login_Proto            // C2G_Player_Login_Proto == 1  客户端到游戏服务器验证
	G2C_Player_Login_Proto            // G2C_Player_Login_Proto == 2  服务器返回 是否成功
	C2G_Player_PiPeiGame_Proto        // C2G_Player_PiPeiGame_Proto == 3 客户端到游戏服务器   玩家匹配
	Broadcast_PiPeiGame_Proto         // Broadcast_PiPeiGame_Proto == 4   服务器返回 是否成功,广播协议
)

// 斗兽棋的枚举
const (
	DSQINIT  = iota // 初始化
	Elephant        // 大象  ==1
	Lion            // 狮子  ==2
	Tiger           // 老虎  ==3
	Leopard         // 豹子  ==4
	Wolf            // 狼    ==5
	Dog             // 狗    ==6
	Cat             // 猫    ==7
	Mouse           // 老鼠  ==8
)

// -----------------------------------------------------------------------------
// C2G_Player_PiPeiGame_Proto == 3 客户端到游戏服务器   玩家匹配
type C2G_Player_PiPeiGame struct {
	Protocol  int
	Protocol2 int
	OpenID    string // 玩家的唯一ID
}

// Broadcast_PiPeiGame_Proto == 4   服务器返回 是否成功
type Broadcast_PiPeiGame struct {
	Protocol  int
	Protocol2 int
	Bsucc     bool             // true ：匹配成功， false：匹配失败
	RoomData  *DSQ_ST.RoomData // 房间信息，房间的ID，匹配成功的玩家的数据，当前的牌桌上的数据（双方的信息，倒计时等）
}

// -----------------------------------------------------------------------------
// G2C_Player_Login_Proto == 2  客户端到游戏服务器验证
type G2C_Player_Login struct {
	Protocol  int
	Protocol2 int
	Bsucc     bool
	Player    *Player_DSQ.PlayerData
}

//  C2G_Player_Login_Proto == 2  服务器返回 是否成功
type C2G_Player_Login struct {
	Protocol  int
	Protocol2 int
	Tocken    string // 登录验证码
}

// -----------------------------------------------------------------------------
