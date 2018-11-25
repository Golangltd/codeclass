package main

//import (
//	"encoding/base64"
//	"encoding/json"
//	"fmt"
//	"glog-master"
//	"net/http"
//	//	"sort"
//	"strconv"
//	//	"strings"
//	//	"time"
//)

//// 测试协议
//type ST_test struct {
//	Id   int
//	Name string
//	Addr string
//	JuLi int
//}

////  玩家数据获取
//func TJWanJiaData(w http.ResponseWriter, req *http.Request) {
//	glog.Info("httpTask is running...")

//	// Get方法
//	if req.Method == "POST" {
//		w.Header().Set("Access-Control-Allow-Origin", "*")
//		req.ParseForm()
//		// 获取函数
//		Protocol, bProtocol := req.Form["Protocol"]
//		Protocol2, bProtocol2 := req.Form["Protocol2"]
//		defer req.Body.Close()
//		if bProtocol && bProtocol2 {
//			// 主协议判断
//			if Protocol[0] == strconv.Itoa(12) {

//				switch Protocol2[0] {
//				case strconv.Itoa(1):
//					{ // 新增欢乐豆协议

//						//				     ['Id']+'</span>'
//						//                   ['Name']+'</div>'
//						//                   ['Addr']+'</div>'
//						dad1 := make(map[string]*ST_test)
//						dad := new(ST_test)
//						dad.Id = 1000
//						dad.Name = "test"
//						dad.Addr = "sss"
//						dad.JuLi = 2000
//						dad1["1"] = dad

//						dadbak := new(ST_test)
//						dadbak.Id = 2000
//						dadbak.Name = "2test"
//						dadbak.Addr = "2sss"
//						dadbak.JuLi = 3000
//						dad1["2"] = dadbak
//						b, _ := json.Marshal(dad1)
//						fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
//						return
//					}
//				default:
//					{
//						return
//					}
//				}
//			}

//		}
//		fmt.Fprint(w, base64.StdEncoding.EncodeToString([]byte("协议不对！！！")))
//		return
//	}
//	fmt.Fprint(w, base64.StdEncoding.EncodeToString([]byte("非post方式！！！")))
//	return
//}
