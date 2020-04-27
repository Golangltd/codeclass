package slot

import (
	"youyugame/modules/slot/internal"
	_ "youyugame/modules/slot/internal/AI"
)

var (
	Module  = new(internal.Module)
	ChanRPC = internal.ChanRPC
)
