package main

import (
	"fmt"
	_ "go-concurrent-map-master"
)

// 全局的网络信息结构
var G_PlayerData map[string]*NetDataConn
var G_PlayerNet map[string]int // 心跳结构信息存储的结构
var G_PlayerNetSys map[string]int
var G_Net_Count map[string]int

// 游戏服务器的初始化
func init() {
	// 初始化
	G_PlayerData = make(map[string]*NetDataConn)
	G_PlayerNet = make(map[string]int)
	G_PlayerNetSys = make(map[string]int)
	G_Net_Count = make(map[string]int)
	go G_timer()
	go G_timeout_kick_Player()
	return
}

func Go_func() {
	fmt.Println("Golang语言社区")
	return
}
