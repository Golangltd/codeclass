package main

import (
	"glog-master"
	"net/http"
	"os"

	_ "net/http/pprof"

	"code.google.com/p/go.net/websocket"
)

func main() {
	// os.Args[0] == 执行文件的名字
	// os.Args[1] == 第一个参数
	// os.Args[2] == 类型 Client -websocket-> GW -websocket/rpc-> GS -websocket/rpc-> DB
	glog.Info(os.Args[:])
	if len(os.Args[:]) < 3 {
		panic("参数小于2个！！！ 例如：xxx.exe +【端口】+【服务器类型】")
		return
	}
	strport := "8888"
	strServerType := "GW"
	strServerType_GW := "GW"
	strServerType_GS := "GS"
	strServerType_DB := "DB"
	if len(os.Args) > 1 {
		strport = os.Args[1]
		strServerType = os.Args[2]
	}
	glog.Info(strport)
	glog.Info(strServerType)
	glog.Info("Golang语言社区")
	glog.Flush()
	if strServerType == strServerType_GW {
		glog.Info("Golang语言社区:网关服务器启动！")
		http.Handle("/GolangLtd", websocket.Handler(wwwGolangLtd))
		if err := http.ListenAndServe(":"+strport, nil); err != nil {
			glog.Error("网络错误", err)
			return
		}
	} else if strServerType == strServerType_GS {
		strport = "8889"    // 多个 -- server
		go GameServerINIT() // 游戏服务器的初始化操作
		http.Handle("/GolangLtdGS", websocket.Handler(wwwGolangLtd))
		if err := http.ListenAndServe(":"+strport, nil); err != nil {
			glog.Error("网络错误", err)
			return
		}
	} else if strServerType == strServerType_DB {
		strport = "8890"
		http.Handle("/GolangLtdDB", websocket.Handler(wwwGolangLtd))
		if err := http.ListenAndServe(":"+strport, nil); err != nil {
			glog.Error("网络错误", err)
			return
		}
	}
	panic("【服务器类型】不存在")
}
