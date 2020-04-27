package db

import (
	"youyugame/modules/db/internal"
	_ "youyugame/modules/db/internal/conf"
)

var (
	Module  = new(internal.Module)
	ChanRPC = internal.ChanRPC
)
