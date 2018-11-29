package match

import (
	"time"
)

/*
玩家匹配算法：
类型一： 1V1

1 一个玩家的结构，list  存储所有的报名的玩家(启动包括正在进入的玩家的)
2 首先：玩家可以一直的加入，并不影响匹配，基本算法形成。


*/

// 1 需要一个玩家报名后进来的，存储数据的结构信息
// 2 匹配玩家，要求按照报名的时间的最长的开始匹配
// 3 生产结果；匹配模块需要处理的
// 4 实现方式：有一个匹配池子，每一个用户都是去池子里面匹配，成功返回结构
// 5 匹配的池子包含很多因素：等级相近，金币相近等

type MatchMoudle interface {
	GetMatchResult()
}

var SubPlayer []int

type MatchPlayer struct { // 匹配的数据
}

//-------------------------------------------------------------

type MatchMoudle interface {
	Init(selfGetter DModuleGetter) bool
	Destroy()
	Run(delta int64)
}

type implMatchModule struct {
	module        MatchMoudle
	tickUseTime   time.Duration
	tickTimeTotal time.Duration
	moduleName    string
}

type MatchModuleMgr []implMatchModule

type MatchModuleGetter struct {
	mgr *MatchModuleMgr
	id  int
}

// NewDmodule ... ...
func NewMatchmodule(num int) MatchModuleMgr {
	return make([]implMatchModule, num)
}

// Get ... ...
func (g *DModuleGetter) Get() MatchMoudle {
	return (*g.mgr)[g.id].module
}

//Register ... ...
func (mgr *MatchModuleMgr) Register(id int, m MatchMoudle) MatchModuleGetter {
	(*mgr)[id].module = m
	mName := strings.Replace(reflect.TypeOf(m).String(), "*", "_", -1)
	mName = "module" + strings.Replace(mName, ".", "_", -1)
	mName = strings.ToLower(mName)
	(*mgr)[id].moduleName = mName
	return mgr.Getter(id)
}
