package main

import (
	"GameServer/conf"
	gamedata "GameServer/gamedata/constDefine"
	"GameServer/rest"
	"log"
	"net/http"

	"GameServer/modules/game"
	"GameServer/modules/gate"
	"GameServer/modules/login"
	"GameServer/modules/web"

	"GameServer/leaf"
	lconf "GameServer/leaf/conf"
	"GameServer/modules/CalRPC"
)

func main() {
	lconf.LogLevel = conf.Server.LogLevel
	lconf.LogPath = conf.Server.LogPath
	lconf.LogFlag = conf.LogFlag
	lconf.ConsolePort = conf.Server.ConsolePort
	lconf.ProfilePath = conf.Server.ProfilePath
	//初始化固定数据
	gamedata.Init()
	//开启restful API 服务
	go startRestAPI()
	//开始游戏主服务
	leaf.Run(
		game.Module,
		gate.Module,
		login.Module,
		web.Module,
		CalRPC.Module,
	)

}
func startRestAPI() {
	api := rest.NewApi()
	api.Use(rest.DefaultDevStack...)
	api.SetApp(web.Router)
	log.Fatal(http.ListenAndServe(":7777", api.MakeHandler()))
}
