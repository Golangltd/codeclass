package main

import (
	"DSQ_login/Proto"
	//	"DSQ_server/Player"
	"DSQ_server/Match"
	"DSQ_server/Proto"
	"LollipopGo/LollipopGo/network"
	"encoding/json"
	"fmt"
)

// 玩家匹配
func (this *DSQGame) PlayerPiPei(ProtocolData map[string]interface{}) {
	fmt.Println("PlayerPiPei-------------------", ProtocolData)
	strOpenID := ProtocolData["OpenID"].(string)

	// 匹配逻辑
	// 1. 队列形式-- chan
	// 2. 匹配成功
	val, err := this.MapSafe.Get(strOpenID + "|User")
	if err != nil {
		fmt.Println(err)
		return
	}
	fmt.Println("--------------------val:", val)
	DSQ_match.PutMatchList(val.(*DSQGame).Player)
	return
}

// 玩家登录
func (this *DSQGame) PlayerLogin(ProtocolData map[string]interface{}) {
	fmt.Println("PlayerLogin-------------------", ProtocolData)
	strtoken := ProtocolData["Tocken"].(string)

	// 去登录服务器验证 tocken是否有效
	retstr := impl.Get("http://127.0.0.1:4001/BaBaLiuLiu_Server?Protocol=10&Protocol2=3&Token=" + strtoken)
	fmt.Println("impl.Get", retstr)
	// 解析login server 返回的数据
	//
	// 整体数据结构解析
	STsend := &ProtoDSQ.L2G_Player_Login{}
	json.Unmarshal([]byte(retstr), STsend)

	// 发送客户端
	data := &Proto_DSQGame.G2C_Player_Login{
		Protocol:  10,
		Protocol2: 2,
		Bsucc:     true,
	}

	if !STsend.Isucc {
		data.Bsucc = false
		this.PlayerSendMessage(data)
		return
	}
	fmt.Println("STsend.PlayerData", STsend.PlayerData)
	fmt.Println("STsend.PlayerData.OpenID", STsend.PlayerData.OpenID)
	// 玩家数据解析
	// 服务器的数据连接信息保存
	// 1. 玩家的链接信息，conn , playerdata 等等 需要进行保存。
	// 2. 玩家的消息的推送，服务器通知玩家（服务器广播）。
	// 3. 网络信息的断线重来。sss
	// 数据保存
	onlineUser := &DSQGame{
		Connection: this.Connection,
		MapSafe:    this.MapSafe,
		Player:     STsend.PlayerData,
	}
	this.MapSafe.Put(STsend.PlayerData.OpenID+"|User", onlineUser)
	// 发送数据
	data.Player = STsend.PlayerData
	this.PlayerSendMessage(data)
	return
}
