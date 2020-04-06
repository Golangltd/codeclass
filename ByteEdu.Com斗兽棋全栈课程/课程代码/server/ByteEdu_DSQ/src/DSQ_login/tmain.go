package main

import (
	"DSQ_login/Proto"
	"DSQ_server/Player"
	"LollipopGo/LollipopGo/util"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
)

/*
   验证码生成规则
   1. DeviceID 生成
   2. DeviceID  MD5 ok
*/

var G_Player_Tocken map[string]string

func init() {
	G_Player_Tocken = make(map[string]string)
}

func main() {
	strport := "4001"
	http.HandleFunc("/BaBaLiuLiu_Server", Server_Func) // 服务器验证  -- game_server 拿到数据后（tocken） 去我们的login_server验证 ，tocken是否有效
	http.HandleFunc("/BaBaLiuLiu_DSQ", Client_Func)    // 客户端验证  -- 客户端到login 获取我们的【tocken】 数据 + 【game_server的链接信息（ip+port）】
	http.ListenAndServe(":"+strport, nil)
}

// 服务器验证流程
func Server_Func(w http.ResponseWriter, req *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	req.Header.Add("content-type", "charset=UTF-8")
	req.ParseForm()
	defer func() {
		if err := recover(); err != nil {
			fmt.Println("%s", err)
			req.Body.Close()
		}
	}()
	// 逻辑处理
	//{
	fmt.Println("-----------服务器验证流程")
	//}
	if req.Method == "GET" {
		Protocol, bProtocol := req.Form["Protocol"]
		Protocol2, bProtocol2 := req.Form["Protocol2"]
		if bProtocol && bProtocol2 {
			fmt.Println("--------Protocol", Protocol)
			fmt.Println("--------Protocol2", Protocol2)
			Token, _ := req.Form["Token"] // ? 这里不够严谨
			fmt.Println("--------Token", Token)
			_, ok := G_Player_Tocken[Token[0]]
			if ok {

				data := ProtoDSQ.L2G_Player_Login{
					Protocol:  10,
					Protocol2: 2,
					Isucc:     true,
				}
				dataplayer := &Player_DSQ.PlayerData{
					UID:    len(G_Player_Tocken) + 1,
					OpenID: util.MD5_LollipopGO(strconv.Itoa(len(G_Player_Tocken) + 1)),
					Name:   "guest_" + strconv.Itoa(len(G_Player_Tocken)+1),
					Avatar: "1",
					Level:  1,
					Coin:   1000,
				}
				data.PlayerData = dataplayer
				datamsg, _ := json.Marshal(data)
				fmt.Fprintf(w, "%s", datamsg)
				return
			}
			data := ProtoDSQ.L2G_Player_Login{
				Protocol:  10,
				Protocol2: 2,
				Isucc:     false,
			}
			datamsg, _ := json.Marshal(data)
			fmt.Fprintf(w, "%s", datamsg)
			return
		}
	}
	//  LollipopGO_Error:
	//	fmt.Println("参数错误")
}

// 客户端验证流程
func Client_Func(w http.ResponseWriter, req *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	req.Header.Add("content-type", "charset=UTF-8")
	req.ParseForm()
	defer func() {
		if err := recover(); err != nil {
			fmt.Println("%s", err)
			req.Body.Close()
		}
	}()
	// 逻辑处理
	//{
	fmt.Println("-----------客户端验证流程")
	//}
	if req.Method == "GET" {
		Protocol, bProtocol := req.Form["Protocol"]
		Protocol2, bProtocol2 := req.Form["Protocol2"]
		if bProtocol && bProtocol2 {
			fmt.Println("--------Protocol", Protocol)
			fmt.Println("--------Protocol2", Protocol2)
			if Protocol[0] == "10" {
				// 功能的逻辑操作
				if Protocol2[0] == "1" {
					// 游客登录
					DeviceID, bDeviceID := req.Form["DeviceID"]
					if bDeviceID {
						tocken := util.MD5_LollipopGO(DeviceID[0])
						G_Player_Tocken[tocken] = tocken
						URL := "192.168.0.103:4002"
						data := ProtoDSQ.L2C_Player_Login{
							Protocol:  10,
							Protocol2: 2,
							Tocken:    tocken, // 登录验证码
							URL:       URL,    // 游戏服务器的地址
						}
						datamsg, _ := json.Marshal(data)
						fmt.Fprintf(w, "%s", datamsg)
						return
					} else {
						goto LollipopGO_Error
					}
				}
			} else {
				fmt.Println("主协议不存在")
				fmt.Fprintf(w, "%s", "主协议不存在")
				return
			}
		}
		return
	} else {
		return
	}
LollipopGO_Error:
	fmt.Println("参数错误")
}
