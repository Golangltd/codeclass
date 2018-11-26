package internal

import (
	"GameServer/conf"
	gamedata "GameServer/gamedata/constDefine"
	"GameServer/modules/game"
	"GameServer/modules/login"
	"GameServer/rest"
	"encoding/json"
	"fmt"
	"io/ioutil"
)

var Router, err = rest.MakeRouter(
	rest.Post("/login", Login), //登录
	rest.Post("/Main", Main),   //主界面相关
	rest.Get("/CMD", CMD),      //命令
	rest.Post("/Room", Room),   //游戏房间中相关
	rest.Post("/KEFU", Kefu),   //客服相关处理
	rest.Post("/Game", Game),   //游戏相关处理
)

//登录逻辑处理函数
func Login(w rest.ResponseWriter, r *rest.Request) {

	end := make(chan bool)
	//获取事件
	Event := r.URL.Query().Get("Event")
	fmt.Println("路由至： Login处理:" + Event)
	//分发消息
	switch Event {
	case "weixinLogin":
		login.ChanRPC.Go("OnWeiXinLogin", w, r, end)
	default:
		w.WriteJson(map[string]interface{}{"Event": Event, "content": map[string]string{"Result": "Error,No suitable handler"}})
	}
	<-end
}

//主界面http交互
func Main(w rest.ResponseWriter, r *rest.Request) {
	//获取事件
	Event := r.URL.Query().Get("Event")
	fmt.Println("路由至： 主界面处理：" + Event)
	//获取用户id
	//userID := r.URL.Query().Get("User")
	//检查该消息客户端是否登录
	//暂时不验证
	/*ok := gamedata.CheckOnline(userID)
	if ok.(bool) == false {
		w.WriteJson(`{"Error":"Not logged in"}`)
		return
	}*/
	end := make(chan bool)
	//分发消息
	switch Event {
	case "GongGao":
		ChanRPC.Go("GongGao", w, r, end)
	case "RuleInstructions":
		ChanRPC.Go("RuleInstructions", w, r, end)
	case "AskFor":
		ChanRPC.Go("AskFor", w, r, end)
	default:
		w.WriteJson(map[string]interface{}{"Event": Event, "content": map[string]string{"Result": "Error,No suitable handler"}})
	}
	<-end

}

//交互命令处理函数
func CMD(w rest.ResponseWriter, r *rest.Request) {
	//获取CMD类型
	//eg:http://127.0.0.1:7777/CMD?CMD=updateConst&Psd=1992424
	cmd := r.URL.Query().Get("CMD")
	psd := r.URL.Query().Get("Psd")
	if psd != conf.Server.CMDpassword {
		w.WriteJson(map[string]string{"Result": "Error,password is wrong!"})
	}
	switch cmd {
	case "updateConst":
		go gamedata.ReadConstDefine()
		w.WriteJson(map[string]string{"Result": "Refresh the ConstDefine suceess!"})
	default:
		w.WriteJson(map[string]string{"Result": "Error,Not hava this command!"})
	}
}

//游戏房间中的相关处理
func Room(w rest.ResponseWriter, r *rest.Request) {
	end := make(chan bool)
	//获取事件
	Event := r.URL.Query().Get("Event")
	fmt.Println("路由至： Room处理:" + Event)
	//分发消息
	switch Event {
	case "UploadVoice":
		//TODO 语言转角到对应的房间处理模块
		//login.ChanRPC.Go("OnWeiXinLogin", w, r, end)
	default:
		w.WriteJson(map[string]interface{}{"Event": Event, "content": map[string]string{"Result": "Error,No suitable handler"}})
	}
	<-end
}

//客服相关处理
func Kefu(w rest.ResponseWriter, r *rest.Request) {
	//TODO 处理主界面客服相关类容
	end := make(chan bool)
	//获取事件
	Event := r.URL.Query().Get("Event")
	fmt.Println("路由至： Kefu处理:" + Event)
	//分发消息
	switch Event {
	case "KefuRequest":
		//TODO 转到请求客服模块
	case "KefuSend":
		//TODO 客户端发送信息到客服
	default:
		w.WriteJson(map[string]interface{}{"Event": Event, "content": map[string]string{"Result": "Error,No suitable handler"}})
	}
	<-end
}

//游戏相关http处理
func Game(w rest.ResponseWriter, r *rest.Request) {
	//TODO 处理主界面客服相关类容
	end := make(chan bool)
	//获取事件
	Event := r.URL.Query().Get("Event")
	fmt.Println("路由至： 游戏相关处理：" + Event)
	//分发消息
	switch Event {
	case "GameEnterRoom":
		//转到Game模块
		game.ChanRPC.Go("GameEnterRoom", w, r, end)
	case "GameRecordList":
		//TODO 转到Game模块
	case "GameTypeSelect":
		//转到游戏房间创建
		game.ChanRPC.Go("GameTypeSelect", w, r, end)
	default:
		w.WriteJson(map[string]interface{}{"Event": Event, "content": map[string]string{"Result": "Error,No suitable handler"}})
	}
	<-end
}

//测试方法
func test(w rest.ResponseWriter, r *rest.Request) {
	Event := r.URL.Query().Get("a")
	result, _ := ioutil.ReadAll(r.Body)
	fmt.Println(Event)
	r.ParseForm()
	r.ParseMultipartForm(32 << 20)
	var Map map[string]string
	json.Unmarshal(result, &Map)
	res, err := rest.Safe_Base64Decode(Map["weixininfo"])
	if err != nil {
		fmt.Println(err)
	}
	ress, iserr := rest.AtoU(res)
	if iserr != "" {
		fmt.Println(iserr)
	}
	fmt.Println(ress)
	fmt.Println(string(result))
	_ = Event
	w.WriteJson(r.PostForm)
}
