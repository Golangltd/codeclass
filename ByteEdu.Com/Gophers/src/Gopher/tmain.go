package main

import (
	"Proto_Go"
	"fmt"
	"net/http"
	"strconv"
)

// http://127.0.0.1:8891/ByteEdu_Gophers?Protocol=1&Protocol2=1

var (
	strport string = "8891"
)

func main() {

	http.HandleFunc("/ByteEdu_Gophers", IndexHandler)
	http.ListenAndServe(":"+strport, nil)
}

func IndexHandler(w http.ResponseWriter, req *http.Request) {

	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		defer func() { // 必须要先声明defer，否则不能捕获到panic异常
			if err := recover(); err != nil {
				fmt.Println("%s", err)
				req.Body.Close()
			}
		}()

		Protocol, bProtocol := req.Form["Protocol"]
		Protocol2, bProtocol2 := req.Form["Protocol2"]

		if bProtocol && bProtocol2 {
			// 主协议判断
			if Protocol[0] == strconv.Itoa(Proto.ProtoGopher) {
				// 子协议判断
				switch Protocol2[0] {
				case strconv.Itoa(Proto.C2S_PlayerLoginProto2):
					// data := DB_rpc_()
					// b, _ := json.Marshal(data)
					// fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
					fmt.Fprintln(w, "登陆成功")
					//------------------------------------------------------
					return
				case strconv.Itoa(Proto.C2S_GetRankProto2):
					// 获取排行
					return
				default:
					fmt.Fprintln(w, "88902")
					return
				}
			}
			fmt.Fprintln(w, "88904")
			return
		}
		// 服务器获取通信方式错误 --> 8890 + 1
		fmt.Fprintln(w, "88901")
		return
	}
}
