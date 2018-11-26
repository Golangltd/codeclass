package LZMJ

import (
	"GameServer/logic"
	"GameServer/modules/CalRPC"
	pb "GameServer/modules/CalRPC/CalCall"
	"GameServer/msg"
	"GameServer/rest"
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"math/rand"
	"strconv"
	"strings"
	"time"
)

//初试化游戏
func (rule *LZMJRule) OnGameInit() {
	//设置游戏状态
	rule.GameState = GAMEPREPARE
}

//===================================流程控制=========================================================
//开始游戏
func (rule *LZMJRule) OnGameStart() {
	//设置游戏状态
	rule.GameState = GAMEPROGRESSING
	//初始化牌
	rule.Pai = make([]int, len(PAI))
	copy(rule.Pai, PAI)
	//洗牌
	for i := 0; i < 10; i++ {
		rule.RandCardData(rule.Pai, len(rule.Pai)-1)
	}
	//随机庄家
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	rule.Banker = r.Intn(int(255))%4 + 1
	//播报游戏开始
	Start := map[string]string{}
	Start["ext1"] = strconv.Itoa(r.Intn(int(255))%6 + 1)
	Start["ext2"] = strconv.Itoa(r.Intn(int(255))%6 + 1)
	Start["ext3"] = strconv.Itoa(rule.Banker)
	rule.room.Broadcast("start", Start)
	//设置当前决策玩家为庄家
	rule.ActivePlayr = rule.Banker
	//发牌
	/*for index := 0; index < 13; index++ {
		rule.DispatchCardToPlayer(1)
		rule.DispatchCardToPlayer(2)
		rule.DispatchCardToPlayer(3)
		rule.DispatchCardToPlayer(4)
	}*/
	card1 := []int{26, 26, 26, 0x2, 0x2, 0x2, 0x3, 0x3, 0x3, 0x4, 0x4, 0x4, 0x5}
	card2 := []int{0x6, 0x6, 0x6, 0x7, 0x7, 0x7, 23, 23, 23, 24, 24, 24, 25}
	card3 := []int{12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16}
	card4 := []int{17, 17, 17, 18, 18, 18, 19, 19, 19, 21, 21, 21, 22}
	rule.DispatchFixCards(card1, 1)
	rule.DispatchFixCards(card2, 2)
	rule.DispatchFixCards(card3, 3)
	rule.DispatchFixCards(card4, 4)
	time.Sleep(2000)
	for dir := 1; dir < 5; dir++ {
		Map := map[string]string{}
		Map["pid"] = strconv.Itoa(rule.GetPlayerIDByDir(dir))
		Map["position"] = strconv.Itoa(dir)
		Map["message"] = rule.GetPlayerCardStr(dir)
		rule.room.BroadcastOne("fapai", rule.GetPlayerIDByDir(dir), Map)
	}
	//庄家摸第一张牌
	t := logic.TimerThinker{}
	t.Close = make(chan bool)
	t.Args = nil
	t.Callback = func([]interface{}) {
		rule.MoPai(rule.Banker, MO)
	}
	t.StartTimerByX(300)

}

//玩家摸牌
func (rule *LZMJRule) MoPai(dir int, moType int) int {
	CardInhandTemp := make([]int, len(rule.CardPlayer[dir].InHand))
	copy(CardInhandTemp, rule.CardPlayer[dir].InHand)
	newCard, err := rule.DispatchCardToPlayer(dir)
	if err != nil {
		return 0
	}
	if newCard == 0 {
		//游戏结束
		return 0
	}
	//设置思考状态
	rule.ThinkState = THINKING
	//设置当前活动玩家
	rule.ActivePlayr = dir
	//设置摸来的牌为当前活动的牌
	rule.CurrentCard = CACard{
		Card:   newCard,
		Target: dir,
		Type:   moType,
	}
	//播报摸牌
	Mopai := map[string]interface{}{}
	Mopai["pid"] = strconv.Itoa(rule.GetPlayerIDByDir(dir))
	Mopai["message"] = "10" + fmt.Sprintf("%02d", newCard)
	Mopai["position"] = dir
	switch moType {
	case MOG:
		Mopai["order"] = "minggang1"
	}
	rule.room.BroadcastOne("mopai", rule.GetPlayerIDByDir(dir), Mopai)
	Mopaio := map[string]interface{}{}
	Mopaio["pid"] = strconv.Itoa(rule.GetPlayerIDByDir(dir))
	Mopaio["message"] = "1001"
	Mopaio["position"] = dir
	rule.room.BroadcastExcept("mopai", Mopaio, []int{rule.GetPlayerIDByDir(dir)})
	go func() {
		//吃碰胡计算
		fmt.Println("开始吃碰胡计算")
		calRes, err := rule.GoCal(CardInhandTemp)
		if err != nil {
			return
		}
		rule.DecisionOpen(dir, calRes)
		rule.DecisionThink()
	}()
	//思考状态为等待
	rule.ThinkState = WAITING
	return newCard
}

//当玩家打牌
func (rule *LZMJRule) OnChuPai(msg *msg.ChuPai) int {
	rule.ThinkState = THINKING
	dir, err := strconv.Atoi(msg.Dir)
	if err != nil {
		panic(err)
		return 0
	}
	//如果不是该玩家决策。该消息无效
	//后期可添加 强制更新该玩家数据
	if dir != rule.ActivePlayr {
		panic(err)
		return 0
	}
	//设置摸来的牌为当前活动的牌
	card, err := strconv.Atoi(rest.Substr(msg.Message, 2, 2))
	if err != nil {
		panic(err)
		return 0
	}
	rule.CurrentCard = CACard{
		Card:   card,
		Target: dir,
		Type:   DA,
	}
	err = rule.RemoveCard(dir, card)
	if err != nil {
		panic(err)
		return 0
	}
	//播报出牌
	rule.room.BroadcastExcept("chupai", msg, []int{rule.GetPlayerIDByDir(dir)})
	go func() {
		//吃碰胡计算
		fmt.Println("开始吃碰胡计算")
		calRes, err := rule.GoCal(rule.CardPlayer[dir].InHand)
		if err != nil {
			return
		}
		rule.DecisionOpen(dir, calRes)
		rule.DecisionThink()
	}()
	rule.ThinkState = WAITING
	return card
}

//碰牌处理
func (rule *LZMJRule) OnPeng(msg *msg.Peng) {
	dir, err := strconv.Atoi(msg.Position)
	if err != nil {
		return
	}
	decisionItem := rule.Decision.FindDecisionItem(dir, CPENGGANG)
	if decisionItem == nil {
		return
	}
	//rule.ActivePlayr = dir
	if msg.Message == "guo" {
		decisionItem.Action = NO
		rule.DecisionThink()
	} else {
		card, err := strconv.Atoi(rest.Substr(msg.Message, 2, 2))
		if err != nil {
			return
		}
		for i := 0; i < 2; i++ {
			err := rule.RemoveCard(dir, card)
			if err != nil {
				return
			}
		}
		target := decisionItem.Data.(map[string]interface{})["target"].(int)
		rule.CardPlayer[dir].AddPengPai(target, card)
		rule.room.Broadcast("bcpeng", msg)
		rule.DecisionClose(CLOSEPENG)
		rule.ActivePlayr = dir
	}
}

//吃牌处理
func (rule *LZMJRule) OnChi(msg *msg.ChiPai) {

}

//杠牌处理
func (rule *LZMJRule) OnGang(msg *msg.Gang) {
	dir, err := strconv.Atoi(msg.Position)
	if err != nil {
		return
	}
	decisionItem := rule.Decision.FindDecisionItem(dir, CPENGGANG)
	if decisionItem == nil {
		return
	}
	rule.ActivePlayr = dir
	if msg.Message == "guo" {
		decisionItem.Action = NO
		rule.DecisionThink()
	} else {
		card, err := strconv.Atoi(rest.Substr(msg.Message, 2, 2))
		if err != nil {
			return
		}
		//手中牌明杠
		if msg.Order == "minggang1" {
			for i := 0; i < 3; i++ {
				err := rule.RemoveCard(dir, card)
				if err != nil {
					return
				}
			}
			target := decisionItem.Data.(map[string]interface{})["target"].(int)
			rule.CardPlayer[dir].AddMingGang(target, card)
		}
		//碰牌中的杠，在添加杠中处理
		if msg.Order == "minggang2" {
			target := decisionItem.Data.(map[string]interface{})["target"].(int)
			rule.CardPlayer[dir].AddMingGang(target, card)
		}
		//暗杠
		if msg.Order == "angang" {
			for i := 0; i < 4; i++ {
				err := rule.RemoveCard(dir, card)
				if err != nil {
					return
				}
			}
			target := decisionItem.Data.(map[string]interface{})["target"].(int)
			rule.CardPlayer[dir].AddAnGang(target, card)
		}
		rule.DecisionClose(CLOSEGANG)
	}
}

//暗杠处理
func (rule *LZMJRule) OnAnGang(msg *msg.AnGang) {
	rule.DecisionClose(CLOSEANGANG)
}

//胡牌处理
func (rule *LZMJRule) OnHu(msg *msg.HuPai) {
	rule.DecisionClose(CLOSENORMAL)
}

//======================================功能部分=====================================================
//洗牌
func (rule *LZMJRule) RandCardData(arr []int, n int) {
	if n <= 0 {
		return
	}
	rule.RandCardData(arr, n-1)
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	ra := r.Intn(n + 1)
	arr[n], arr[ra] = arr[ra], arr[n]
}

//获取下一个玩家,参数为当前决策玩家的方位
func (rule *LZMJRule) GetNextPlayer(dir int) int {
	return dir%4 + 1
}

//获取下一个玩家，参数为当前决策玩家的playerID
func (rule *LZMJRule) GetNextPlayerByID(playerID int) int {
	dir, err := rule.GetDir(playerID)
	if err != nil {
		return 0
	}
	return dir%4 + 1
}

//获取一张牌，从剩余牌中获取一张
func (rule *LZMJRule) GetOneCard() int {
	if len(rule.Pai) < 1 {
		fmt.Println("GameOver")
		return 0
	}
	temp := rule.Pai[0]
	rule.Pai = append(rule.Pai[1:])
	return temp
}

//获取一张指定的牌
func (rule *LZMJRule) GetCard(card int) (int, error) {
	if len(rule.Pai) < 1 {
		fmt.Println("GameOver")
		return 0, nil
	}
	var temp int
	for i := 0; i < len(rule.Pai); i++ {
		if card == rule.Pai[i] {
			temp = card
			rule.Pai = append(rule.Pai[:i], rule.Pai[i+1:]...)
			return temp, nil
		}
	}
	return temp, errors.New("Card not found")
}

//给某个方位的玩家发指定的牌
func (rule *LZMJRule) DispatchFixCards(cards []int, fw int) error {
	for _, v := range cards {
		card, err := rule.GetCard(v)
		if err != nil {
			panic(err)
		}
		rule.CardPlayer[fw].InHand = append(rule.CardPlayer[fw].InHand, card)
	}
	return nil
}

//给某个方位的玩家发牌
func (rule *LZMJRule) DispatchCardToPlayer(fw int) (int, error) {
	if fw < 1 || fw > 4 {
		return 0, errors.New("方位出错，必须为 1-4")
	}
	newCard := rule.GetOneCard()
	rule.CardPlayer[fw].InHand = append(rule.CardPlayer[fw].InHand, newCard)
	return newCard, nil
}

//吃碰胡计算
func (rule *LZMJRule) GoCal(inhand []int) ([]CalResult, error) {
	var buf bytes.Buffer
	request := &pb.MajongCal{}
	//添加玩法
	request.Rule = "LZMJ"
	//添加摸打牌
	buf.WriteString(strconv.Itoa(rule.CurrentCard.Card))
	buf.WriteString("_")
	buf.WriteString(strconv.Itoa(rule.CurrentCard.Type))
	buf.WriteString("_")
	buf.WriteString(strconv.Itoa(rule.CurrentCard.Target))
	request.NewCard = buf.String()
	//添加牌
	for index := 1; index < 5; index++ {
		buf.Reset()
		//手牌
		if index == rule.ActivePlayr {
			Len := len(inhand)
			for i, v := range inhand {
				buf.WriteString(strconv.Itoa(v))
				if i != Len {
					buf.WriteString("_")
				}
			}
		} else {
			Len := len(rule.CardPlayer[index].InHand)
			for i, v := range rule.CardPlayer[index].InHand {
				buf.WriteString(strconv.Itoa(v))
				if i != Len {
					buf.WriteString("_")
				}
			}
		}

		//暗杠
		buf.WriteString("*")
		Len := len(rule.CardPlayer[index].AnGang)
		for i, v := range rule.CardPlayer[index].AnGang {
			buf.WriteString(strconv.Itoa(v.TargetCard))
			if i != Len {
				buf.WriteString("_")
			}
		}
		//处理明杠
		buf.WriteString("*")
		Len = len(rule.CardPlayer[index].Gang)
		for i, v := range rule.CardPlayer[index].Gang {
			buf.WriteString(fmt.Sprintf("%02d", v.Target))
			buf.WriteString(fmt.Sprintf("%02d", v.TargetCard))
			if i != Len {
				buf.WriteString("_")
			}
		}
		//处理碰牌
		buf.WriteString("*")
		Len = len(rule.CardPlayer[index].Peng)
		for i, v := range rule.CardPlayer[index].Peng {
			buf.WriteString(fmt.Sprintf("%02d", v.Target))
			buf.WriteString(fmt.Sprintf("%02d", v.TargetCard))
			if i != Len {
				buf.WriteString("_")
			}
		}
		//处理吃牌
		buf.WriteString("*")
		Len = len(rule.CardPlayer[index].Chi)
		for i, v := range rule.CardPlayer[index].Chi {
			for _, c := range v.Cards {
				buf.WriteString(fmt.Sprintf("%02d", c))
			}
			if i != Len {
				buf.WriteString("_")
			}
		}
		request.PlayerCard = append(request.PlayerCard, buf.String())
	}
	fmt.Println("吃碰胡计算：玩家牌：", request.PlayerCard)
	res, err := CalRPC.ChanRPC.Call1("LZMJCal", request)
	if err != nil {
		panic(err)
		return nil, err
	}
	var result []CalResult
	resStr := res.([]string)
	fmt.Println("计算结果", resStr)
	for m := 0; m < len(resStr); m++ {
		r := CalResult{}
		err := json.Unmarshal([]byte(resStr[m]), &r)
		if err != nil {
			panic(err)
			return nil, err
		}
		result = append(result, r)
	}
	return result, nil
}

//获取玩家牌的字符串
func (rule *LZMJRule) GetPlayerCardStr(dir int) string {
	var buf bytes.Buffer
	cards := rule.CardPlayer[dir].InHand
	Len := len(cards)
	for i, v := range cards {
		buf.WriteString("10")
		buf.WriteString(fmt.Sprintf("%02d", v))
		if i != Len {
			buf.WriteString("_")
		}
	}
	return buf.String()
}

//去除玩家手中的某张牌
func (rule *LZMJRule) RemoveCard(dir int, card int) error {
	cards := rule.CardPlayer[dir].InHand
	var index int = -1
	for i := 0; i < len(cards); i++ {
		if cards[i] == card {
			index = i
			break
		}
	}
	if index == -1 {
		return errors.New("no this card")
	}
	rule.CardPlayer[dir].InHand = append(cards[:index], cards[index+1:]...)
	return nil
}

//================================决策机==========================================================
func (rule *LZMJRule) DecisionClose(closeType int) {

	switch closeType {
	case CLOSEPENG:
		break
	case CLOSECHI:
		break
	case CLOSEGANG:
		rule.MoPai(rule.ActivePlayr, MOG)
		break
	case CLOSEANGANG:
		break
	case CLOSENORMAL:
		if rule.CurrentCard.Type == MO || rule.CurrentCard.Type == MOG {

		} else {
			rule.MoPai(rule.GetNextPlayer(rule.ActivePlayr), MO)
		}
		break
	}
	rule.Decision.IsOpen = false
	rule.Decision.Result = nil
	rule.Decision.StartPlayer = 0

}
func (rule *LZMJRule) DecisionOpen(startPlayer int, calres []CalResult) {
	rule.Decision.IsOpen = true
	rule.Decision.Result = calres
	rule.Decision.StartPlayer = startPlayer
	rule.Decision.WaitingDecison = []DecisonItem{}
}
func (rule *LZMJRule) DecisionExcute() {
	fmt.Println("=========执行决策")
	d := &rule.Decision
	if d.IsOpen == false {
		return
	}
	decisionTemp := make([]DecisonItem, len(d.WaitingDecison))
	close := true
	for _, v := range d.WaitingDecison {
		if v.Action == INIT || v.Action == WAITING {
			close = false
		}
	}
	if close {
		rule.DecisionClose(CLOSENORMAL)
		return
	}
	copy(decisionTemp, d.WaitingDecison)
	for i := 0; i < len(decisionTemp); i++ {
		if d.WaitingDecison[i].Action == INIT {
			playerID := rule.GetPlayerIDByDir(decisionTemp[i].Dir)
			rule.room.BroadcastOne(decisionTemp[i].BroadcastEvent, playerID, decisionTemp[i].Data)
			d.WaitingDecison[i].Action = WAITING
		}
	}

}
func (rule *LZMJRule) DecisionThink() bool {
	fmt.Println("=========进入决策思考")
	d := &rule.Decision
	if d.IsOpen == false {
		return false
	}
	//处理胡
	countHu := 0
	HUDir := 0
	for i := 0; i < 4; i++ {
		dir := i + 1
		if d.Result[i].Hu != "" {
			huRes := strings.Split(d.Result[i].Hu, ",")
			Map := map[string]interface{}{}
			Map["pid"] = rule.GetPlayerIDByDir(dir)
			Map["message"] = "10" + rest.Substr(huRes[0], 2, 2)
			if rule.CurrentCard.Type == MO || rule.CurrentCard.Type == MOG {
				Map["order"] = "zimohu"
			} else {
				Map["order"] = "dianpaohu"
			}
			Map["position"] = dir
			d.AddDecison(dir, CHU, INIT, "hashu", Map)
			d.Result[i].Hu = ""
			countHu++
			HUDir = dir
		}
	}
	//处理碰杠暗杆
	PGDir := 0
	if countHu < 2 {
		for i := 0; i < 4; i++ {
			dir := i + 1
			if d.Result[i].AG != "" {
				Map := map[string]interface{}{}
				Map["pid"] = rule.GetPlayerIDByDir(dir)
				Map["message"] = "10" + d.Result[i].AG
				Map["position"] = dir
				Map["order"] = "angang"
				Map["target"] = dir
				d.AddDecison(dir, CPENGGANG, INIT, "hasgan", Map)
				d.Result[i].AG = ""
				PGDir = dir
			}
			if d.Result[i].G != "" {
				Map := map[string]interface{}{}
				Map["pid"] = rule.GetPlayerIDByDir(dir)
				Map["message"] = "10" + rest.Substr(d.Result[i].G, 2, 2)
				target, err := strconv.Atoi(rest.Substr(d.Result[i].G, 0, 2))
				if err != nil {
					panic(err)
					return false
				}
				if rule.CurrentCard.Type == MO || rule.CurrentCard.Type == MOG {
					Map["order"] = "minggang2"
				} else {
					Map["order"] = "minggang1"
				}
				Map["target"] = target
				Map["position"] = dir
				d.AddDecison(dir, CPENGGANG, INIT, "hasgan", Map)
				d.Result[i].G = ""
				PGDir = dir
			}

			if d.Result[i].P != "" {
				Map := map[string]interface{}{}
				Map["pid"] = rule.GetPlayerIDByDir(dir)
				Map["message"] = "10" + rest.Substr(d.Result[i].P, 2, 2)
				target, _ := strconv.Atoi(rest.Substr(d.Result[i].P, 0, 2))
				Map["target"] = target
				Map["position"] = dir
				d.AddDecison(dir, CPENGGANG, INIT, "haspeng", Map)
				d.Result[i].P = ""
				PGDir = dir
			}

		}
	}
	if countHu < 2 {
		for i := 0; i < 4; i++ {
			dir := i + 1
			if d.Result[i].C != "" && (PGDir == dir || PGDir == 0) && (HUDir == 0 || HUDir == dir) {
				Map := map[string]interface{}{}
				Map["pid"] = rule.GetPlayerIDByDir(dir)
				Map["message"] = d.Result[i].C
				Map["position"] = dir
				//d.AddDecison(dir, CCHI, INIT, "haschi", Map)
				d.Result[i].C = ""
			}
		}
	}
	rule.DecisionExcute()
	return true
}

//================================规则无关==========================================================

//创建16位无符号的整型
func MAKEWORD(low, high uint8) uint32 {
	var ret uint16 = uint16(high)<<8 + uint16(low)
	return uint32(ret)
}
