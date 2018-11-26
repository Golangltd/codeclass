package internal

import (
	"GameServer/gamedata"
	"GameServer/leaf/gate"
	"GameServer/logic"
	"GameServer/logic/LZMJ"
	"GameServer/msg"
	"GameServer/rest"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"reflect"
	"strconv"
)

func init() {
	skeleton.RegisterChanRPC("GameTypeSelect", GameTypeSelect)
	skeleton.RegisterChanRPC("GameEnterRoom", GameEnterRoom)
	handler(&msg.RoomCreate{}, RoomCreate)
	handler(&msg.RoomEnter{}, RoomEnter)
	handler(&msg.ClientClose{}, ClientClose)
	handler(&msg.RoomExit{}, RoomExit)
	//麻将指令
	handler(&msg.ChuPai{}, OnChuPai)
	handler(&msg.Peng{}, OnPengPai)
	handler(&msg.ChiPai{}, OnChiPai)
	handler(&msg.Gang{}, OnGangPai)
	handler(&msg.AnGang{}, OnAnGang)
	handler(&msg.HuPai{}, OnHuPai)
}

func handler(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

//创建房间
func GameTypeSelect(args []interface{}) {
	w := args[0].(rest.ResponseWriter)
	r := args[1].(*rest.Request)
	end := args[2].(chan bool)
	Event := r.URL.Query().Get("Event")

	result, _ := ioutil.ReadAll(r.Body)
	r.ParseForm()
	r.ParseMultipartForm(32 << 20)
	var Map map[string]string
	json.Unmarshal(result, &Map)
	var room *logic.Room
	switch Map["GameType"] {
	case "LZMJ":
		var err error
		gameRule := LZMJ.LZMJRule{}
		//设置房间总局数
		num, err := strconv.Atoi(Map["jushu"])
		if err != nil {
			w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"Error": "Create room faile!d"}})
			end <- true
			return
		}
		gameRule.NumTotal = num
		//设着房间钓鱼数
		yunum, err := strconv.Atoi(Map["ext1"])
		if err != nil {
			w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"Error": "Create room faile!d"}})
			end <- true
			return
		}
		gameRule.YuNum = yunum
		//设置钓鱼方式
		yuType, err := strconv.Atoi(Map["ext2"])
		if err != nil {
			w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"Error": "Create room faile!d"}})
			end <- true
			return
		}
		gameRule.YuType = yuType
		//设置房间最大人数
		numMax, err := strconv.Atoi(Map["maxnum"])
		if err != nil {
			w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"Error": "Create room faile!d"}})
			end <- true
			return
		}
		//uid
		uid, err := strconv.Atoi(Map["uid"])
		if err != nil {
			w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"Error": "Create room faile!d"}})
			end <- true
			return
		}
		room, err = logic.NewRoom(numMax, uid, &gameRule)
		if err != nil {
			w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"Error": "Create room faile!d"}})
			end <- true
			return
		}
	}
	w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]int{"roomid": room.RoomID, "isOK": 1}})
	end <- true
}

//进入房间
func GameEnterRoom(args []interface{}) {
	w := args[0].(rest.ResponseWriter)
	r := args[1].(*rest.Request)
	end := args[2].(chan bool)
	Event := r.URL.Query().Get("Event")

	result, _ := ioutil.ReadAll(r.Body)
	r.ParseForm()
	r.ParseMultipartForm(32 << 20)
	var Map map[string]string
	json.Unmarshal(result, &Map)
	roomid, err := strconv.Atoi(Map["roomid"])
	if err != nil {
		w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"isOK": "-1"}})
		end <- true
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"isOK": "-1"}})
		end <- true
		return
	}
	if room.RoomState == logic.ROOMEND {
		w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"isOK": "-1"}})
		end <- true
		return
	}
	w.WriteJson(map[string]interface{}{"Event": Event, "body": map[string]string{"isOK": "1"}})
	end <- true
}

//相应房间创建
func RoomCreate(args []interface{}) {
	m := args[0].(*msg.RoomCreate)
	a := args[1].(gate.Agent)
	uid, err := strconv.Atoi(m.UID)
	if err != nil {
		return
	}
	player, ok := gamedata.GetOnlinePlayerByID(uid)
	if ok {
		player.Agent = a
	}
	a.Emit("RoomCreate", map[string]bool{})
}

//处理进入房间
func RoomEnter(args []interface{}) {
	m := args[0].(*msg.RoomEnter)
	a := args[1].(gate.Agent)
	uid, err := strconv.Atoi(m.UID)
	if err != nil {
		return
	}
	player, ok := gamedata.GetOnlinePlayerByID(uid)
	if ok {
		player.Agent = a
	}
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		fmt.Println("Get room failed")
	}
	err = room.Enter(uid)
	if err != nil {
		fmt.Println("Enter room failed")
	}
}

//客户端关闭
func ClientClose(args []interface{}) {
	fmt.Println("有客户端关闭")
	m := args[0].(*msg.ClientClose)
	a := args[1].(gate.Agent)
	uid, err := strconv.Atoi(m.UID)
	if err != nil {
		return
	}
	player, ok := gamedata.GetOnlinePlayerByID(uid)
	if ok {
		player.Agent = a
	}
	if player.Room != 0 {
		room, err := logic.GetRoom(player.Room)
		if err != nil {
			return
		}
		room.Leave(uid)
	}
}

//玩家离开房间
func RoomExit(args []interface{}) {
	m := args[0].(*msg.RoomExit)
	//a := args[1].(gate.Agent)
	uid, err := strconv.Atoi(m.UID)
	if err != nil {
		return
	}
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		return
	}
	room.Leave(uid)
}

//麻将出牌
func OnChuPai(args []interface{}) {
	m := args[0].(*msg.ChuPai)
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		return
	}
	room.Message <- logic.RoomMessage{
		Name: "chupai",
		Body: m,
	}
}

//麻将碰牌
func OnPengPai(args []interface{}) {
	m := args[0].(*msg.Peng)
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		return
	}
	room.Message <- logic.RoomMessage{
		Name: "pengpai",
		Body: m,
	}
}

//麻将杠牌
func OnGangPai(args []interface{}) {
	m := args[0].(*msg.Gang)
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		return
	}
	room.Message <- logic.RoomMessage{
		Name: "gangpai",
		Body: m,
	}
}

//麻将暗杠牌
func OnAnGang(args []interface{}) {
	m := args[0].(*msg.AnGang)
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		return
	}
	room.Message <- logic.RoomMessage{
		//Name: "angang",
		Name: "gangpai",
		Body: m,
	}
}

//麻将吃牌
func OnChiPai(args []interface{}) {
	m := args[0].(*msg.ChiPai)
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		return
	}
	room.Message <- logic.RoomMessage{
		Name: "chipai",
		Body: m,
	}
}

//麻将胡牌
func OnHuPai(args []interface{}) {
	m := args[0].(*msg.HuPai)
	roomid, err := strconv.Atoi(m.RoomID)
	if err != nil {
		return
	}
	room, err := logic.GetRoom(roomid)
	if err != nil {
		return
	}
	room.Message <- logic.RoomMessage{
		Name: "hupai",
		Body: m,
	}
}
