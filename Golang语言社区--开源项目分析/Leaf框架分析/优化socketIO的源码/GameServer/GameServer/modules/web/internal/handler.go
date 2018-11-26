package internal

import (
	gamedata "GameServer/gamedata/constDefine"
	"GameServer/msg"
	"GameServer/rest"
	"reflect"
	"strconv"
)

var (
	Task  = make(chan msg.TaskMessage)
	Tasks = make(map[int][]interface{})
)

func init() {
	skeleton.RegisterChanRPC("GongGao", gongGao)
	skeleton.RegisterChanRPC("RuleInstructions", ruleInstructions)
	skeleton.RegisterChanRPC("AskFor", askFor)
	//开始任务处理，等待客户端轮询
	go handleTask()
}

func handler(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}

//------------------------------------------------------------------------------------
//客户端申请更新公告
func gongGao(args []interface{}) {
	w := args[0].(rest.ResponseWriter)
	r := args[1].(*rest.Request)
	end := args[2].(chan bool)
	Event := r.URL.Query().Get("Event")

	w.WriteJson(map[string]string{"Event": Event, "content": gamedata.ConstDefine["GongGao"]})
	end <- true
}

//客户端请求更新规则说明
func ruleInstructions(args []interface{}) {
	w := args[0].(rest.ResponseWriter)
	r := args[1].(*rest.Request)
	end := args[2].(chan bool)
	Event := r.URL.Query().Get("Event")

	w.WriteJson(map[string]string{"Event": Event, "content": gamedata.ConstDefine["RuleInstructions"]})
	end <- true
}

//处理http轮训
func askFor(args []interface{}) {
	w := args[0].(rest.ResponseWriter)
	r := args[1].(*rest.Request)
	end := args[2].(chan bool)
	Event := r.URL.Query().Get("Event")
	//获取用户id
	userID := r.URL.Query().Get("User")
	if userID == "" {
		(w).WriteJson(map[string]interface{}{Event: map[string]bool{"Error": true}})
	}
	uid, err := strconv.Atoi(userID)
	if err != nil {
		end <- true
		return
	}
	tasks, ok := Tasks[uid]
	if ok {
		w.WriteJson(map[string]interface{}{"Event": Event, "body": tasks})
		delete(Tasks, uid)
	}
	end <- true
}

//task处理
func handleTask() {
	for {
		select {
		case task := <-Task:
			Tasks[task.UID] = append(Tasks[task.UID], task)
		}
	}
}
