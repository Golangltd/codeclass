package main

import (
	"Proto"
	"fmt"
	"net/http"
)

var (
	strport string = "8891"
)

func init() {

}

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
			if Protocol[0] == strconv.Itoa(Proto.G_GameLogin_Proto) {
				// 子协议判断
				switch Protocol2[0] {
				case strconv.Itoa(Proto2.C2GL_GameLoginProto2):
					// data := DB_rpc_()
					// b, _ := json.Marshal(data)
					// fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
					//------------------------------------------------------
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
