package internal

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math/rand"
	"reflect"
	"time"

	data "GameServer/gamedata"
	"GameServer/rest"
	"strconv"
	"strings"
)

func handleMsg(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

func init() {
	skeleton.RegisterChanRPC("OnWeiXinLogin", OnWeiXinLogin)
}

func OnWeiXinLogin(args []interface{}) {
	//TODO 查询数据库，检查用户是否注册
	r := args[1].(*rest.Request)
	end := args[2].(chan bool)
	w := args[0].(rest.ResponseWriter)

	//获取用户id
	userID := r.URL.Query().Get("User")
	if userID == "" {
		(w).WriteJson(map[string]bool{"LoginResult": false})
	}

	var player *data.PlayerInfo

	uid, ok := data.GetUID(userID)
	if ok {
		//检查该消息客户端是否登录
		p, ok := data.CheckOnline(uid)
		if ok == true {
			player = p.Playerinfo
		} else {
			w.WriteJson(map[string]interface{}{"LoginResult": false, "body": "{}"})
		}

	} else {
		//检查是否注册
		ok, err := data.CheckRegister(userID)
		if err != nil {
			panic(err)
		}

		if ok {
			//获取用户相关信息
			player, err = data.GetPlayerInfoByID(userID)
			if err != nil {
				panic(err)
			}
		} else {
			//新建用户，初始化相关信息
			newPlayer := data.PlayerInfo{}
			newPlayer.ID = userID
			uid, err := GetNewUID()
			if err != nil {
				w.WriteJson(map[string]interface{}{"LoginResult": false, "body": "{}"})
				end <- true
				return
			}
			newPlayer.UID = uid
			newPlayer.FK = 10
			regOk, regErr := data.AccountRegister(newPlayer)
			if regErr != nil {
				w.WriteJson(map[string]interface{}{"LoginResult": false, "body": "{}"})
				end <- true
				return
			}
			if regOk == true {
				player = &newPlayer
			} else {
				w.WriteJson(map[string]interface{}{"LoginResult": false, "body": "{}"})
				end <- true
				return
			}
		}
	}

	//更新玩家的动态信息
	result, _ := ioutil.ReadAll(r.Body)
	r.ParseForm()
	r.ParseMultipartForm(32 << 20)
	var Map map[string]string
	json.Unmarshal(result, &Map)
	res, err := rest.Safe_Base64Decode(Map["weixininfo"])
	if err != nil {
		fmt.Println(err)
		w.WriteJson(map[string]interface{}{"LoginResult": false, "body": "{}"})
		end <- true
		return
	}
	ress, iserr := rest.AtoU(res)
	if iserr != "" {
		fmt.Println(iserr)
	}
	fmt.Println("收到登录信息：" + ress)
	var MapUser map[string]string
	json.Unmarshal([]byte(ress), &MapUser)

	//昵称
	player.Name = MapUser["nikename"]
	//性别
	sex, _ := strconv.Atoi(MapUser["sex"])
	player.Sex = sex
	//头像网址
	player.TXUrl = MapUser["headimgurl"]
	//客户端IP
	addr := r.RemoteAddr
	ip := strings.Split(addr, ":")[0]
	player.IP = ip
	w.WriteJson(map[string]interface{}{"LoginResult": true, "body": player})

	//获取到的玩家信息交到在线玩家
	data.NewOnlinePlayer(player, nil)
	end <- true
}

//------------------------------------------------------------------------------
//生成游戏数字ID
func GetNewUID() (int, error) {
	count := 0
	rnd := rand.New(rand.NewSource(time.Now().UnixNano()))
	num, err := strconv.Atoi(fmt.Sprintf("%06v", rnd.Int31n(100000000-10000000)+10000000))
	if err != nil {
		fmt.Println("创建房间号，生成随机码出错！")
	}
A:
	count++
	if ok, _ := data.CheckUID(num); ok {
		num++
		if count > 100 {
			fmt.Println("创建房间号，生成随机码出错！")
			return 0, nil
		}
		if num > 999999 || count > 10 {
			count = 0
			num, err = strconv.Atoi(fmt.Sprintf("%06v", rnd.Int31n(100000000-10000000)+10000000))
			goto A
		}
		goto A
	}
	return num, nil
}
