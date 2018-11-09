package internal

import (
	"LollipopGo/base"

	"FenDZ/glog-master"

	"github.com/LollipopGo/lollipopgo/module"
)

var (
	skeleton = base.NewSkeleton()
	ChanRPC  = skeleton.ChanRPCServer // 模块间通信
)

// 定义了结构
type Module struct {
	*module.Skeleton
}

// 启动的时候调用
func (m *Module) OnInit() {
	glog.Info("Entry module OnInit")
	m.Skeleton = skeleton
}

// 服务器关闭的时候调用
func (m *Module) OnDestroy() {
	glog.Info("Entry module OnDestroy")
}
