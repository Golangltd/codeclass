package Proto_DSQGameErr

// 1000的主协议
const (
	ERRORINIT     = iota
	ErrorMsgProto // ErrorMsg == 1
)

// 统一错误格式
type ErrorMsg struct {
	Protocol  int
	Protocol2 int
	Code      string // 错误码 --> 描述信息
	Msg       string // 显示的错误描述信息
}
