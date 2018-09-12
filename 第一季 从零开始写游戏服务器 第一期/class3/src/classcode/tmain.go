package main

import (
	"flag"
	"glog-master"
	"net/http"
	"os"
	"runtime"

	"code.google.com/p/go.net/websocket"
)

// 初始化函数
func init() {
	// 命令的执行：
	// server.exe -log_dir="./" -v=3

	// 程序当中执行：
	flag.Set("alsologtostderr", "true") // 日志写入文件的同时，输出到stderr
	flag.Set("log_dir", "./log")        // 日志文件的保存的目录
	flag.Set("v", "3")                  // 配置日志输出的等级
	flag.Parse()

	return
}

func main() {
	// os.Args[0] == 执行文件的名字
	// os.Args[1] == 第一个参数
	// glog.Info(os.Args[:])
	strport := "8888"
	if len(os.Args) > 1 {
		strport = os.Args[1]
	}
	glog.Info(strport)
	glog.Info("Golang语言社区")
	glog.Flush()
	// 游戏服务器开发 如何利用cpu的多核？
	glog.Info("本机几核：", runtime.NumCPU())
	runtime.GOMAXPROCS(runtime.NumCPU())

	http.Handle("/GolangLtd", websocket.Handler(wwwGolangLtd))
	if err := http.ListenAndServe(":"+strport, nil); err != nil {
		glog.Error("网络错误", err)
		return
	}
	glog.Flush()
}
