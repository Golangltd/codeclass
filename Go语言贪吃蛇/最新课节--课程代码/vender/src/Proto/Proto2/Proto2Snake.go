package Proto2

const (
	ININSNAKE              = iota
	C2S_PlayerLoginSProto2 // PlayerLoginSProto2 == 1 登陆协议
	S2S_PlayerLoginSProto2 // S2S_PlayerLoginSProto2 == 2 登陆协议
)

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
