package main

import (
	"Proto_Go"
	"fmt"
	"net/http"
	"strconv"
)

// http://127.0.0.1:8891/ByteEdu_Gophers?Protocol=1&Protocol2=1&Itype=1&LoginName=ByteEdu.Com&LoginPW=ByteEdu.Com

//------------------------------------------------------------------------------
// 保存玩家登陆、注册的协议
type PlayerData struct {
	OpenID    string
	LoginName string
	LoginPW   string
	Lev       string // 职位
}

//------------------------------------------------------------------------------

var (
	strport     string = "8891"
	PlayerDataG map[string]*PlayerData
)

func init() {
	// 初始化 map
	PlayerDataG = make(map[string]*PlayerData)
	return
}

func main() {

	http.HandleFunc("/ByteEdu_Gophers", IndexHandler)
	http.ListenAndServe(":"+strport, nil)
}

func IndexHandler(w http.ResponseWriter, req *http.Request) {

	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		defer func() {
			if err := recover(); err != nil {
				fmt.Println("%s", err)
				req.Body.Close()
				return
			}
		}()

		Protocol, bProtocol := req.Form["Protocol"]
		Protocol2, bProtocol2 := req.Form["Protocol2"]
		Itype, bItype := req.Form["Itype"]

		if bProtocol && bProtocol2 && bItype {
			if Protocol[0] == strconv.Itoa(Proto.ProtoGopher) {
				switch Protocol2[0] {
				case strconv.Itoa(Proto.C2S_PlayerLoginProto2):

					LoginName, _ := req.Form["LoginName"]
					LoginPW, _ := req.Form["LoginPW"]
					strItype := Itype[0]
					if strItype == "1" { // 注册
						//1. 用户名要脏字过滤
						//2. 判断用户是否注册过
						_, ok := PlayerDataG[LoginName[0]]
						if ok {
							fmt.Fprintln(w, "已经注册过！")
							return
						} else {
							data := &PlayerData{
								OpenID:    LoginName[0],
								LoginName: LoginName[0],
								LoginPW:   LoginPW[0],
								Lev:       "军师", // 职位
							}
							// 保存
							PlayerDataG[LoginName[0]] = data
							fmt.Fprintln(w, "注册成功")
							return
						}
						return
					} else if strItype == "2" { // 登陆

						return
					}

					fmt.Fprintln(w, "类型错误")
					//------------------------------------------------------
					return
				case strconv.Itoa(Proto.C2S_GetRankProto2):
					return
				default:
					fmt.Fprintln(w, "88902")
					return
				}
			}
			fmt.Fprintln(w, "88904")
			return
		}
		fmt.Fprintln(w, "88901")
		return
	}
}
