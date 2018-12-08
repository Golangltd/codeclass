package log

import (
	"flag"
	"glog-master"
)

func init() {
	// 程序当中执行：
	flag.Set("alsologtostderr", "true") // 日志写入文件的同时，输出到stderr
	flag.Set("log_dir", "./log")        // 日志文件的保存的目录
	flag.Set("v", "3")                  // 配置日志输出的等级
	flag.Parse()
}

func LollipopGoInfo(data interface{}) {
	glog.Info(data)
	return
}
