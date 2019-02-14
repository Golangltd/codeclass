package main

import (
	"FenDZ/glog-master"
	"FenDZ/go-concurrentMap-master"
	"LollipopGo/conf"
	//  "LollipopGo/game"
	"LollipopGo/gate"
	"LollipopGo/global"
	"flag"
	_ "time"

	"LollipopGo/login"
	"github.com/LollipopGo/LollipopGo"
	_ "github.com/dop251/goja" // JS 解析器
	lconf "github.com/name5566/leaf/conf"
)

func init() {
	glog.Info("Entry init")
	// 初始化 日志系统
	flag.Set("alsologtostderr", "true") // 日志写入文件的同时，输出到stderr
	flag.Set("log_dir", "./log")        // 日志文件保存目录
	flag.Set("v", "3")                  // 配置V输出的等级。
	flag.Parse()
	// 初始化并发安全map
	global.M = concurrent.NewConcurrentMap()

	return
}

func main() {

	// 加载配置
	lconf.LogLevel = conf.Server.LogLevel
	lconf.LogPath = conf.Server.LogPath
	lconf.LogFlag = conf.LogFlag
	lconf.ConsolePort = conf.Server.ConsolePort
	lconf.ProfilePath = conf.Server.ProfilePath

	lollipopgo.Run(
		//game.Module,
		gate.Module,
		login.Module,
	)
}
