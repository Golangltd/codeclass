package msg

import (
	"github.com/name5566/leaf/network"
)

// 网络处理，处理数据结构：json，protobuf
var Processor network.Processor // json 管理器  leaf 默认
//var Processor = json.NewProcessor() // json
//var Processor = protobuf.NewProcessor() // protobuf

func init() {
	// 注册消息
	//test {
	Processor.Register(&Hello{})
	Processor.Register(&Hellobak{})
	// }
}

// 一个结构体定义了一个 JSON 消息的格式
// 消息名为 Hello
type Hello struct {
	Name string
}

type Hellobak struct {
	Name string
}
