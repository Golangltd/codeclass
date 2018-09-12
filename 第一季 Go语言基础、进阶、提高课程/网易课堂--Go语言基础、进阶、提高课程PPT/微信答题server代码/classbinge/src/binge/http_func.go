package main

import (
	"binge/DB"
	"binge/Protocol"
	"binge/weixin"
	_ "encoding/base64"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
)

// 相同目录下 文件必须同一个包名字
// 1 用户的授权
func examfunc(w http.ResponseWriter, req *http.Request) {
	fmt.Println("examfunc")
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取参数
		strProto, bProto := req.Form["Proto"]

		if bProto {
			switch strProto[0] {
			case strconv.Itoa(Proto.C2S_Login_Proto):
				{ // 登陆
					strcode, bcode := req.Form["code"]
					if !bcode {
						fmt.Fprint(w, "code 数据不存在！")
						return
					}
					data := WeiXin.HttpGet(strcode[0])
					if data != nil {
						fmt.Fprint(w, "登陆成功")
						return
					} else {
						fmt.Fprint(w, "登陆失败")
						return
					}
				}
			case strconv.Itoa(Proto.C2S_ExamList_Proto):
				{ // 获取首页列表
					// 解析数据 http://local.bytedancing.com:8080?Proto=2&pagenum=0
					strpagenum, bpagenum := req.Form["pagenum"]
					if !bpagenum {
						fmt.Fprint(w, "pagenum 数据不存在！")
						return
					}
					// 数据库操作
					fmt.Println(strpagenum[0])
					data := DB.Read_Exam_List(strpagenum[0])
					// 返回给客户端的结构数据
					// ---->  校验的处理
					b, _ := json.Marshal(data)
					fmt.Fprint(w, string(b))
					// fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
					return
				}
			case strconv.Itoa(Proto.C2S_ExamSearch_Proto):
				{ // 搜索 试卷
					strItype, bItype := req.Form["Itype"]
					strkeyword, bkeyword := req.Form["keyword"]
					if !bkeyword || !bItype {
						fmt.Fprint(w, "keyword || Itype数据不存在！")
						return
					}
					strkeywordtmp := strkeyword[0]
					strItypetmp := strItype[0]
					// 数据库操作
					data := DB.Read_ExamSearch_List(strItypetmp, strkeywordtmp)
					fmt.Fprint(w, data)
					return
				}
			case strconv.Itoa(Proto.C2S_Personal_Proto):
				{ // 个人中心
					strItype, bItype := req.Form["Itype"]
					stropenid, bopenid := req.Form["openid"]
					if bItype && bopenid {
						// 数据库操作
						data := DB.Read_UserExamList_List(strItype[0], stropenid[0])

						fmt.Fprint(w, data)
						return
					}
					fmt.Fprint(w, "个人中心参数错误")
					return
				}
			case strconv.Itoa(Proto.C2S_AddExamData_Proto):
				{ // 增加试题--web管理系统
					strName, bName := req.Form["Name"]
					strType, bType := req.Form["Type"]
					strPrice, bPrice := req.Form["Price"]
					strMsg, bMsg := req.Form["Msg"] // AI 脏数据的过滤  **;做信用记录
					strTime, bTime := req.Form["Time"]
					if bName && bType &&
						bPrice && bMsg && bTime {
						// DB操作数据
						ret := DB.InsertExam_Info(strName[0], strType[0], strPrice[0], strMsg[0], strTime[0])
						if ret == 1 {
							fmt.Fprint(w, "1|数据保存成功")
							return
						}
						fmt.Fprint(w, "0|数据保存失败")
						return
					}
					fmt.Fprint(w, "web系统传递参数错误")
					return
				}
			case strconv.Itoa(Proto.C2S_ExamStatistic_Proto):
				{
					strNum, bNum := req.Form["Num"]
					_ = strNum
					if bNum {
						// 获取的数据 ---- 非实时 -->
						// AI --> redis 实时获取我们的数据 -- update 小程序首页推荐栏
						// 产生的排名的数据  --->  定时写入 每天的0点到1点直接 DB

						fmt.Fprint(w, "")
						return
					}
					fmt.Fprint(w, "web系统传递参数错误")
					return
				}
			default:
				fmt.Fprint(w, "协议不存在")
				return
			}
			fmt.Fprint(w, "Proto数据错误")
			return
		}
		fmt.Fprint(w, "参数错误")
		return
	}
	fmt.Fprint(w, "非GET请求！！！")
	return
}
