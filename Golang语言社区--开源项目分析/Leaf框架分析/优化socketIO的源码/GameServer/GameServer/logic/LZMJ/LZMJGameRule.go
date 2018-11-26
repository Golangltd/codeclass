package LZMJ

import (
	"GameServer/gamedata"
	"GameServer/logic"
	"GameServer/msg"
	"errors"
	"fmt"
)

const (
	//方位
	DONG = 1 //冬
	NAN  = 2 //南
	XI   = 3 //西
	BEI  = 4 //北

	//游戏状态
	GAMEPREPARE     = 1 //准备
	GAMEPROGRESSING = 2 //游戏中

	//摸打牌类型
	MO  = 1 //摸牌
	DA  = 2 //打牌
	MOG = 3 //杠后摸牌
	DAG = 4 //杠后打牌

	//逻辑思考状态
	THINKING = 1 //思考中
	WAITING  = 2 //等待玩家指令

	//吃碰杠胡层级
	CCHI      = 3 //吃
	CPENGGANG = 2 //碰、杠
	CHU       = 1 //胡

	//胡的类型
	HU   = "dianpaohu"   //点炮
	HUZM = "zimohu"      //自摸
	HUQG = "qiangganghu" //抢杠

	//胡牌种类
	PHU  = "01" //平胡
	MQ   = "02" //门清
	XQD  = "03" //七对
	PPH  = "04" //碰碰胡
	QYS  = "05" //清一色
	HYS  = "06" //混一色
	HDLY = "07" //海底捞月
	GSP  = "08" //杠上炮
	SSY  = "09" //十三幺
	QG   = "10" //抢杠
	QQR  = "11" //全求人
	CSB  = "12" //吃三比

	//决策机关闭原因
	CLOSENORMAL = 1 //正常关闭
	CLOSEPENG   = 2 //碰完关闭
	CLOSEGANG   = 3 //杠完关闭
	CLOSECHI    = 4 //吃完关闭
	CLOSEANGANG = 6 //暗杠完关闭

	//决策结果
	INIT      = 0 //未执行的决策
	WAITINGJC = 1 //等待决策
	YES       = 2 //执行
	NO        = 3 //过
)

var (
	PAI = []int{
		1, 2, 3, 4, 5, 6, 7, 8, 9, //万
		1, 2, 3, 4, 5, 6, 7, 8, 9,
		1, 2, 3, 4, 5, 6, 7, 8, 9,
		1, 2, 3, 4, 5, 6, 7, 8, 9,
		11, 12, 13, 14, 15, 16, 17, 18, 19, //条
		11, 12, 13, 14, 15, 16, 17, 18, 19,
		11, 12, 13, 14, 15, 16, 17, 18, 19,
		11, 12, 13, 14, 15, 16, 17, 18, 19,
		21, 22, 23, 24, 25, 26, 27, 28, 29, //筒
		21, 22, 23, 24, 25, 26, 27, 28, 29,
		21, 22, 23, 24, 25, 26, 27, 28, 29,
		21, 22, 23, 24, 25, 26, 27, 28, 29,
	}
	PaiName = map[int]string{
		1: "一万", 2: "二万", 3: "三万", 4: "四万", 5: "五万", 6: "六万", 7: "七万", 8: "八万", 9: "九万",
		11: "一条", 12: "二条", 13: "三条", 14: "四条", 15: "五条", 16: "六条", 17: "七条", 18: "八条", 19: "九条",
		21: "一筒", 22: "二筒", 23: "三筒", 24: "四筒", 25: "五筒", 26: "六筒", 27: "七筒", 28: "八筒", 29: "九筒",
		31: "东", 32: "南", 33: "西", 34: "北",
		41: "中", 42: "发", 43: "白板",
	}
)

//吃碰杠
type CPG struct {
	Target     int   //吃碰杠对象
	TargetCard int   //吃碰杠的牌
	Cards      []int //对应的一坎牌
}

//=========================================玩家牌=================================================

type PlayerCard struct {
	InHand []int
	AnGang []CPG
	Gang   []CPG
	Peng   []CPG
	Chi    []CPG
}

//添加碰
func (pc *PlayerCard) AddPengPai(target int, card int) {
	peng := CPG{
		Target:     target,
		TargetCard: card,
		Cards: []int{
			card,
			card,
			card,
		},
	}
	pc.Peng = append(pc.Peng, peng)
}

//添加吃牌
func (pc *PlayerCard) AddChiPai(target int, card int, cards []int) {
	chi := CPG{
		Target:     target,
		TargetCard: card,
		Cards:      cards,
	}
	pc.Chi = append(pc.Chi, chi)
}

//添加明杠
func (pc *PlayerCard) AddMingGang(target int, card int) {
	for i, v := range pc.Peng {
		if v.TargetCard == card {
			pc.Peng = append(pc.Peng[:i], pc.Peng[i+1:]...)
		}
	}
	gang := CPG{
		Target:     target,
		TargetCard: card,
		Cards: []int{
			card,
			card,
			card,
			card,
		},
	}
	pc.Gang = append(pc.Gang, gang)
}

//添加暗杠
func (pc *PlayerCard) AddAnGang(target int, card int) {
	angang := CPG{
		Target:     target,
		TargetCard: card,
		Cards: []int{
			card,
			card,
			card,
			card,
		},
	}
	pc.AnGang = append(pc.AnGang, angang)
}

//=========================================玩家最新活动的牌=================================================
type CACard struct {
	Card   int //牌
	Target int //该牌的对象，比如谁摸的牌
	Type   int //类型，MO、DA、MOG、DAG
}

//=========================================吃碰杠胡计算=================================================
//计算结果
type CalResult struct {
	ZM string `json:"ZM"`
	Hu string `json:"Hu"`
	AG string `json:"AG"`
	G  string `json:"G"`
	P  string `json:"P"`
	C  string `json:"C"`
}

//牌局结果
type GameResult struct {
	Dir   int //玩家方位
	UID   int //胡牌玩家的UID
	Card  int //胡的那一张牌
	ZYNnm int //中鱼的数量
	Types int //胡牌的类型
}

//=========================================消息传递=================================================

//接收到消息
func (rule *LZMJRule) OnMessage(message logic.RoomMessage) {
	switch message.Name {
	//房间相关消息
	case "enter":
		rule.OnEnter(message.Body.(*gamedata.Player))
	case "GameStart":
		rule.GameStart()
	case "reEnter":
		rule.ReEnter(message.Body.(*gamedata.Player))
	case "leave":
		rule.OnLeave(message.Body.(int))
	//麻将动作指令相关消息
	case "chupai":
		rule.OnChuPai(message.Body.(*msg.ChuPai)) //出牌
	case "pengpai":
		rule.OnPeng(message.Body.(*msg.Peng)) //碰牌
	case "gangpai":
		rule.OnGang(message.Body.(*msg.Gang)) //杠牌
	case "angang":
		rule.OnAnGang(message.Body.(*msg.AnGang)) //暗杠
	case "chipai":
		rule.OnChi(message.Body.(*msg.ChiPai)) //吃牌
	case "hupai":
		rule.OnHu(message.Body.(*msg.HuPai)) //胡牌
	}
}

//开始游戏逻辑
func (rule *LZMJRule) Start() {
	fmt.Println("Room logic start")
	rule.OnGameInit()
}

//开始逻辑思考，每秒执行一次
func (rule *LZMJRule) StartLogicThinker() {
	//fmt.Println("=========================== 每秒执行")
}

//游戏开始
func (rule *LZMJRule) GameStart() {
	fmt.Println("=========================== 游戏开始")
	rule.OnGameStart()
}

//=========================================规则玩法=================================================
//柳州麻将玩法
type LZMJRule struct {
	room        *logic.Room   //该局游戏的房间
	FW          [5]int        //方位
	numCurrent  int           //当前局数
	NumTotal    int           //总局数
	YuNum       int           //钓鱼数
	YuType      int           //钓鱼方式
	GameState   int           //游戏状态
	Banker      int           //庄家,用方位表示
	ActivePlayr int           //当前决策的玩家，用方位表示
	Pai         []int         //所有牌
	CardPlayer  [5]PlayerCard //玩家手牌，方位表示
	CardOut     []int         //已经出掉的牌
	CurrentCard CACard        //当前活动的牌，比如刚摸的牌，刚打的牌
	ThinkState  int           //逻辑思考状态
	Decision    Decision      //决策机
	MJResult    string        //本局游戏结果
}

//设置房间
func (rule *LZMJRule) SetRoom(room *logic.Room) {
	rule.room = room
}

//设置玩家方位
func (rule *LZMJRule) SetDir(UID int) (int, error) {
	dir, err := rule.GetEmptyDir()
	if err != nil {
		return 0, errors.New("SetDir failed")
	}
	rule.FW[dir] = UID
	return dir, nil
}

//通过方位获取玩家ID
func (rule *LZMJRule) GetPlayerIDByDir(dir int) int {
	return rule.FW[dir]
}

//获取玩家方位
func (rule *LZMJRule) GetDir(UID int) (int, error) {
	for i, v := range rule.FW {
		if v == UID {
			return i, nil
		}
	}
	return 0, errors.New("No this Player")
}

//获取空余方位
func (rule *LZMJRule) GetEmptyDir() (int, error) {
	for index := 1; index < 5; index++ {
		if rule.FW[index] == 0 {
			return index, nil
		}
	}
	return 0, errors.New("dir is full!")
}

//当玩家进入房间
func (rule *LZMJRule) OnEnter(player *gamedata.Player) {
	fmt.Println("有玩家进入：UID：", player.Playerinfo.ID)
	dir, err := rule.SetDir(player.Playerinfo.UID)
	if err != nil {
		player.Agent.Emit("RoomEnter", map[string]bool{"Error": true})
		return
	}
	//构建消息,可以通过room.players去构建，以后重构
	res := map[string]interface{}{}
	res["position"] = dir
	res["pid"] = player.Playerinfo.UID
	for index := 1; index < 5; index++ {
		if uid := rule.FW[index]; uid != 0 {
			roomplayer, ok := rule.room.Players[uid]
			if !ok {
				continue
			}
			res[fmt.Sprintf("ext%d", index)] = fmt.Sprintf(
				"%d*%d*%d*%s*%s*%s*%d",
				roomplayer.Playerinfo.UID,
				index,
				roomplayer.Playerinfo.Sex,
				roomplayer.Playerinfo.Name,
				roomplayer.Playerinfo.TXUrl,
				roomplayer.Playerinfo.IP,
				rule.room.RoomHost)
		} else {
			res[fmt.Sprintf("ext%d", index)] = ""
		}
	}
	//向加入房间的玩家播报
	player.Agent.Emit("RoomEnter", res)
	//向其他玩家播报，其他玩家不需要发送position和加入房间的pid
	delete(res, "position")
	delete(res, "pid")
	rule.room.BroadcastExcept("RoomEnter", res, []int{player.Playerinfo.UID})
	//播报该玩家准备
	Ready := map[string]int{}
	Ready["pid"] = player.Playerinfo.UID
	Ready["position"] = dir
	rule.room.BroadcastExcept("ready", Ready, []int{player.Playerinfo.UID})

	for i, _ := range rule.room.Players {
		Readys := map[string]int{}
		Readys["pid"] = i
		d, err := rule.GetDir(i)
		if err != nil {
			return
		}
		Readys["position"] = d
		player.Agent.Emit("ready", Readys)
	}

	//发送房间信息
	roomInfo := map[string]interface{}{}
	roomInfo["ext1"] = rule.NumTotal
	roomInfo["ext2"] = rule.numCurrent
	roomInfo["ext3"] = rule.YuNum
	roomInfo["ext4"] = rule.room.RoomID
	go player.Agent.Emit("roominfo", roomInfo)
}

//玩家重新进入
func (rule *LZMJRule) ReEnter(player *gamedata.Player) {
	dir, err := rule.GetDir(player.Playerinfo.UID)
	if err != nil {
		player.Agent.Emit("RoomEnter", map[string]bool{"Error": true})
		return
	}
	//构建消息,可以通过room.players去构建，以后重构
	res := map[string]interface{}{}
	res["position"] = dir
	res["pid"] = player.Playerinfo.UID
	for index := 1; index < 5; index++ {
		if uid := rule.FW[index]; uid != 0 {
			roomplayer, ok := rule.room.Players[uid]
			if !ok {
				continue
			}
			res[fmt.Sprintf("ext%d", index)] = fmt.Sprintf(
				"%d*%d*%d*%s*%s*%s*%d",
				roomplayer.Playerinfo.UID,
				index,
				roomplayer.Playerinfo.Sex,
				roomplayer.Playerinfo.Name,
				roomplayer.Playerinfo.TXUrl,
				roomplayer.Playerinfo.IP,
				rule.room.RoomHost)
		} else {
			res[fmt.Sprintf("ext%d", index)] = ""
		}
	}
	//向加入房间的玩家播报
	player.Agent.Emit("RoomEnter", res)
}

//当玩家离开房间
func (rule *LZMJRule) OnLeave(playerID int) {
	res := map[string]interface{}{}
	dir, err := rule.GetDir(playerID)
	if err != nil {
		return
	}
	res["position"] = dir
	rule.FW[dir] = 0
	go rule.room.BroadcastExcept("RoomExit", res, []int{playerID})
}

//当玩家掉线
func (rule *LZMJRule) OnDisconnect(playerID int) {

}

//=======================================决策机===================================================
type Decision struct {
	IsOpen         bool        //是否为空
	StartPlayer    int         //开始玩家，方位
	Result         []CalResult //计算结果
	WaitingDecison []DecisonItem
}

//等待决策
type DecisonItem struct {
	Dir            int
	Type           int
	Action         int
	Data           interface{}
	BroadcastEvent string //事件
}

func (d *Decision) AddDecison(dir int, dType int, action int, event string, data interface{}) {
	d.WaitingDecison = append(d.WaitingDecison, DecisonItem{
		Dir:            dir,
		Type:           dType,
		Action:         action,
		Data:           data,
		BroadcastEvent: event,
	})
}

func (d *Decision) CheckExcute() bool {
	res := true
	for _, v := range d.WaitingDecison {
		if v.Action == WAITINGJC {
			res = false
		}
	}
	return res
}
func (d *Decision) FindDecisionItem(dir int, dType int) *DecisonItem {
	for i, v := range d.WaitingDecison {
		if v.Dir == dir && v.Type == dType {
			return &d.WaitingDecison[i]
		}
	}
	return nil
}
