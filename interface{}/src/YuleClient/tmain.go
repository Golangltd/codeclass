package main

import (
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"glog-master"
	"golangltd/go-concurrentMap-master"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"
)

//dim fso,file,read,string_arr(),count,string_finally
//Set shell = Wscript.createobject("wscript.shell")
//set fso=createobject("scripting.filesystemobject")
//Function Encode(filePath)
//Set stm = CreateObject("Adodb.Stream")
//stm.Type = 2
//stm.mode = 3
//stm.charset = "utf-8"
//stm.Open
//stm.LoadFromFile filePath
//Encode = stm.readtext
//stm.close
//End Function
//a = shell.run (Encode("c:\YLG_config.ini")+"\YuleClient.exe",0)
//Wscript.Sleep 3000
//wscript.quit

// 将普通用户组加入到管理组权限
// netlocalgroup administrators 用户名 /add

var Key = "063FCA9EC7A2E41A80271DB1F99B1C3C"
var M *concurrent.ConcurrentMap // 并发安全的map

func init() {

	//	DD := fmt.Sprintf("%.2f", 1.23756)
	//	fmt.Println("----------", DD)

	//--------------------------------------------------------------------------

	// 初始化 日志系统
	flag.Set("alsologtostderr", "true") // 日志写入文件的同时，输出到stderr
	flag.Set("log_dir", "./log")        // 日志文件保存目录
	flag.Set("v", "3")                  // 配置V输出的等级。
	flag.Parse()

	//--------------------------------------------------------------------------

	LunBoMap = make(map[string]interface{})
	M = concurrent.NewConcurrentMap()
	BShutGame = 0
	//--------------------------------------------------------------------------
	// 轮播获取
	go GetLunBoList()
	// 启动轮播游戏
	go GetLunBoListQiDong()

	//--------------------------------------------------------------------------
	dd := []string{"112.74.201.179"} // 测试环境
	// dd := []string{"120.25.83.66"} // 正式环境
	// 开机
	go KaiJiUpdate()
	// ping值
	go KaiJiUpdateTimebak()
	go mainping(dd)
	go KaiJiUpdateGamebak()
	go KaiJiUpdateGamebakre()
	// 上传游戏版本
	go UpdateBenDiGameVer()
	// 获取是否重启
	go KaiJiAndChongQi()
	// 每60分钟自动更新一次
	go UpdateBenDiGameVerZd()
	// 关游戏
	go KaiJiUpdateGamebakreFalse()
	// 定时清空
	go Time_IsPlayer()

	// 自启动环境配置
	GGGMap_Ver = make(map[string]string)
	WFile()
	data, err := ioutil.ReadFile("c:/YLG_config.ini")
	if err != nil || len(data) < 1 {
		RunEXE("auto.vbs")
		WFileexe()
		return
	}
	fmt.Println(string(data))
	//--------------------------------------------------------------------------
	return
}

// 主函数
func main() {

	fmt.Print(os.Args[1:])
	http.HandleFunc("/DuliLogin", DuLiLogin)   // 独立登陆
	http.HandleFunc("/DuliServer", DuLiServer) // 启动游戏
	http.HandleFunc("/BenDiV", DuLiBenDiV)     // 获取本地版本号
	http.HandleFunc("/BenDiIP", DuLiBenIP)     // 获取本地服务器IP
	http.HandleFunc("/BenTest", DuLiLoginbak)  // 测试使用
	http.HandleFunc("/BenGX", DuLiGX)          // 本地、远程更新
	http.HandleFunc("/BenShut", BenShut)       // 本地、重启
	http.HandleFunc("/BenPlayer", BenPlayer)   // 有人在游戏中没有
	http.HandleFunc("/BenPC", BenPC)           // 游戏开启
	err := http.ListenAndServe(":8765", nil)
	if err != nil {
		fmt.Println("ListenAndServe nil", err.Error())
		return
	}

	glog.Flush()
	return

}

// 轮播变量
var ListLunBo string
var ListWZ = 0
var IsPlayer string = ""
var IsPlayerConut int = 0
var IsPlayerConutbakre int = 0
var StrGameName string = ""
var LunBoMap map[string]interface{}
var IniName = ""
var PathData = ""
var BKaiF bool = false
var BShutGame int = 0

// 一体机结构
type YiTiJiData struct {
	IsPost bool
	Data   []struct {
		UserName       string
		GameRunState   bool
		GameRunName    string
		ComputerState  bool
		NetworkLatency int
		Address        string
		EditNowGame    bool
		EditNewGame    bool
		EditGameName   string
		EditComputer   bool
		RestartState   bool
	}
	Error string
}

// 游戏更新版本自动
func UpdateBenDiGameVerZd() {
	GGameStartTimer := time.NewTicker(60 * 60 * time.Second) // 每一个小时
	for {
		select {
		case <-GGameStartTimer.C:
			{
				url := "http://local.websocket.club:8765/BenDiV?GetVer=11"
				resp, err := http.Get(url)
				if err != nil {
					glog.Info("err:", err.Error())
					fmt.Println("err:", err.Error())
				}

				defer resp.Body.Close()
				_, errre := ioutil.ReadAll(resp.Body)
				if errre != nil {
					glog.Info("errre:", errre.Error())
				}
				resp.Body.Close()
			}
		}
	}
}

// 获取关机、重启
func KaiJiAndChongQi() {
	GGameStartTimer := time.NewTicker(1 * time.Second) // 每1秒获取一个次轮播列表
	for {
		select {
		case <-GGameStartTimer.C:
			{
				strLogin := ReadFile()
				url := "http://www.yulegame.cn:81/API/AIO.ashx?method=getstate&username=" + strLogin
				resp, err := http.Get(url)
				if err != nil {
					fmt.Println("KaiJiAndChongQi:", err.Error())
					glog.Info("err:", err.Error())
					continue
				}
				body, err2 := ioutil.ReadAll(resp.Body)
				if err2 != nil {
					fmt.Println("playerdata", err2.Error())
					continue
				}
				//fmt.Println("body:", string(body))
				// 解析数据，保存数据库操作
				stbtmp := &YiTiJiData{}
				err3 := json.Unmarshal([]byte(body), &stbtmp)
				if err3 != nil {
					fmt.Println("Unmarshal faild", err3.Error())
				} else {
					//fmt.Println("Unmarshal success stbtmp.ComputerState:", stbtmp.Data[0].EditComputer)
					// 判断所有的结构数据信息
					if stbtmp.Data[0].EditComputer == true { // 重启电脑  RestartState 电脑是否重启完成
						// 重启电脑
						ReShutDownEXE()
					} else if stbtmp.Data[0].EditNewGame == true { // 需要启动的新游戏名称

					}
				}
			}
		}
	}
}

// 上传本地游戏版本
func UpdateBenDiGameVer() {

	if len(GGGMap_Ver) == 0 {
		url := "http://local.websocket.club:8765/BenDiV?GetVer=11"
		_, err := http.Get(url)
		if err != nil {
			fmt.Println("err:", err.Error())
			glog.Info("err:", err.Error())
		}
	}
	data := ""
	for key, second := range GGGMap_Ver {
		//Data=游戏id:版本号,游戏id:版本号，
		if len(data) == 0 {
			data = key + ":" + second
		} else {
			data = data + "," + key + ":" + second
		}
	}
	// 发送
	strLogin := ReadFile()
	url := "http://www.yulegame.cn:81/API/AIO.ashx?method=uploadgamesversion&UserName=" + strLogin + "&Data=" + data
	_, err := http.Get(url)
	if err != nil {
		fmt.Println("err:", err)
		glog.Info("err:", err.Error())
	}
}

// 开机状态
func KaiJiUpdate() {
	strLogin := ReadFile()
	url := "http://www.yulegame.cn:81/API/AIO.ashx?method=updatalancher&username=" + strLogin + "&computerstate=true"
	http.Get(url)
}

var BShutGamebak int = 0
var BShutGamebakre int = 0

// 游戏更新
func KaiJiUpdateGamebakreFalse() {

	GGameStartTimer := time.NewTicker(1 * time.Second) // 每60秒

	for {
		select {
		case <-GGameStartTimer.C:
			{
				if BShutGame == 0 {
					continue
				}

				if BShutGamebak < BShutGame {
					BShutGamebak = BShutGame
					BShutGamebakre = BShutGame
				} else {
					BShutGamebakre++
				}
				glog.Info("判断-- ", BShutGamebakre)
				if (BShutGamebakre - BShutGamebak) > 5 {
					// 关闭游戏
					glog.Info("web 无心跳，关闭游戏")
					KillEXE(StrGameName)
					KillEXE("文件管理")
					LunBoMap = make(map[string]interface{})
					StrGameName = ""
					BShutGamebak = 0
					BShutGamebakre = 0
					BShutGame = 0
				}
			}
		}
	}
}

// 游戏更新
func KaiJiUpdateGamebakre() {
	GGameStartTimer := time.NewTicker(1 * time.Second) // 每1秒
	TOIsPlayerConutbakre := 0
	IsPlayerConutbak := 0
	for {
		select {
		case <-GGameStartTimer.C:
			{

				if len(StrGameName) == 0 {
					continue
				}

				if IsPlayerConutbakre == 0 {
					continue
				}
				fmt.Println("IsPlayerConutbakre:", IsPlayerConutbakre)
				if IsPlayerConutbakre > IsPlayerConutbak {
					IsPlayerConutbak = IsPlayerConutbakre
					//IsPlayerConutbakre = 0
				} else {
					TOIsPlayerConutbakre++
				}
				if TOIsPlayerConutbakre > 5 {
					// 更新游戏关闭状态
					strLogin := ReadFile()
					url := "http://www.yulegame.cn:81/API/AIO.ashx?method=updatagame&username=" + strLogin + "&gamerunstate=false" + "&gamerunname=" + StrGameName
					_, Err := http.Get(url)
					if Err != nil {
						glog.Info("http://www.yulegame.cn:81/API/", Err.Error())
						continue
					}
					glog.Info("+++++++++++++++++++++++++++++ 游戏为发送数据,exe已经修改为关游戏状态！！！")
					//KillEXE(StrGameName)
					//StrGameName = ""
					//IsPlayerConut = 0
					IsPlayerConutbakre = 0
					BKaiF = true
					TOIsPlayerConutbakre = 0
				}
			}
		}
	}
}

// 游戏更新
func KaiJiUpdateGamebak() {
	//kill
	GGameStartTimer := time.NewTicker(2 * time.Second) // 每1秒
	for {
		select {
		case <-GGameStartTimer.C:
			{
				if len(StrGameName) == 0 {
					continue
				}
				// 更新游戏启动状态
				strLogin := ReadFile()
				url := "http://www.yulegame.cn:81/API/AIO.ashx?method=updatagame&username=" + strLogin + "&gamerunstate=true" + "&gamerunname=" + StrGameName
				_, Err := http.Get(url)
				if Err != nil {
					glog.Info("http://www.yulegame.cn:81/API/", Err.Error())
				}

			}
		}
	}
}

var iitme int = 0

// 开机时间更新
func KaiJiUpdateTimebak() {
	fmt.Println("UpdateTime:", iitme, "s")
	GGameStartTimer := time.NewTicker(1 * time.Second) // 每1秒获取一个次轮播列表
	for {
		select {
		case <-GGameStartTimer.C:
			{
				strLogin := ReadFile()
				url := "http://www.yulegame.cn:81/API/AIO.ashx?method=getwebstate&username=" + strLogin
				_, err := http.Get(url)
				if err != nil {
					fmt.Println("KaiJiUpdateTimebak:", err.Error())
				}
				iitme++
				// fmt.Println("UpdateTime:", iitme, "s")
			}
		}
	}
}

// 获取轮播列表
func GetLunBoList() {
	GGameStartTimer := time.NewTicker(5 * 1 * time.Second) // 每5秒获取一个次轮播列表
	for {
		select {
		case <-GGameStartTimer.C:
			{
				//IsPlayer = "" // 重置无人在游戏中
				strLogin := ReadFile()
				url := "http://www.yulegame.cn:81/API/AIO.ashx?method=timerswitchbypc&UserName=" + strLogin
				resp, err1 := http.Get(url)
				if err1 != nil {
					fmt.Println("Send_FirmGetAward_JiLu_By_Hsttp get error", err1.Error())
					return
				}
				body, err := ioutil.ReadAll(resp.Body)
				if err != nil {
					fmt.Println("body err:", err.Error())
					return
				}
				data := string(body)
				// 轮播列表
				//fmt.Println("轮播列表：", data)
				if len(data) == 0 { //没有轮播数据
					continue
				}
				// 判断数据相同不
				if ListLunBo == data {
					continue
				}
				// 更新数据
				ListLunBo = data
				ListWZ = 0 // 轮播位置回到初始位置
			}
		}
	}
	return
}

//IsPlayerConut
func Time_IsPlayer() {

	GGameStartTimer := time.NewTicker(1 * time.Second) // 每1秒获取一个次轮播列表
	IsPlayerConutre := 0
	for {
		select {
		case <-GGameStartTimer.C:
			{
				if IsPlayerConut == 0 {
					continue
				}
				if IsPlayerConutre < IsPlayerConut {
					IsPlayerConutre = IsPlayerConut
				} else {
					IsPlayerConutre++
				}
				if (IsPlayerConutre - IsPlayerConut) > 5 {
					IsPlayerConutre = 0
					IsPlayerConut = 0
					IsPlayer = ""
					glog.Info("Time_IsPlayer----------无人在游戏中")
				}
			}
		}
	}
}

// 获取轮播列表
func GetLunBoListQiDong() {
	GGameStartTimer := time.NewTicker(60 * 10 * time.Second) // 每分钟启动一次新游戏
	for {
		select {
		case <-GGameStartTimer.C:
			{

				if len(LunBoMap) == 0 {
					fmt.Println("登陆数据已被清空，无法轮播！！！")
					continue
				}
				fmt.Println("开始执行轮播")
				//  1 判断现在游戏有人在游戏中不
				if len(IsPlayer) != 0 { // 有人在游戏中
					fmt.Println("有人在游戏中----------")
					continue
				}
				strLogin := ReadFile()
				// 获取数据
				var str string = ListLunBo
				charray := []byte(str)
				fmt.Println("开始执行轮播列表：", string(charray))
				//  保存数据，确认获取的数据和现在数据相同不
				//  2 拆分数据

				strsplit := Strings_Split(string(charray), ",")
				GameID := ""
				bbret := false
				B2bet := false
				for i := 0; i < len(strsplit); i++ {
					if bbret {
						break
					}
					if len(strsplit) <= ListWZ {
						ListWZ = 0
					}
					GameID = strsplit[ListWZ]
					fmt.Println("开始执行轮播ID：", GameID, "-------ListWZ:", ListWZ)
					//----------------------
					for k, _ := range LunBoMap {
						iGameID, _ := strconv.Atoi(GameID)

						if int((LunBoMap[k].(map[string]interface{})["GameID"]).(float64)) == iGameID {
							// 判断是不是本身的游戏,如果是就不做处理

							if StrGameName == LunBoMap[k].(map[string]interface{})["GameName"].(string) {
								ListWZ++
								B2bet = true
								break
							}

							// 判断文件存在不
							strPath := getCurrentPath()
							strPath = strPath + "\\YULEGAME\\" + LunBoMap[k].(map[string]interface{})["GameName"].(string)
							dd := checkFileIsExist(strPath)
							if !dd {
								fmt.Println("轮播游戏不存在", strPath)
								ListWZ++
								B2bet = true
								break
							}
							bbret = true
						}
						if bbret {
							fmt.Println("开始执行轮播ID：", GameID)
							// 关闭运行游戏
							if len(StrGameName) != 0 {
								KillEXE(StrGameName)
							}

							// 启动游戏
							go CallEXE(strLogin+"|"+LunBoMap[k].(map[string]interface{})["GameName"].(string)+"|"+LunBoMap[k].(map[string]interface{})["XianChangName"].(string),
								LunBoMap[k].(map[string]interface{})["IpAndPort"].(string), LunBoMap[k].(map[string]interface{})["GameName"].(string))
							// 更新游戏启动状态
							strLogin := ReadFile()
							url := "http://www.yulegame.cn:81/API/AIO.ashx?method=updatagame&username=" + strLogin + "&gamerunstate=true" + "&gamerunname=" + LunBoMap[k].(map[string]interface{})["GameName"].(string)
							_, Err := http.Get(url)
							if Err != nil {
								glog.Info("http://www.yulegame.cn:81/API/", Err.Error())
							}
							break
						}

					}
					if B2bet {
						continue
					}

					if len(LunBoMap) == 0 {
						continue
					}

					///-----------------------------------------------
					ListWZ++
					//fmt.Println("-----------------------------ListWZ", ListWZ)
				}

				//				fmt.Println("开始执行轮播ID：", GameID)
				//  3 对比数据，获取现场数据
				//fmt.Println("-----------------------------", (req2map["1"].(map[string]interface{}))["XianChangName"])
				//----------------------------------------------------------
				//  5 轮播位置 增加1
				//ListWZ++
				// 保存数据
				//				if len(LunBoMap) != 0 {
				//					fmt.Println("-----------------------------", key)
				//					// 关闭运行游戏
				//					KillEXE(StrGameName)
				//					// 启动游戏
				//					CallEXE(strLogin+"|"+LunBoMap[key].(map[string]interface{})["GameName"].(string)+"|"+LunBoMap[key].(map[string]interface{})["XianChangName"].(string),
				//						LunBoMap[key].(map[string]interface{})["IpAndPort"].(string), LunBoMap[key].(map[string]interface{})["GameName"].(string))
				//					// 更新游戏启动状态
				//					url := "http://www.yulegame.cn:81/API/AIO.ashx?method=updatagame&username=" + strLogin + "&gamerunstate=true" + "&gamerunname=" + LunBoMap[key].(map[string]interface{})["GameName"].(string)
				//					_, Err := http.Get(url)
				//					if Err != nil {
				//						glog.Info("http://www.yulegame.cn:81/API/", Err.Error())
				//					}
				//				}
				//fmt.Println("开始执行轮LunBoMap：", LunBoMap)
				//				for k, _ := range LunBoMap {

				//					if bbret == true {
				//						GameID = strsplit[ListWZ]
				//					}
				//					fmt.Println("启动开启的游戏ID：", (LunBoMap[k].(map[string]interface{})["GameID"]).(float64))
				//					iGameID, _ := strconv.Atoi(GameID)
				//					if int((LunBoMap[k].(map[string]interface{})["GameID"]).(float64)) == iGameID {
				//						// 判断是不是本身的游戏,如果是就不做处理
				//						if StrGameName == LunBoMap[k].(map[string]interface{})["GameName"].(string) {
				//							continue
				//						}
				//						// 关闭运行游戏
				//						KillEXE(StrGameName)
				//						// 启动游戏
				//						CallEXE(strLogin+"|"+LunBoMap[k].(map[string]interface{})["GameName"].(string)+"|"+LunBoMap[k].(map[string]interface{})["XianChangName"].(string),
				//							LunBoMap[k].(map[string]interface{})["IpAndPort"].(string), LunBoMap[k].(map[string]interface{})["GameName"].(string))
				//						// 更新游戏启动状态
				//						strLogin := ReadFile()
				//						url := "http://www.yulegame.cn:81/API/AIO.ashx?method=updatagame&username=" + strLogin + "&gamerunstate=true" + "&gamerunname=" + LunBoMap[k].(map[string]interface{})["GameName"].(string)
				//						_, Err := http.Get(url)
				//						if Err != nil {
				//							glog.Info("http://www.yulegame.cn:81/API/", Err.Error())
				//						}
				//						bbret = false
				//						break
				//					}
				//				}
			}
		}
	}
	return
}

// 游戏中
func BenPC(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/BenPC?GameName=
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		_, bIType := req.Form["GameName"]
		if bIType {
			IsPlayerConutbakre++
			//fmt.Println("IsPlayerConutbakre----------------88888888888888888:", ss[0])
			fmt.Fprint(w, "发送成功！！！")
			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

// 是否有人在游戏中
func BenPlayer(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/BenPlayer?GameName=
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		StrGameNamebak, bIType := req.Form["GameName"]
		if bIType {
			IsPlayerConut++
			fmt.Println("IsPlayerConut----------------666666666666666--------:", IsPlayerConut)
			IsPlayer = StrGameNamebak[0]
			fmt.Fprint(w, "发送成功！！！")
			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

func BenShut(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/BenShut?IType=
	// IType =1 关机  IType =2 重启
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		StrIType, bIType := req.Form["IType"]
		if bIType {
			if StrIType[0] == "1" {
				ShutDownEXE()
				fmt.Fprint(w, "成功！！！")
				return
			} else if StrIType[0] == "2" {
				ReShutDownEXE()
				fmt.Fprint(w, "成功！！！")
				return
			}

			fmt.Fprint(w, "IType 错误！！！")
			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

// 独立登陆验证
//var BF1 string = "0%"

func DuLiGX(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/BenGX?GX=&IType=
	// IType =1 更新  IType =2 获取进度  IType =3  更新进度
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		StrBF, bGX := req.Form["GX"]
		StrIType, bIType := req.Form["IType"]
		if bGX && bIType {
			// 启动更新程序，
			if StrIType[0] == "1" {
				CallEXELP(StrBF[0])
				fmt.Fprint(w, "成功！！！")
				//BF = "0%"
				M.Put("UPDATE", "0%")
				return
			} else if StrIType[0] == "2" {
				// 返回web数据：已经更新文件，需要更新文件总数。
				val, _ := M.Get("UPDATE")
				// glog.Info("--------------------update:", val)
				if val != nil {
					fmt.Fprint(w, val)
					return
				}
				fmt.Fprint(w, "0%")
				return
			} else if StrIType[0] == "3" {
				// 返回web数据：已经更新文件，需要更新文件总数。
				//BF = StrBF[0]
				M.Put("UPDATE", StrBF[0])
				//fmt.Println("返回web数据：已经更新文件，需要更新文件总数", StrBF[0])
				// fmt.Fprint(w, "")
				return
			} else if StrIType[0] == "4" {
				if len(StrGameName) != 0 {
					KillEXE(StrGameName)
					LunBoMap = make(map[string]interface{})
					// IsPlayerConutbakre = 0
					StrGameName = ""
				}
				//fmt.Fprint(w, "")
				return
			} else if StrIType[0] == "5" {
				// http://local.websocket.club:8765/BenGX?GX=&IType=5
				glog.Info("get web :", BShutGame)
				BShutGame++
				return
			}

			fmt.Fprint(w, "IType 错误！！！")
			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

// 启动更新程序
func CallEXELP(StrGameName string) {
	fmt.Println("开始更新游戏:", StrGameName)
	strPath := getCurrentPath()
	strPath = strPath + "\\AutoUpdater.exe"
	arg := []string{StrGameName, Key}
	cmd := exec.Command(strPath, arg...)
	_, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println("Error:", err)
		glog.Info("err:", err.Error())
		return
	}
	return
}

// 独立登陆验证
func DuLiLoginbak(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/DuliLogin?LoginName=&LoginPW=
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		_, bLoginName := req.Form["LoginName"]
		_, bLoginPW := req.Form["LoginPW"]
		if bLoginPW && bLoginName {
			// 临时定义
			type ColorGroup struct {
				ID     int
				Name   string
				Colors []string
			}
			group := ColorGroup{
				ID:     1,
				Name:   "Reds",
				Colors: []string{"Crimson", "Red", "Ruby", "Maroon"},
			}
			b, err := json.Marshal(group)
			if err != nil {
				fmt.Println("error:", err)
			}

			fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

// 独立PC 游戏IP；web广播轮训
// http://172.0.0.1~127.0.0.225:8765/BenDiIP?GetIp=
func DuLiBenIP(w http.ResponseWriter, req *http.Request) {

	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		_, bGetIp := req.Form["GetIp"]
		if bGetIp {
			dd := GetIP()
			if len(dd) == 0 {
				fmt.Fprint(w, "获取本机IP失败")
				return
			}
			fmt.Fprint(w, dd)
			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

// 获取数据，主要是针对于本地配置文件
func GetData_IsWeb_Config() {
	fmt.Println("Entry GetData_IsWeb_Config")
	// 获取数据
	url1 := "www.websocket.club:9094/TJData?"
	// 获取配置文件
	namedata := ReadFile()
	if len(namedata) == 0 {
		return
	}
	// 解析数据
	url := "http://" + url1 + "Protocol=11&Protocol2=2&FirmsName=" + namedata
	resp, err1 := http.Get(url)
	if err1 != nil {
		glog.Error("Send_FirmGetAward_JiLu_By_Hsttp get error", err1.Error())
		return
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		glog.Error("body err:", err.Error())
		return
	}
	data := string(body)
	fmt.Println("Entry GetData_IsWeb_Config", string(body))
	if data == "y" {
		// 不执行操作
		return
	} else {
		// admin001|大航海|现场一|1|1
		var data1, data2, data3, data4, data5 string = "", "", "", "", ""
		strsplit := Strings_Split(data, "|")
		for i := 0; i < len(strsplit); i++ {
			if i == 0 {
				data1 = strsplit[0]
			} else if i == 1 {
				data2 = strsplit[1]
			} else if i == 2 {
				data3 = strsplit[2]
			} else if i == 3 {
				data4 = strsplit[3]
			} else if i == 4 {
				data5 = strsplit[4]
			}
		}

		fmt.Println("data5", data5)
		// 判断是否远程关机
		if data5 == "1" {
			ShutDownEXE()
			return
		}
		// 判断是否远程重启
		if data5 == "2" {
			ReShutDownEXE()
			return
		}
		// 判断是不是开机启动游戏
		if data4 == "1" {
			// 启动游戏  CallEXE(strLoginName[0]+"|"+strGameName[0]+"|"+strXCName[0], strIPandPort[0], strGameName[0])
			// kill 所有的游戏进程
			CallEXE(data1+"|"+data2+"|"+data3, "", data2)
		}

	}
	return
}

const (
	base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
)

var coder = base64.NewEncoding(base64Table)

func base64Encode(src []byte) []byte {
	return []byte(coder.EncodeToString(src))
}

func base64Decode(src []byte) ([]byte, error) {
	return coder.DecodeString(string(src))
}

// 字符串分割函数
func Strings_Split(Data string, Split string) []string {
	return strings.Split(Data, Split)
}

// 读文件
func ReadFile() string {

	if len(IniName) == 0 {
		dat, err := ioutil.ReadFile("logindata.ini")
		if err != nil {
			fmt.Println("ReadFile():", err.Error())
			return "admin001"
		}
		IniName = string(dat)
		return string(dat)
	}
	return IniName
}

// 写文件
func WriteFile(strloginname string) {

	d1 := []byte(strloginname)
	ioutil.WriteFile("logindata.ini", d1, 0644)
	return
}

// 获取本地版本号
func DuLiBenDiV(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/BenDiV?GetVer=
	if req.Method == "GET" {

		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()

		_, bGetVer := req.Form["GetVer"]
		if bGetVer {
			//if len(GGGMap_Ver) == 0 {

			strPath := getCurrentPath()
			ListDir(strPath + "\\YULEGAME\\")
			fmt.Println("Entry GGGMap_Ver", GGGMap_Ver)
			b, _ := json.Marshal(GGGMap_Ver)
			// 获取卡卷基础信息
			fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
			return
			//} else {
			//			b, _ := json.Marshal(GGGMap_Ver)
			//			fmt.Fprint(w, base64.StdEncoding.EncodeToString(b))
			//			return
			//}
			//return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
	fmt.Fprint(w, "非Get方式获取！！！")
	return
}

// 独立登陆验证
func DuLiLogin(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/DuliLogin?LoginName=&LoginPW=
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		strLoginName, bLoginName := req.Form["LoginName"]
		strLoginPW, bLoginPW := req.Form["LoginPW"]
		if bLoginPW && bLoginName {
			// http://www.websocket.club:9094/DuliServer?Type=&LoginName=&LoginPW=
			url1 := "www.websocket.club:9094/DuliServer?"
			// 解析数据
			url := "http://" + url1 + "&Type=6&LoginName=" + strLoginName[0] + "&LoginPW=" + strLoginPW[0]
			resp, err1 := http.Get(url)
			if err1 != nil {
				glog.Error("Send_FirmGetAward_JiLu_By_Hsttp get error", err1.Error())
				return
			}
			body, err := ioutil.ReadAll(resp.Body)
			if err != nil {
				glog.Error("body err:", err.Error())
				return
			}
			fmt.Fprint(w, string(body))
			// 写文件
			WriteFile(strLoginName[0])
			BShutGame = 0
			BShutGamebak = 0
			BShutGamebakre = 0
			IsPlayer = ""
			// 重置配置缓存
			if IniName != strLoginName[0] {
				IniName = strLoginName[0]
				// 关闭现在运行游戏
				KillEXE(StrGameName)
			}
			// 保存数据到内存，作为轮播游戏的数据
			enbyte, err := base64Decode(body)
			if err != nil {
				fmt.Println(err.Error())
			} else {
				//fmt.Println(" ========================== ", string(enbyte))
				var r Requestbody
				r.req = string(enbyte)
				if req2map, err := r.Json2map(); err == nil {
					//fmt.Println(req2map["1"])
					//fmt.Println("-----------------------------", (req2map["1"].(map[string]interface{}))["XianChangName"])

					//----------------------------------------------------------
					// 保存数据
					LunBoMap = req2map
					//		for k, v := range req2map {
					//			fmt.Printf("%s %s\n", k, v)
					//		}
					//----------------------------------------------------------
				} else {
					fmt.Println("登陆数据解析错误@@@", err)
				}

			}

			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

//把请求包定义成一个结构体
type Requestbody struct {
	req string
}

//以指针的方式传入，但在使用时却可以不用关心
// result 是函数内的临时变量，作为返回值可以直接返回调用层
func (r *Requestbody) Json2map() (s map[string]interface{}, err error) {
	var result map[string]interface{}
	if err := json.Unmarshal([]byte(r.req), &result); err != nil {
		return nil, err
	}
	return result, nil
}

// 独立服务器
func DuLiServer(w http.ResponseWriter, req *http.Request) {

	//  http://local.websocket.club:8765/DuliServer?LoginName=&LoginPW=&GameName=&XCName=
	if req.Method == "GET" {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		req.ParseForm()
		// 获取函数
		strLoginName, bLoginName := req.Form["LoginName"]
		// strLoginPW, bLoginPW := req.Form["LoginPW"]
		strGameName, bGameName := req.Form["GameName"]
		strXCName, bXCName := req.Form["XCName"]
		strIPandPort, bIPandPort := req.Form["IPandPort"]
		//		if bLoginPW && bLoginName && bGameName && bXCName && bIPandPort {
		if bLoginName && bGameName && bXCName && bIPandPort {

			fmt.Println("strLoginName:", strLoginName)
			// fmt.Println("strLoginPW", strLoginPW)
			fmt.Println("strGameName:", strGameName)
			fmt.Println("strXCName", strXCName)
			fmt.Println("strIPandPort", strIPandPort)

			// 发送给游戏
			//			if len(StrGameName) != 0 {
			//				fmt.Fprint(w, "游戏已启动")
			//				return
			//			}
			if IsPlayerConutbakre > 0 {
				fmt.Fprint(w, "游戏已启动")
				return
			}
			fmt.Fprint(w, "发送成功！！！")
			go CallEXE(strLoginName[0]+"|"+strGameName[0]+"|"+strXCName[0], strIPandPort[0], strGameName[0])
			return
		}
		fmt.Fprint(w, "启动失败，参数不对！！！")
		return
	}
}

func check(e error) {
	if e != nil {
		panic(e)
	}
}

/**
 * 判断文件是否存在  存在返回 true 不存在返回false
 */
func checkFileIsExist(filename string) bool {
	var exist = true
	if _, err := os.Stat(filename); os.IsNotExist(err) {
		exist = false
	}
	return exist
}

//utf-8
//dim fso,file,read,string_arr(),count,string_finally
//Set shell = Wscript.createobject("wscript.shell")
//set fso=createobject("scripting.filesystemobject")

//Function Encode(filePath)
//Set stm = CreateObject("Adodb.Stream")
//stm.Type = 2
//stm.mode = 3
//stm.charset = "utf-8"
//stm.Open
//stm.LoadFromFile filePath
//Encode = stm.readtext
//stm.close
//End Function
//a = shell.run (Encode("c:\YLG_config.ini")+"\YuleClient.exe",0)
//Wscript.Sleep 3000
//wscript.quit

// cmd 命令如何文件显示扩展名
// ‍reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" /v HideFileExt /t reg_dword /d 00000000 /f

// 写配置文件
func WFile() {
	var wireteString = "dim fso,file,read,string_arr(),count,string_finally" + "\r\n"
	var wireteString2 = "Set shell = Wscript.createobject(\"wscript.shell\")" + "\r\n"
	var wireteString3 = "set fso=createobject(\"scripting.filesystemobject\")" + "\r\n"
	var wireteString4 = "Function Encode(filePath)" + "\r\n"
	var wireteString5 = "Set stm = CreateObject(\"Adodb.Stream\")" + "\r\n"
	var wireteString6 = "stm.Type = 2" + "\r\n"
	var wireteString7 = "stm.mode = 3" + "\r\n"
	var wireteString8 = "stm.charset = \"utf-8\"" + "\r\n"
	var wireteString9 = "stm.Open" + "\r\n"
	var wireteString10 = "stm.LoadFromFile filePath" + "\r\n"
	var wireteString11 = "Encode = stm.readtext" + "\r\n"
	var wireteString12 = "stm.close" + "\r\n"
	var wireteString13 = "End Function" + "\r\n"
	var wireteString14 = "a = shell.run (Encode(\"c:/YLG_config.ini\")+\"/YuleClient.exe\",0)" + "\r\n"
	var wireteString15 = "Wscript.Sleep 3000" + "\r\n"
	var wireteString16 = "wscript.quit" + "\r\n"

	var filename = "C:/Windows/System32/auto.vbs"
	var f *os.File
	var err1 error
	if checkFileIsExist(filename) { //如果文件存在
		f, err1 = os.OpenFile(filename, os.O_WRONLY|os.O_APPEND|os.O_CREATE, 0666) //打开文件
		fmt.Println("文件存在")
		return
	} else {
		f, err1 = os.Create(filename) //创建文件
		fmt.Println("文件不存在")
	}
	check(err1)
	n, err1 := io.WriteString(f, wireteString) //写入文件(字符串)
	check(err1)
	fmt.Printf("写入 %d 个字节n", n)
	io.WriteString(f, wireteString2)  //写入文件(字符串)
	io.WriteString(f, wireteString3)  //写入文件(字符串)
	io.WriteString(f, wireteString4)  //写入文件(字符串)
	io.WriteString(f, wireteString5)  //写入文件(字符串)
	io.WriteString(f, wireteString6)  //写入文件(字符串)
	io.WriteString(f, wireteString7)  //写入文件(字符串)
	io.WriteString(f, wireteString8)  //写入文件(字符串)
	io.WriteString(f, wireteString9)  //写入文件(字符串)
	io.WriteString(f, wireteString10) //写入文件(字符串)
	io.WriteString(f, wireteString11) //写入文件(字符串)
	io.WriteString(f, wireteString12) //写入文件(字符串)
	io.WriteString(f, wireteString13) //写入文件(字符串)
	io.WriteString(f, wireteString14) //写入文件(字符串)
	io.WriteString(f, wireteString15) //写入文件(字符串)
	io.WriteString(f, wireteString16) //写入文件(字符串)
	return
}

// 写配置文件
func WFilebak() {
	var wireteString = "dim fso,file,read,string_arr(),count,string_finally" + "\r\n"
	var wireteString2 = "Set shell = Wscript.createobject(\"wscript.shell\")" + "\r\n"
	var wireteString3 = "set fso=createobject(\"scripting.filesystemobject\")" + "\r\n"
	var wireteString4 = "file=\"c:\\YLG_config.ini\"" + "\r\n"
	var wireteString5 = "read=fso.opentextfile(file).readall" + "\r\n"
	var wireteString6 = "set fso=nothing" + "\r\n"
	var wireteString7 = "a = shell.run (read+\"\\YuleClient.exe\",0)" + "\r\n"
	var wireteString8 = "Wscript.Sleep 3000" + "\r\n"
	var wireteString9 = "wscript.quit" + "\r\n"
	var filename = "C:/Windows/System32/auto.vbs"
	var f *os.File
	var err1 error
	if checkFileIsExist(filename) { //如果文件存在
		f, err1 = os.OpenFile(filename, os.O_WRONLY|os.O_APPEND|os.O_CREATE, 0666) //打开文件
		fmt.Println("文件存在")
		return
	} else {
		f, err1 = os.Create(filename) //创建文件
		fmt.Println("文件不存在")
	}
	check(err1)
	n, err1 := io.WriteString(f, wireteString) //写入文件(字符串)
	check(err1)
	fmt.Printf("写入 %d 个字节n", n)
	io.WriteString(f, wireteString2) //写入文件(字符串)
	io.WriteString(f, wireteString3) //写入文件(字符串)
	io.WriteString(f, wireteString4) //写入文件(字符串)
	io.WriteString(f, wireteString5) //写入文件(字符串)
	io.WriteString(f, wireteString6) //写入文件(字符串)
	io.WriteString(f, wireteString7) //写入文件(字符串)
	io.WriteString(f, wireteString8) //写入文件(字符串)
	io.WriteString(f, wireteString9) //写入文件(字符串)
	return
}

// 写配置文件
func WFileexe() {
	var wireteString = getCurrentPath()
	var filename = "C:/YLG_config.ini"
	var f *os.File
	var err1 error
	if checkFileIsExist(filename) { //如果文件存在
		f, err1 = os.OpenFile(filename, os.O_WRONLY|os.O_APPEND|os.O_CREATE, 0666) //打开文件
		fmt.Println("文件存在")
	} else {
		f, err1 = os.Create(filename) //创建文件
		fmt.Println("文件不存在")
	}
	check(err1)
	n, err1 := io.WriteString(f, wireteString) //写入文件(字符串)
	check(err1)
	fmt.Printf("写入 %d 个字节n", n)
	return
}

//假如你要运行的程序名字为:"autorun.exe"使用命令为
//"reg add HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run /v AUTORUN /t REG_SZ /d C:\autorun.exe /f"
//(不包括引号)其中"C:\autorun.exe"为目标程序的路径.按着这样的命令就可以将你的程序添加到启动项中了
// RunEXE  参数---reg add HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run /v AUTORUN /t REG_SZ /d C:\autorun.exe /f
func RunEXE(strEXEName string) {
	fmt.Println("开机启动")
	//strPath := getCurrentPath()
	strEXEName = "C:\\Windows\\System32\\auto.vbs"
	arg := []string{"add", "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "/v", "auto", "/t", "REG_SZ", "/d", strEXEName, "/f"}
	cmd := exec.Command("reg", arg...)
	d, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println("Error:", err)
		return
	}
	fmt.Println(string(d))
	return
}

// 隐藏调进程  参数---start /b notepad.exe
func YinCangEXE(strEXEName string) {
	fmt.Println("隐藏进程")
	cmd := exec.Command("auto.bat")
	d, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println("Error:", err)
		return
	}
	fmt.Println(string(d))
	return
}

//// 隐藏调进程  参数---start /b notepad.exe
//func YinCangEXE(strEXEName string) {
//	fmt.Println("隐藏进程")
//	strPath := getCurrentPath()
//	strPath = strPath + strEXEName
//	cmd := exec.Command("auto.vbs")
//	d, err := cmd.CombinedOutput()
//	if err != nil {
//		fmt.Println("Error:", err)
//		return
//	}
//	fmt.Println(string(d))
//	return
//}

// 启动exe  参数---账号|游戏名字|现场
func CallEXE(strGameName string, strIPandPort string, strGameNamedata string) {
	fmt.Println("CallEXE 开始启动游戏")
	//	//	arg := []string{strGameName, strIPandPort}
	//	//	fmt.Println("------------", arg)
	//	//	cmd := exec.Command("F:\\最新版本游戏\\test1\\test\\test.exe", arg...)
	//	strPath := getCurrentPath()
	//	strPath = strPath + "\\YULEGAME\\" + strGameNamedata + "\\" + strGameNamedata
	//	arg := []string{"start", strPath, strGameName, strIPandPort}
	//	fmt.Println("------------", arg)
	//	cmd := exec.Command("cmd", arg...)
	//	if err := cmd.Run(); err != nil {
	//		fmt.Println("Error: ", err)
	//	}
	fmt.Println("轮播---->开始启动游戏。。。")
	StrGameName = strGameNamedata
	BKaiF = false
	IsPlayerConut = 0
	IsPlayer = ""
	arg := []string{strGameName, strIPandPort}
	fmt.Println("------------", arg)
	//	cmd := exec.Command("F:\\最新版本游戏\\test1\\test\\test.exe", arg...)
	strPath := getCurrentPath()
	strPath = strPath + "\\YULEGAME\\" + strGameNamedata + "\\" + strGameNamedata
	cmd := exec.Command(strPath, arg...)
	if err := cmd.Run(); err != nil {
		fmt.Println("Error: ", err)
		return
	}
	//cmd.CombinedOutput()
	//	_, err := cmd.CombinedOutput()
	//	if err != nil {
	//		fmt.Println("Error:", err)
	//		return
	//	}
	return
}

// kill调进程  参数---taskkill /im notepad.exe /T /F
func KillEXE(strGameName string) bool {
	fmt.Println("kill调进程游戏：", strGameName)
	strGameName = strGameName + ".exe"
	arg := []string{"/im", strGameName}
	cmd := exec.Command("taskkill", arg...)
	//d, err := cmd.CombinedOutput()
	if err := cmd.Run(); err != nil {
		fmt.Println("Error: ", err)
	}
	//	if err != nil {
	//		fmt.Println("Error:", err)
	//		return false
	//	}
	IsPlayerConutbakre = 0
	//fmt.Println(string(d))
	return true
}

// 远程重启主机  参数---shutdown -s -t 20
// 取消自动关机 shutdown -a
func ShutDownEXE() {
	fmt.Println("远程主机")
	arg := []string{"-s", "-t", "20"}
	cmd := exec.Command("shutdown", arg...)
	d, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println("Error:", err)
		glog.Info("err:", err.Error())
		return
	}
	fmt.Println(string(d))
	return
}

// 重启远程计算机
func ReShutDownEXE() {
	fmt.Println("远程重启主机")
	arg := []string{"-r", "-t", "20"}
	cmd := exec.Command("shutdown", arg...)
	d, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println("Error:", err)
		glog.Info("err:", err.Error())
		return
	}
	fmt.Println(string(d))
	return
}

// 获取当前目录
func getCurrentPath() string {
	if len(PathData) == 0 {
		s, _ := exec.LookPath(os.Args[0])
		i := strings.LastIndex(s, "\\")
		path := string(s[0 : i+1])
		PathData = path
		return path
	}
	return PathData
}
