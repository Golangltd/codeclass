package main

import (
	"Proto_Go"
	"fmt"
	"net/http"
	"strconv"
)

// http://127.0.0.1:8891/ByteEdu_Gophers?Protocol=1&Protocol2=1

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
	strport    string = "8891"
	PlayerData map[string]*PlayerData
)

func init() {
	// 初始化 map
	PlayerData = make(map[string]*PlayerData)
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

		if bProtocol && bProtocol2 {
			if Protocol[0] == strconv.Itoa(Proto.ProtoGopher) {
				switch Protocol2[0] {
				case strconv.Itoa(Proto.C2S_PlayerLoginProto2):
					// data := DB_rpc_()
					// b, _ := json.Marshal(data)
					// fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
					fmt.Fprintln(w, "登陆成功")
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
