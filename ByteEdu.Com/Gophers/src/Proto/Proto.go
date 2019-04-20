package Proto

/*
  主协议
*/
const (
	ProtoINIT   = iota
	ProtoGopher // ProtoGopher == 1  地鼠的协议

)

//------------------------------------------------------------------------------

/*
  子协议
*/
const (
	Proto2INIT            = iota // 初始化
	C2S_PlayerLoginProto2        // C2S_PlayerLoginProto2 == 1  用户登录协议
	S2C_PlayerLoginProto2        // S2C_PlayerLoginProto2 == 2
)

//------------------------------------------------------------------------------
// 用户登录或者注册
type C2S_PlayerLogin struct {
	Protocol  int
	Protocol2 int
	Itype     int // 1:表示注册，2：表示登录
	LoginName string
	LoginPW   string
}

type S2C_PlayerLogin struct {
	Protocol  int
	Protocol2 int
	Token     string
}

//------------------------------------------------------------------------------
