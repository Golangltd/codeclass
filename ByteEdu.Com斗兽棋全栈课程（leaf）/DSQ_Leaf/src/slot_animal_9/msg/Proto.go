package msg

//------------------------------------------------------------------------------
/*
   游戏服务器消息处理
   客户端请求
*/
type UserEntryGameReq struct {
	Timestamp   int64
	UserID      uint64
	Token       string
	Version     int32  //客户端版本，现在传0就行
	IsReconnect bool   //如果用户是点击返回游戏进来的，会将上一局游戏结果(108)推送过去，然后用户要点击继续游戏才会变成准备状态；
	IP          string //客户端上传ip
}

// 服务器返回
type UserEntryGameResp struct {
	Bsucc bool // 是否成功
}

//------------------------------------------------------------------------------
/*
  玩家开始游戏
*/
type UserYaZhuReq struct {
	Timestamp int64
	UserID    int32 // UID信息
	CoinNum   int64 // 压注的总金额
}

//服务器 返回
type UserYaZhuResp struct {
	// 结果数据
}

//------------------------------------------------------------------------------
