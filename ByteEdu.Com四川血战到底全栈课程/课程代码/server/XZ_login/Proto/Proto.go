package ProtoDSQ

import (
	"XZ_server/Player"
)

/*
   LollipopGo架构的设计的规则：C语言设计，顺序执行
   区分游戏：主协议区分 例如 斗兽棋，麻将，斗地主，U3D对战游戏的
   功能区分：子协议区分

   leaf，等
   消息号通过启动服务器加载到内存，绑定func；异步回调
   消息--server find map  消息 ？ 回调func：不处理
*/

// 斗兽棋的主协议 == 10号
const (
	PROTODSQ_INIT          = iota // 初始化
	C2L_Player_Login_Proto        // C2L_Player_Login_Proto == 1  客户端请求登录服务器，获取：tocken+url
	L2C_Player_Login_Proto        // L2C_Player_Login_Proto == 2  服务器返回 tocken+url
	G2L_Player_Login_Proto        // G2L_Player_Login_Proto == 3  游戏服务器验证 tocken
	L2G_Player_Login_Proto        // L2G_Player_Login_Proto == 4  服务器--返回数据 playerdata
)

// -----------------------------------------------------------------------------
// G2L_Player_Login_Proto == 3  游戏服务器验证 tocken
type G2L_Player_Login struct {
	Protocol  int
	Protocol2 int
	Tocken    string // 登录验证码
}

// L2G_Player_Login_Proto == 4  服务器--返回数据 playerdata
type L2G_Player_Login struct {
	Protocol   int
	Protocol2  int
	Isucc      bool                   // true:验证成功，false:标识验证失败
	PlayerData *Player_DSQ.PlayerData // 玩家的结构信息
}

// -----------------------------------------------------------------------------
// L2C_Player_Login_Proto == 2  服务器返回 tocken+url
type L2C_Player_Login struct {
	Protocol  int
	Protocol2 int
	Tocken    string // 登录验证码
	URL       string // 游戏服务器的地址
}

//C2L_Player_Login_Proto == 1  客户端请求登录服务器，获取：tocken+url
type C2L_Player_Login struct {
	Protocol  int
	Protocol2 int
	DeviceID  string
}

// -----------------------------------------------------------------------------
