package main

import (
	"DSQ_server/Player"
	"LollipopGo/LollipopGo"
	"LollipopGo/LollipopGo/network"
	"cache2go"
	"flag"
	"glog-master"
	"go-concurrentMap-master"
	"net/http"

	"code.google.com/p/go.net/websocket"
)

// 链接存储结构
type DSQGame struct {
	Connection *websocket.Conn
	StrMD5     string // 唯一ID
	MapSafe    *concurrent.ConcurrentMap
	Player     *Player_DSQ.PlayerData
}

var cache *cache2go.CacheTable
var M *concurrent.ConcurrentMap // 并发安全

func init() {
	impl.IMsg = new(DSQGame)
	cache = cache2go.Cache("DSQGame")
	M = concurrent.NewConcurrentMap()

	flag.Set("alsologtostderr", "true") // 日志写入文件的同时，输出到stderr
	flag.Set("log_dir", "./log")        // 日志文件保存目录
	flag.Set("v", "3")                  // 配置V输出的等级。
	flag.Parse()
	return
}

func main() {
	LollipopGo.Run()
	http.Handle("/DSQ", websocket.Handler(BuildConnection))
	if err := http.ListenAndServe(":4002", nil); err != nil {
		glog.Info("Entry nil", err.Error())
		glog.Flush()
		return
	}
	return
}

func BuildConnection(ws *websocket.Conn) {
	data := ws.Request().URL.Query().Get("data")
	glog.Info(data)
	if data == "" {
		glog.Info("data is Nil")
		glog.Flush()
	}
	impl.InitConnection(ws)
}
