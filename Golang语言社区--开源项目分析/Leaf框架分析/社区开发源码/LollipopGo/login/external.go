package login

import (
	"LollipopGo/login/internal"
)

var (
	Module  = new(internal.Module)
	ChanRPC = internal.ChanRPC // 用于不同模块进行通信
)
