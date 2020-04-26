package DSQ_match

import (
	"DSQ_server/Player"
	"time"
)

/*
   如何解决 匹配高并发下的panic问题（第二期解决）
*/

var (
	GMatchChan chan map[string]*Player_DSQ.PlayerData
	Gmap       map[string]*Player_DSQ.PlayerData
)

func init() {
	GMatchChan = make(chan map[string]*Player_DSQ.PlayerData, 1000)
	Gmap = make(map[string]*Player_DSQ.PlayerData)
	go Timer()
}

// 玩家的数据匹配压入
func PutMatchList(data *Player_DSQ.PlayerData) {
	Gmap[data.OpenID] = data
}

// 匹配timer
func Timer() {
	for {
		select {
		case <-time.After(time.Millisecond * 1):
			{
				// 进行数据验证 -- 确保我们链接信息完全获取到
				// 不够严谨，如果不验证玩家数据是否保存成功，会导致客户端一个匹配成功，无法游戏。
				// 作业，剔除不在线玩家
				// 确保2个人， 如果满足len(Gmap)%2 == 0  --->  GMatchChan
				if len(Gmap)%2 == 0 && len(Gmap) != 0 {
					datatmp := make(map[string]*Player_DSQ.PlayerData)
					for k, v := range Gmap {
						datatmp[k] = v
						delete(Gmap, k)
					}
					SendGMatchChan(datatmp)
				}
			}
		}
	}
}

// 压如匹配函数
func SendGMatchChan(data map[string]*Player_DSQ.PlayerData) {
	if len(data) == 0 {
		return
	}
	GMatchChan <- data
}
