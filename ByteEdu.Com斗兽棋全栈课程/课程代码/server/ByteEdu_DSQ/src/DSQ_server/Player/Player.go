package Player_DSQ

// 玩家架构信息
type PlayerData struct {
	UID    int    // 玩家UID
	OpenID string // MD5 字符串
	Name   string // 玩家名字
	Avatar string // 玩家头像
	Level  int    // 玩家等级
	Coin   int64  // 玩家金币
	// ... ...
}
