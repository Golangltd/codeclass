package main

import (
	"DSQ_server/Proto"
	"Proto"
	_ "fmt"
	"glog-master"

	"code.google.com/p/go.net/websocket"
)

// 主协议处理
func (this *DSQGame) HandleCltProtocol(protocol interface{}, protocol2 interface{}, ProtocolData map[string]interface{}, Connection *websocket.Conn) interface{} {
	switch protocol {
	case float64(Proto.G_GameDSQ_Proto):
		{
			this.HandleCltProtocol2(protocol2, ProtocolData, Connection)
		}
	default:
		glog.Info("protocol default")
	}
	return 0
}

func (this *DSQGame) HandleCltProtocol2(protocol2 interface{}, ProtocolData map[string]interface{}, Connection *websocket.Conn) interface{} {
	ConnectionData := &DSQGame{
		Connection: Connection,
		MapSafe:    M,
	}
	switch protocol2 {
	case float64(Proto_DSQGame.C2G_Player_PiPeiGame_Proto):
		{
			ConnectionData.PlayerPiPei(ProtocolData)
		}
	case float64(Proto_DSQGame.C2G_Player_Login_Proto):
		{
			ConnectionData.PlayerLogin(ProtocolData)
		}
	default:
		glog.Info("protocol2 default")
	}
	return 0
}
