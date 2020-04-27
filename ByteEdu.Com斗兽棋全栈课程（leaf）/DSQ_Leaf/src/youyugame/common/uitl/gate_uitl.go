package uitl_yy

import (
	"github.com/name5566/leaf/gate"
)

func PlayerSendMessage(args []interface{}, data interface{}) {
	a := args[1].(gate.Agent)
	a.WriteMsg(data)
}
