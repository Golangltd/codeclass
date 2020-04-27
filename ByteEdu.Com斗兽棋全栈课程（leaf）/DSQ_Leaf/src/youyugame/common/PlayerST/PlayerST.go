package playerdata

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

// 座位信息
type SeatDataST struct {
	SeatID     int // 默认的 0,1
	PlayerData *PlayerData
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
	RoomID          string         // 房间if=d  唯一的
	RoomName        string         // 非必要的
	SeatData        [2]*SeatDataST // 座位信息,默认规则 0号位置默认是 红方，1号位置默认是 蓝方
	TurnID          int            // 轮到谁，防止：玩家多操作，0和1
	ChessData       [4][4]int      // 棋盘数据--server  0：表示空地
	ChessDataClient [4][4]int      // 客户单显示棋盘数据 ---client，就是记录客户端行为：翻开操作，0：表示未翻起，1：翻起，2：空地（断线重连）
}

// 结算的数据块
type GameOver struct {
	PlayerWin  *PlayerData // 胜利的玩家
	PlayerFail *PlayerData // 失败的玩家
	GameName   string      // 游戏的名字
	WinSeatID  int         // 胜利人的位置信息，0或者1的位置信息
	Score      int         // 胜利人获取的分数信息
	// ... ... 根据自己游戏类型设计结算的数据块
}
