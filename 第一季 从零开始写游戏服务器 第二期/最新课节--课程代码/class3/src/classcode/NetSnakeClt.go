package main

import (
	"Proto"
	"Proto/Proto2"
)

// 子协议的处理
func (this *NetDataConn) HandleCltProtocol2Snake(protocol2 interface{}, ProtocolData map[string]interface{}) {

	switch protocol2 {
	case float64(Proto2.C2S_PlayerEntryGameProto2):
		{
			// 玩家进入游戏的协议
			this.EntryGameSnake(ProtocolData)
		}
	case float64(Proto2.C2S_PlayerLoginSProto2):
		{
			// 登录协议
			this.LoginGameSnake(ProtocolData)
		}

	default:
		panic("子协议：不存在！！！")
	}

	return
}

// 玩家 进入游戏的协议
func (this *NetDataConn) EntryGameSnake(ProtocolData map[string]interface{}) {
	// 进入游戏 进行匹配
	return
}

// 登录游戏协议
func (this *NetDataConn) LoginGameSnake(ProtocolData map[string]interface{}) {
	// 数据链接信息需要保存

	return
}
