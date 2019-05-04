package main

import (
	"Proto_Go"
	"fmt"
	"net/http"
	"sort"
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
	Score     int
}

type ByteEduList []PlayerData

func (this ByteEduList) Len() int {
	return len(this)
}

func (this ByteEduList) Less(i, j int) bool {
	return this[i].Score < this[j].Score
}

func (this ByteEduList) Swap(i, j int) {
	this[i], this[j] = this[j], this[i]
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
						_, ok := PlayerDataG[LoginName[0]]
						if ok {
							fmt.Fprintln(w, "登陆成功！")

							return
						} else {
							fmt.Fprintln(w, "用户名或密码错误！")
							return
						}
					}

					fmt.Fprintln(w, "类型错误")
					//------------------------------------------------------
					return
				case strconv.Itoa(Proto.C2S_GetRankProto2):
					// 排行协议
					LoginName, bLoginName := req.Form["LoginName"]
					Score, bScore := req.Form["Score"]
					if bLoginName && bScore {
						v, ok := PlayerDataG[LoginName[0]]
						iscore, _ := strconv.Atoi(Score[0])
						// 一：数据的保存
						if ok {
							// 1. 保存的数据是否小于当前数据
							if v.Score < iscore {
								data := &PlayerData{
									OpenID:    LoginName[0],
									LoginName: LoginName[0],
									LoginPW:   v.LoginPW,
									Lev:       v.Lev, // 职位
									Score:     iscore,
								}
								// 保存
								PlayerDataG[LoginName[0]] = data
							}
						} else {
							// 内存不存在的情况下保存
							data := &PlayerData{
								OpenID:    LoginName[0],
								LoginName: LoginName[0],
								Lev:       "军师",
								Score:     iscore,
							}
							// 保存
							PlayerDataG[LoginName[0]] = data
						}
						// 二：排行
						// 1. map ---> slice
						playerdata := make([]PlayerData, len(PlayerDataG)+1)
						i := 0
						for _, v := range PlayerDataG {
							i++
							var data PlayerData
							data.OpenID = v.OpenID
							data.LoginName = v.LoginName
							data.LoginPW = v.LoginPW
							data.Lev = v.Lev
							data.Score = v.Score
							playerdata[i] = data
						}
						//2. Sort
						sort.Sort(ByteEduList(playerdata))

						// 三：返回数据给客户端
						fmt.Fprintln(w, playerdata)
						return
					}
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
