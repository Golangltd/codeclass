package Proto

// 主协议 == 规则
const (
	INIT_PROTO       = iota
	GameData_Proto   //  GameData_Proto == 1    游戏的主协议
	GameDataDB_Proto //  GameDataDB_Proto == 2  游戏的DB的主协议
	GameNet_Proto    //  GameNet_Proto == 3  游戏的NET主协议
)
