package main

import (
	"DSQ_server/Match"
	"DSQ_server/Proto"
	"DSQ_server/ST"
	"fmt"
	"strconv"
	"sync"
	"time"
)

var (
	// GRoomManager    map[string]*GRoomSTData // 其中一种
	GRoomManagerPtr *GRoomSTData // 彬哥推荐
	GRoomID         int
)

/*
   1. 肯定联系到我们的之前设计的房间结构：RoomData
   2. 并发安全问题
*/

type GRoomSTData struct {
	GRoomData map[string]*DSQ_ST.RoomData
	RoomLock  *sync.RWMutex
}

func init() {
	// GRoomManager = make(map[string]*GRoomSTData)
	GRoomManagerPtr = NewGRoomManagerPtr()
	GRoomID = 1000
	go MatchTimer() // 全局的定时器，出现panic 或者中断，整个匹配机制就无法继续进行了。
}

func NewGRoomManagerPtr() *GRoomSTData {
	return &GRoomSTData{
		GRoomData: make(map[string]*DSQ_ST.RoomData),
		RoomLock:  new(sync.RWMutex),
	}
}

/*
	获取RoomID长度
	每一个匹配成功的数据的总和，例如： A,B ---> {A,B} len(GRoomData) == 1 ,C,D   ---> {C,D} len(GRoomData)+1
    1. 读写锁的使用
    2. 获取全局的RoomID
*/
func GetGRoomDataLen() int {
	GRoomManagerPtr.RoomLock.RLock()
	ilen := len(GRoomManagerPtr.GRoomData) + GRoomID
	GRoomManagerPtr.RoomLock.RUnlock()
	return ilen
}

/*
说明：1. 2人组合一个房间。
     2. 发送广播消息（Broadcast_PiPeiGame_Proto）
	 3. 房间生成规则
	 4. 处理数据结构，go 与我们住进程  -- 线程通信方式：Go 通过通信来实现共享内存
*/

func MatchTimer() {
	startTumer := time.NewTicker(time.Millisecond * 10)
	for {
		select {
		case <-startTumer.C:
			{ // 存的数据结构：map[string]*Player_DSQ.PlayerData
				// len(Gmap)%2 == 0 && len(Gmap) != 0
				datachan := <-DSQ_match.GMatchChan
				openida, openidb := "", ""
				for kc, _ := range datachan {
					if len(openida) == 0 {
						openida = kc
					} else {
						openidb = kc
					}
				}
				for _, v := range datachan {
					_ = v
					// 2个人匹配成功 ---> 去创建房间 （房间的创建规则？）  ---  逻辑思维
					// 玩家的广播链接信息，我们如何取？--- 看听课认真不
					// this.MapSafe.Put(STsend.PlayerData.OpenID+"|User", onlineUser)
					// 1. Get方法去取数据 --
					// 2. 循环我们这个数据结构，  for k,v :=range M  -- 1W
					vala, _ := M.Get(openida + "|User")
					valb, _ := M.Get(openidb + "|User")

					/*
					  1. 通过roomid获取获取roomData数据
					  2. 判断roomdata下面的seatdata数据是否2个玩家都有数据
					  3. 如果 第2步骤seatdata 已经全部有数据，以第1步骤的roomid创建房间信息
					  4. 否则继续循环数据，添加到 roomdata成员变量seatdata
					*/
					GRoomManagerPtr.RoomLock.RLock()
					roomdatatmp := GRoomManagerPtr.GRoomData[strconv.Itoa(GetGRoomDataLen())]
					GRoomManagerPtr.RoomLock.RUnlock()
					if roomdatatmp == nil {
						roomdatatmp = &DSQ_ST.RoomData{}
					}
					/* seatData的数据结构
						1. 2个玩家
						2. 每个玩家的棋牌初始化数据，4*4 服务器的棋盘初始化操作--保存到server内存数据中的
						type SeatDataST struct {
						SeatID     int // 默认的 0,1
						PlayerData *Player_DSQ.PlayerData
						LeftTime   int
					}
					*/
					seatdataa := &DSQ_ST.SeatDataST{
						SeatID:     0,
						PlayerData: vala.(*DSQGame).Player,
						LeftTime:   20,
					}

					seatdatab := &DSQ_ST.SeatDataST{
						SeatID:     1,
						PlayerData: valb.(*DSQGame).Player,
						LeftTime:   20,
					}

					roomdatatmp.SeatData[0] = seatdataa
					roomdatatmp.SeatData[1] = seatdatab

					/*
						val  是什么数据 -->
						type DSQGame struct {
							Connection *websocket.Conn
							StrMD5     string // 唯一ID
							MapSafe    *concurrent.ConcurrentMap
							Player     *Player_DSQ.PlayerData
						}

						// Broadcast_PiPeiGame_Proto == 4   服务器返回 是否成功
						type Broadcast_PiPeiGame struct {
							Protocol  int
							Protocol2 int
							Bsucc     bool             // true ：匹配成功， false：匹配失败
							RoomData  *DSQ_ST.RoomData // 房间信息，房间的ID，匹配成功的玩家的数据，当前的牌桌上的数据（双方的信息，倒计时等）
						}
					*/
					// 发消息，通知玩家匹配成功了
					data := Proto_DSQGame.Broadcast_PiPeiGame{
						Protocol:  10,
						Protocol2: 4,
						Bsucc:     true,
					}

					/*  房间的数据结构
					    1.  服务器的房间的manager
						2.  管理房间的数据结构，包括：玩家的数据（红方，蓝方），房间的GC（timer）
						3.  玩家的数据的游戏数据存储，包括（玩家的报名，玩家的结算数据）
						4.  自学习的数据收集，包括（玩家的行为，大数据的记录，4*4 ，先手玩家最先点那个牌）
					type RoomData struct {
						RoomID   int            // 房间if=d
						RoomName string         // 非必要的
						SeatData [2]*SeatDataST // 座位信息,默认规则 0号位置默认是 红方，1号位置默认是 蓝方
						TurnID   int            // 轮到谁，防止：玩家多操作
					}
					*/
					data.RoomData = roomdatatmp
					fmt.Println(data)
					vala.(*DSQGame).PlayerSendMessage(data)
					valb.(*DSQGame).PlayerSendMessage(data)
					break
				}
			}
		}
	}
}
