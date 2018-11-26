package web

import (
	"GameServer/modules/web/internal"
)

var (
	Module      = new(internal.Module)
	Router      = internal.Router
	ChanRPC     = internal.ChanRPC
	TaskChannel = internal.Task
)
