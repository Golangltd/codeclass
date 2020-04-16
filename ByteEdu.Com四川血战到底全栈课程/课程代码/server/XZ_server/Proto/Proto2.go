package Proto_DSQGame

import (
	"XZ_server/Player"
	"XZ_server/ST"
)

// 主协议 10号
const (
	DSQGameINIT                    = iota // 初始化操作
	C2G_Player_Login_Proto                // C2G_Player_Login_Proto == 1  客户端到游戏服务器验证
	G2C_Player_Login_Proto                // G2C_Player_Login_Proto == 2  服务器返回 是否成功
	C2G_Player_PiPeiGame_Proto            // C2G_Player_PiPeiGame_Proto == 3 客户端到游戏服务器   玩家匹配
	Broadcast_PiPeiGame_Proto             // Broadcast_PiPeiGame_Proto == 4   服务器返回 是否成功,广播协议
	C2G_Player_FanPai_Proto               // C2G_Player_FanPai_Proto == 5 客户端到游戏服务器   玩家翻牌
	Broadcast_Player_FanPai_Proto         // Broadcast_Player_FanPai_Proto == 6   服务器返回 是否成功,翻盘数据
	C2G_Player_XingZou_Proto              // C2G_Player_XingZou_Proto == 7 客户端到游戏服务器   棋子“行走”,整个我们斗兽棋游戏最复杂的逻辑处理
	Broadcast_Player_XingZou_Proto        // Broadcast_Player_XingZou_Proto == 8   服务器返回
	C2G_Player_RenShu_Proto               // C2G_Player_RenShu_Proto == 9 客户端到游戏服务器  发送认输
	Broadcast_GameOver_Proto              // Broadcast_GameOver_Proto == 10   服务器返回 游戏结束的数据
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
// C2G_Player_RenShu_Proto == 9 客户端到游戏服务器  发送认输
type C2G_Player_RenShu struct {
	Protocol  int
	Protocol2 int
	OpenID    string
}

/*
// 结算的数据块
type GameOver struct {
	PlayerWin  *Player_DSQ.PlayerData // 胜利的玩家
	PlayerFail *Player_DSQ.PlayerData // 失败的玩家
	GameName   string                 // 游戏的名字
	WinSeatID  int                    // 胜利人的位置信息，0或者1的位置信息
	Score      int                    // 胜利人获取的分数信息
	// ... ... 根据自己游戏类型设计结算的数据块
}
*/
// Broadcast_GameOver_Proto == 10   服务器返回
type Broadcast_GameOver struct {
	Protocol  int
	Protocol2 int
	GameOver  *DSQ_ST.GameOver // 计算的数据信息
}

// -----------------------------------------------------------------------------
// C2G_Player_XingZou_Proto == 7 客户端到游戏服务器   棋子“行走”
type C2G_Player_XingZou struct {
	Protocol  int
	Protocol2 int
	OpenID    string
	OldPos    string
	NewPos    string
}

// Broadcast_Player_XingZou_Proto == 8   服务器返回
type Broadcast_Player_XingZou struct {
	Protocol  int
	Protocol2 int
	OpenID    string
	OldPos    string
	NewPos    string
}

// -----------------------------------------------------------------------------
// C2G_Player_FanPai_Proto == 5 客户端到游戏服务器   玩家翻牌
type C2G_Player_FanPai struct {
	Protocol  int
	Protocol2 int
	OpenID    string // 告诉服务器，谁翻了牌
	//SeatID    string // 和openid字段 同样的效果 0 1
	StrPos string // 位置信息
	RoomID string // 告诉服务器，我是再那个房间下；非必须的？，通过玩家的唯一ID去获取到房间的信息
}

/*
	[
	  [6 8 11 5]
	  [12 10 15 3]
	  [16 1 2 14]
	  [4 7 13 9]
	]
*/
// Broadcast_Player_FanPai_Proto == 6   服务器返回 是否成功,翻盘数据
type Broadcast_Player_FanPai struct {
	Protocol  int
	Protocol2 int
	IChess    int    // 棋盘的数据 1-16的枚举值
	OpenID    string // 告诉服务器，谁翻了牌
	StrPos    string // 位置信息
	SeatID    int
}

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
