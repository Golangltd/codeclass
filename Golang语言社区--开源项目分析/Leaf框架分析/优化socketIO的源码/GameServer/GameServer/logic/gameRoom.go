package logic

import (
	"GameServer/gamedata"
	"errors"
	"fmt"
	"math/rand"
	"strconv"
	"time"
)

const (
	//房间状态
	ROOMINIT       = 1 //房间初始化
	ROOMPREPARE    = 2 //房间准备阶段
	ROOMPROGREEING = 3 //房间游戏中
	ROOMPAUSE      = 4 //房间暂停中
	ROOMEND        = 5 //房间结束

)

var (
	rooms map[int]*Room //当前使用的房间号
)

func init() {
	rooms = make(map[int]*Room)
}

//=========================================================房间管理=============================================================
//创建一个新房间
func NewRoom(psersonTotal int, hostID int, gameRule GameRule) (*Room, error) {
	roomNumber, err := GetNewRoomNumber()
	if err != nil {
		return &Room{}, nil
	}
	room := Room{}
	room.RoomID = roomNumber
	room.PersonTotal = psersonTotal
	room.GameRule = gameRule
	room.RoomState = ROOMINIT
	room.Players = make(map[int]*gamedata.Player)
	room.Message = make(chan RoomMessage)
	room.RoomHost = hostID
	fmt.Println(rooms)
	rooms[roomNumber] = &room
	//运行房间
	room.Run()
	return &room, nil
}

//通过房间号查找房间
func GetRoom(id int) (*Room, error) {
	if v, ok := rooms[id]; ok {
		return v, nil
	}
	return &Room{}, errors.New("get room failed")
}

//生成房间号
func GetNewRoomNumber() (int, error) {
	count := 0
	rnd := rand.New(rand.NewSource(time.Now().UnixNano()))
	num, err := strconv.Atoi(fmt.Sprintf("%06v", rnd.Int31n(1000000-100000)+100000))
	if err != nil {
		fmt.Println("创建房间号，生成随机码出错！")
	}
A:
	count++
	if _, ok := rooms[num]; ok {
		num++
		if count > 100 {
			fmt.Println("创建房间号，生成随机码出错！")
			return 0, nil
		}
		if num > 999999 || count > 10 {
			count = 0
			num, err = strconv.Atoi(fmt.Sprintf("%06v", rnd.Int31n(1000000)))
			goto A
		}
		goto A
	}
	return num, nil
}

//=========================================================房间=============================================================

type RoomMessage struct {
	Name string      //消息名
	Body interface{} //消息体
}

type Room struct {
	RoomID        int                      //房间ID
	PersonTotal   int                      //房间总人数人数
	GameRule      GameRule                 //游戏规则
	PersonCurrent int                      //房间当前人数
	RoomState     int                      //房间状态
	Players       map[int]*gamedata.Player //房间玩家列表
	Message       chan RoomMessage         //消息通道
	RoomTime      int                      //房间开始的时间
	timer1        TimerThinker             //逻辑计时器
	timer2        TimerThinker             //房间计时器
	RoomHost      int                      //房主

}

//开始运行房间
func (room *Room) Run() {
	room.GameRule.SetRoom(room)
	//开始规则逻辑
	go room.GameRule.Start()
	//开始规则逻辑思考，每秒一次
	/*room.timer1 = TimerThinker{
		func([]interface{}) {
			if room.RoomState > ROOMPREPARE {
				room.GameRule.StartLogicThinker()
			}
		},
		[]interface{}{},
		make(chan bool),
	}
	room.timer1.StartTimer()
	//开始房间计时器，用于销毁超过一小时没有人加入的空房间
	room.timer2 = TimerThinker{
		func([]interface{}) {
			if room.RoomState == ROOMINIT || room.PersonCurrent < 1 {
				room.Close()
			}
		},
		[]interface{}{},
		make(chan bool),
	}
	room.timer2.StartTimerByHour()*/
	room.DispatchMessage()
}

//消息响应
func (room *Room) DispatchMessage() {
	var msg RoomMessage
	go func() {
		for {
			select {
			case msg = <-room.Message:
				room.GameRule.OnMessage(msg)
			}
		}
	}()

}

//进入房间
func (room *Room) Enter(playerID int) error {
	//添加玩家
	player, ok := gamedata.GetOnlinePlayerByID(playerID)
	if ok {
		if room.RoomState == ROOMPROGREEING || room.PersonCurrent >= room.PersonTotal {
			if _, ok := room.Players[playerID]; ok {
				//触发进入逻辑
				room.Message <- RoomMessage{
					"reEnter",
					player,
				}
			}
			return nil
		}
		room.Players[playerID] = player
		room.PersonCurrent = len(room.Players)
	} else {
		return errors.New("Enter room failed")
	}
	//触发进入逻辑
	room.Message <- RoomMessage{
		"enter",
		player,
	}
	//设置房间状态
	if room.RoomState == ROOMINIT && room.PersonCurrent > 0 && room.PersonCurrent < room.PersonTotal {
		room.RoomState = ROOMPREPARE
	}
	if room.RoomState == ROOMPREPARE && room.PersonCurrent >= room.PersonTotal {
		room.RoomState = ROOMPROGREEING
		room.Message <- RoomMessage{
			"GameStart",
			nil,
		}
	}
	return nil
}

//离开房间
func (room *Room) Leave(playerID int) error {
	if room.RoomState == ROOMPREPARE || room.RoomState == ROOMEND {
		room.Players[playerID].Room = 0
		delete(room.Players, playerID)
		room.PersonCurrent = len(room.Players)
		//触发进入逻辑
		room.Message <- RoomMessage{
			"leave",
			playerID,
		}
		return nil
	}
	return errors.New("cannot leave without ending")
}

//关闭房间
func (room *Room) Close() {
	//TODO 通知房间的玩家关闭房间
	//room.Broadcast()
	//room.timer1.Close <- true
	//room.timer2.Close <- true
	for playerID, player := range room.Players {
		player.Room = 0
		player.State = gamedata.NOTONLINE

		_ = playerID
		_ = player
	}
	delete(rooms, room.RoomID)
}

//广播消息
func (room *Room) Broadcast(event string, data interface{}) {
	for _, player := range room.Players {
		go player.Agent.Emit(event, data)
	}
}

//广播消息,除了某个玩家
func (room *Room) BroadcastExcept(event string, data interface{}, uids []int) {
	//fmt.Println("===============================BroadcastExcept========================================")
	skip := false
	for playerID, player := range room.Players {
		//fmt.Println("=====================", playerID)
		for _, uid := range uids {
			//fmt.Println("UID:", uid)
			if uid == playerID {
				skip = true
			}
		}
		if skip {
			skip = false
		} else {
			go player.Agent.Emit(event, data)
			//fmt.Println("Send One")
		}
	}
	//fmt.Println("========================================================================================")
}

//发送消息到某一玩家
func (room *Room) BroadcastOne(event string, uid int, msg interface{}) {
	if player, ok := room.Players[uid]; ok {
		go player.Agent.Emit(event, msg)
	} else {
		fmt.Println("No this player id")
	}

}
