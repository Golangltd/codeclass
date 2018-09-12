package Proto

const (
	INIT_Proto              = iota
	C2S_Login_Proto         // C2S_Login_Proto == 1         用户登陆协议   http://local.bytedancing.com:8080?Proto=1&code="ssss"
	C2S_ExamList_Proto      // C2S_ExamList_Proto ==2       获取考试列表   http://local.bytedancing.com:8080?Proto=2&pagenum=0
	C2S_ExamSearch_Proto    // C2S_ExamSearch_Proto ==3     获取试卷列表   http://local.bytedancing.com:8080?Proto=3&Itype=(1:type类型搜索，2：关键字搜索)&keyword="彬哥"
	C2S_Personal_Proto      // C2S_Personal_Proto ==4       个人中心      http://local.bytedancing.com:8080?Proto=4&openid=""&Itype=(1:type类型我的考试，2：我的订单)
	C2S_AddExamData_Proto   // C2S_AddExamData_Proto == 5   用户登陆协议   http://local.bytedancing.com:8080?Proto=5&Name=&Type=&Price&Msg=&Time=
	C2S_ExamStatistic_Proto // C2S_ExamStatistic_Proto == 6 数据统计      http://local.bytedancing.com:8080?Proto=6&Num=10
)
