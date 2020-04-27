package main

import (
	"youyugame/conf"
	"youyugame/modules/MsgCenter"
	"youyugame/modules/db"
	"youyugame/modules/dsq"
	"youyugame/modules/gate"
	"youyugame/modules/login"
	"youyugame/modules/slot"

	"github.com/name5566/leaf"
	lconf "github.com/name5566/leaf/conf"
)

func main() {

	lconf.LogLevel = conf.Server.LogLevel
	lconf.LogPath = conf.Server.LogPath
	lconf.LogFlag = conf.LogFlag
	lconf.ConsolePort = conf.Server.ConsolePort
	lconf.ProfilePath = conf.Server.ProfilePath

	leaf.Run(
		// ---------------------------------------------------------------------
		// 子游戏服务器
		gate.Module,
		login.Module, // 大厅服务器，也是长链接
		slot.Module,
		// ---------------------------------------------------------------------
		MsgCenter.Module,
		db.Module,
		dsq.Module,
		// ---------------------------------------------------------------------
	)
}
