package main

import (
	"DSQ_login/Proto"
	"DSQ_server/Match"
	"DSQ_server/Proto"
	"DSQ_server/ST"
	"LollipopGo/LollipopGo/network"
	"encoding/json"
	"fmt"
	"math"
)

//------------------------------------------------------------------------------
// 共用函数

func (this *DSQGame) GetRoomID(stropenid string) string {
	val, err := M.Get(stropenid + "|User")
	if err != nil {
		return ""
	}
	return val.(*DSQGame).StrMD5
}

// 客户端与服务器的棋盘位置对应关系
func (this *DSQGame) GetClientAndServerPos(strpos string) (int, int) {
	if strpos == "1" {
		return 0, 0
	} else if strpos == "2" {
		return 0, 1
	} else if strpos == "3" {
		return 0, 2
	} else if strpos == "4" {
		return 0, 3
	} else if strpos == "5" {
		return 1, 0
	} else if strpos == "6" {
		return 1, 1
	} else if strpos == "7" {
		return 1, 2
	} else if strpos == "8" {
		return 1, 3
	} else if strpos == "9" {
		return 2, 0
	} else if strpos == "10" {
		return 2, 1
	} else if strpos == "11" {
		return 2, 2
	} else if strpos == "12" {
		return 2, 3
	} else if strpos == "13" {
		return 3, 0
	} else if strpos == "14" {
		return 3, 1
	} else if strpos == "15" {
		return 3, 2
	} else if strpos == "16" {
		return 3, 3
	}

	return -1, -1
}

//------------------------------------------------------------------------------
// 认输协议处理
func (this *DSQGame) PlayerRenShu(ProtocolData map[string]interface{}) {

	strOpenID := ProtocolData["OpenID"].(string)

	/*
			type GameOver struct {
				PlayerWin  *Player_DSQ.PlayerData // 胜利的玩家
				PlayerFail *Player_DSQ.PlayerData // 失败的玩家
				GameName   string                 // 游戏的名字
				WinSeatID  int                    // 胜利人的位置信息，0或者1的位置信息
				Score      int                    // 胜利人获取的分数信息
			}

		  1. 通过OpenID，获取roomid信息
		  2. 通过第1步骤获取的roomid信息，去获取roomdata信息
		  3. 通过第2步骤获取到的roomdata信息，去获取TurnID，是否轮到本玩家操作
		  4. 铜鼓OpenID 获取位置信息， 1 --WinSeatID =0 或者 0 --WinSeatID = 1
		  5. Score 分数概念，一般是策划配置，不同的场次，等到的分数是不同的 web的GM系统，服务器的读表配置
		  6. 发送广播协议，给同一个房间下的玩家；结算信息的数据持久化，把结算数据保存到DB后者redis等---GM统计后者前端会显示个人战绩的信息
		 *7. 释放房间信息

	*/
	strroomid := this.GetRoomID(strOpenID)
	GRoomManagerPtr.RoomLock.RLock()
	roomdata := GRoomManagerPtr.GRoomData[strroomid]
	GRoomManagerPtr.RoomLock.RUnlock()
	if roomdata.SeatData[roomdata.TurnID].PlayerData.OpenID != strOpenID {
		// 没有轮到此玩家移动
		return
	}
	iseatid := 0 // 赢的一方的座位ISSD
	if roomdata.TurnID == 0 {
		iseatid = 1
	}
	// 游戏结束的协议
	gameover := &DSQ_ST.GameOver{
		PlayerWin:  roomdata.SeatData[iseatid].PlayerData,         // 胜利的玩家
		PlayerFail: roomdata.SeatData[roomdata.TurnID].PlayerData, // 失败的玩家
		WinSeatID:  iseatid,                                       // 胜利人的位置信息，0或者1的位置信息
		Score:      1000,
	}
	//广播消息
	data := &Proto_DSQGame.Broadcast_GameOver{
		Protocol:  10,
		Protocol2: 10,
		GameOver:  gameover,
	}
	vala, _ := M.Get(roomdata.SeatData[0].PlayerData.OpenID + "|User")
	valb, _ := M.Get(roomdata.SeatData[1].PlayerData.OpenID + "|User")
	vala.(*DSQGame).PlayerSendMessage(data)
	valb.(*DSQGame).PlayerSendMessage(data)
	// 注意点： 结算信息的数据持久化，把结算数据保存到DB后者redis等---GM统计后者前端会显示个人战绩的信息
	// 逻辑缺失*******************************
	// 释放房间信息
	GRoomManagerPtr.RoomLock.RLock()
	delete(GRoomManagerPtr.GRoomData, strroomid)
	GRoomManagerPtr.RoomLock.RUnlock()
	return
}

// 棋子”行走“
// 需要完成棋子行走后的，坐标的数据校验 例如：如果是左右移动，x坐标相差1，如果是上下移动，y的坐标相差1
func (this *DSQGame) PlayerXingZou(ProtocolData map[string]interface{}) {
	// { 解析客户端发过来的数据的字段，
	strOpenID := ProtocolData["OpenID"].(string)
	strOldPos := ProtocolData["OldPos"].(string)
	strNewPos := ProtocolData["NewPos"].(string)
	//}
	/*
	  棋子行走（服务器的数据校验）：
	  1. 通过OpenID，获取roomid信息
	  2. 通过第1步骤获取的roomid信息，去获取roomdata信息
	  3. 通过第2步骤获取到的roomdata信息，去获取TurnID，是否轮到本玩家操作
	  4. 通过OldPos获取当前位置数据是否被”翻开“，不被翻开的话，服务器不允许操作(ChessDataClient)
	  5. 第4步骤ok的话，NewPos是否被”翻开“
	  6. 第6步骤ok的话，服务器需要检测<1>:是否是同一方（8为分界线），<2>:不是同一方，比较大小
	  7. 触发棋子移动，广播同一房间下的玩家
	*/
	strroomid := this.GetRoomID(strOpenID)
	GRoomManagerPtr.RoomLock.RLock()
	roomdata := GRoomManagerPtr.GRoomData[strroomid]
	GRoomManagerPtr.RoomLock.RUnlock()
	// 获取turnid 0或者1

	if roomdata.SeatData[roomdata.TurnID].PlayerData.OpenID != strOpenID {
		// 没有轮到此玩家移动
		return
	}
	// 获取起始位置OldPos是否翻开
	xold, yold := this.GetClientAndServerPos(strOldPos)
	iposold := roomdata.ChessDataClient[xold][yold]
	xnew, ynew := this.GetClientAndServerPos(strNewPos)
	iposnew := roomdata.ChessDataClient[xnew][ynew]
	if iposold == 0 || iposnew == 0 {
		// 返回客户端无法移动
		return
	}
	// ????? 作业的逻辑

	// 检查是否是同一战队
	iposoldchess := roomdata.ChessData[xold][yold]
	iposnewchess := roomdata.ChessData[xnew][ynew]
	if (iposoldchess <= Proto_DSQGame.Mouse && iposnewchess <= Proto_DSQGame.Mouse) || (iposoldchess > Proto_DSQGame.Mouse && iposnewchess > Proto_DSQGame.Mouse) {
		// 同一战队，无法移动
		return
	}
	/*
	  棋子可以移动
	  1. 相等：自相残杀 oldpos，newpos  --> 空地 ，空地
	  2. oldpos保存的动物数值，大于newpos保存的动物的数值，吃掉newpos的动物,删除oldpos的显示信息  oldpos，newpos  --> 空地 ，oldpos
	  3. oldpos保存的动物数值，小于newpos保存的动物的数值，吃掉oldpos的动物， oldpos，newpos  --> 空地 ，newpos
	*/
	// 1. 相等：自相残杀 oldpos，newpos  --> 空地 ，空地
	if math.Abs(float64(iposoldchess-iposnewchess)) == Proto_DSQGame.Mouse {
		// 更新缓存数据
		roomdata.ChessData[xold][yold] = 0
		roomdata.ChessData[xnew][ynew] = 0
		roomdata.ChessDataClient[xold][yold] = 2
		roomdata.ChessDataClient[xnew][ynew] = 2
	}
	if iposoldchess > Proto_DSQGame.Mouse {
		if int(math.Abs(float64(iposoldchess-Proto_DSQGame.Mouse))) < iposnewchess {
			roomdata.ChessData[xold][yold] = 0
			roomdata.ChessData[xnew][ynew] = iposoldchess
			roomdata.ChessDataClient[xold][yold] = 2
		}
	} else {
		if int(math.Abs(float64(iposnewchess-Proto_DSQGame.Mouse))) < iposoldchess {
			roomdata.ChessData[xold][yold] = 0
			roomdata.ChessData[xnew][ynew] = iposnewchess
			roomdata.ChessDataClient[xold][yold] = 2
		}
	}
	//广播消息
	data := &Proto_DSQGame.Broadcast_Player_XingZou{
		Protocol:  10,
		Protocol2: 8,
		OpenID:    strOpenID,
		OldPos:    strOldPos,
		NewPos:    strNewPos,
	}
	vala, _ := M.Get(roomdata.SeatData[0].PlayerData.OpenID + "|User")
	valb, _ := M.Get(roomdata.SeatData[1].PlayerData.OpenID + "|User")
	vala.(*DSQGame).PlayerSendMessage(data)
	valb.(*DSQGame).PlayerSendMessage(data)
	return
}

// 翻牌的函数实现
func (this *DSQGame) PlayerFanPai(ProtocolData map[string]interface{}) {
	strOpenID := ProtocolData["OpenID"].(string)
	StrPos := ProtocolData["StrPos"].(string)
	fmt.Println("strOpenID-------------------", strOpenID)
	fmt.Println("StrPos-------------------", StrPos)
	// 服务器处理逻辑
	strroomid := this.GetRoomID(strOpenID)
	fmt.Println("strroomid-------------------", strroomid)
	if len(strroomid) == 0 {
		// 返回客户端错误码，为什么错误
		return
	}
	/*
			  1. 获取到了客户端发过来的信息
			  2. 通过获取的数据信息---> roomid信息
			  3. 通过第2步骤获取到的roomid信息，去获取我们的roomdata信息
			  4. 通过第3步骤获取到的roomdata信息，去查找客户端翻牌对应位置的动物的数据--->(算法的对应的关系)
		      5. 广播此房间下的玩家
	*/
	GRoomManagerPtr.RoomLock.RLock()
	roomdata := GRoomManagerPtr.GRoomData[strroomid]
	GRoomManagerPtr.RoomLock.RUnlock()
	if roomdata == nil {
		// 房间信息不存在
		return
	}
	/*
		    通过第3步骤获取到的roomdata信息，去查找客户端翻牌对应位置的动物的数据--->(算法的对应的关系)
		    // 初始化棋盘数据
			[
			  [6 8 11 5]
			  [12 10 15 3]
			  [16 1 2 14]
			  [4 7 13 9]
			]
	*/
	chessdata := roomdata.ChessData
	fmt.Println("chessdata-------------------", chessdata)
	x, y := this.GetClientAndServerPos(StrPos)
	chessnum := chessdata[x][y]
	roomdata.ChessDataClient[x][y] = 1
	fmt.Println("chessnum-------------------", chessnum)
	fmt.Println("roomdata.ChessDataClient-------------------", roomdata.ChessDataClient)
	// 发送消息
	data := &Proto_DSQGame.Broadcast_Player_FanPai{
		Protocol:  10,
		Protocol2: 6,
		IChess:    chessnum,
		OpenID:    strOpenID,
		StrPos:    StrPos,
	}
	// 广播协议
	vala, _ := M.Get(roomdata.SeatData[0].PlayerData.OpenID + "|User")
	valb, _ := M.Get(roomdata.SeatData[1].PlayerData.OpenID + "|User")
	vala.(*DSQGame).PlayerSendMessage(data)
	valb.(*DSQGame).PlayerSendMessage(data)
	return
}

// 玩家匹配
func (this *DSQGame) PlayerPiPei(ProtocolData map[string]interface{}) {
	fmt.Println("PlayerPiPei-------------------", ProtocolData)
	strOpenID := ProtocolData["OpenID"].(string)

	// 匹配逻辑
	// 1. 队列形式-- chan
	// 2. 匹配成功
	val, err := this.MapSafe.Get(strOpenID + "|User")
	if err != nil {
		fmt.Println(err)
		return
	}
	fmt.Println("--------------------val:", val)
	DSQ_match.PutMatchList(val.(*DSQGame).Player)
	return
}

// 玩家登录
func (this *DSQGame) PlayerLogin(ProtocolData map[string]interface{}) {
	fmt.Println("PlayerLogin-------------------", ProtocolData)
	strtoken := ProtocolData["Tocken"].(string)

	// 去登录服务器验证 tocken是否有效
	retstr := impl.Get("http://127.0.0.1:4001/BaBaLiuLiu_Server?Protocol=10&Protocol2=3&Token=" + strtoken)
	fmt.Println("impl.Get", retstr)
	// 解析login server 返回的数据
	//
	// 整体数据结构解析
	STsend := &ProtoDSQ.L2G_Player_Login{}
	json.Unmarshal([]byte(retstr), STsend)

	// 发送客户端
	data := &Proto_DSQGame.G2C_Player_Login{
		Protocol:  10,
		Protocol2: 2,
		Bsucc:     true,
	}

	if !STsend.Isucc {
		data.Bsucc = false
		this.PlayerSendMessage(data)
		return
	}
	fmt.Println("STsend.PlayerData", STsend.PlayerData)
	fmt.Println("STsend.PlayerData.OpenID", STsend.PlayerData.OpenID)
	// 玩家数据解析
	// 服务器的数据连接信息保存
	// 1. 玩家的链接信息，conn , playerdata 等等 需要进行保存。
	// 2. 玩家的消息的推送，服务器通知玩家（服务器广播）。
	// 3. 网络信息的断线重来。sss
	// 数据保存
	onlineUser := &DSQGame{
		Connection: this.Connection,
		MapSafe:    this.MapSafe,
		Player:     STsend.PlayerData,
	}
	this.MapSafe.Put(STsend.PlayerData.OpenID+"|User", onlineUser)
	// 发送数据
	data.Player = STsend.PlayerData
	this.PlayerSendMessage(data)
	return
}
