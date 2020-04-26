package conn

import (
	"go-concurrentMap-master"
	"slot_animal_9/common/PlayerST"
)

/*

  1. 定义结构数据
  2. 玩家的链接信息，玩家的唯一ID，

*/

type DSQGame struct {
	// Connection *websocket.Conn  leaf的结构信息    作业: 如何获取  Connection *websocket.Conn
	StrMD5  string //  目前可以设计为房间ID信息
	MapSafe *concurrent.ConcurrentMap
	Player  *playerdata.Player
}
