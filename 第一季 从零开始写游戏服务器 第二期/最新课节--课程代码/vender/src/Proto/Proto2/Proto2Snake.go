package Proto2

const (
	ININSNAKE              = iota
	C2S_PlayerLoginSProto2 // PlayerLoginSProto2 == 1 登陆协议
	S2S_PlayerLoginSProto2 // S2S_PlayerLoginSProto2 == 2 登陆协议

	C2S_PlayerEntryGameProto2 // C2S_PlayerEntryGameProto2 == 3 进入游戏
	S2S_PlayerEntryGameProto2 // S2S_PlayerEntryGameProto2 == 4
)

//------------------------------------------------------------------------------

// 进入游戏匹配
type C2S_PlayerEntryGame struct {
	Protocol  int
	Protocol2 int
	Code      string //临时码
}

//  返回数据操作
type S2S_PlayerEntryGame struct {
	Protocol  int
	Protocol2 int
	RoomID    int //房间ID
}

//------------------------------------------------------------------------------

// 登陆  客户端--> 服务器
type C2S_PlayerLoginS struct {
	Protocol   int
	Protocol2  int
	Login_Name string
	Login_PW   string
}

type S2S_PlayerLoginS struct {
	Protocol  int
	Protocol2 int
	Token     string // Token 的设计
}

//------------------------------------------------------------------------------
