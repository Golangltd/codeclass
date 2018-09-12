package DB

import (
	"binge/ST_data"
	"database/sql"
	"fmt"
	"glog-master"
	"strconv"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

func InsertExam_Info(strName, strType, strPrice, strMsg, strTime string) int {

	stmt, err := GetMySQL().Prepare("INSERT INTO t_ExamData(Name,Type,Price,Msg,Createtime,Time) VALUES(?, ?, ?, ?, ?, ?)")
	defer stmt.Close()
	if err != nil {
		glog.Info(err.Error())
		return 0
	}
	//时间戳到具体显示的转化
	iLastLoginTime := time.Now().Unix()
	if stmt == nil {
		glog.Info("InsertExam_Info stmt is nil!!!")
		return 0
	}
	// 传入对应的参数
	var res sql.Result
	res, err = stmt.Exec(strName, strType, strPrice, strMsg, iLastLoginTime, strTime)
	if err != nil {
		glog.Info(err.Error())
		return 0
	}
	glog.Info(res)
	stmt.Close()
	glog.Info("stmt.Close()!!!!")
	return int(1)
}

// 读取首页列表
func Read_Exam_List(strPageNum string) (mapGameInfo map[string]*ST_Data.ExamST) {
	// pagenum if pagenum == 0 --- > 获取最新的数据
	// pagenum > 0  pagenum -- 最大的一个考试的数据  ---> 获取第几页的数据
	// 每页的数据量 ---> 5个考试的item

	mapGameInfo = make(map[string]*ST_Data.ExamST)
	strSql := ""
	if strPageNum == "0" {
		strSql = "select * from t_ExamData where id ORDER BY id DESC LIMIT 5 "
	} else {
		strSql = "select * from t_ExamData where id < " + strPageNum + " ORDER BY id DESC LIMIT 5 "
	}
	Rows, err := getMySQL().Query(strSql)
	if err != nil {
		glog.Info("Read_Exam_List err:" + err.Error())
		return nil
	}
	//glog.Info(Rows)
	var iTmp int64
	iTmp = 0
	for Rows.Next() {
		iTmp++
		gameinfo := new(ST_Data.ExamST)
		Rows.Scan(&gameinfo.ID, &gameinfo.Name, &gameinfo.Type, &gameinfo.Price, &gameinfo.Msg, &gameinfo.CreateTime, &gameinfo.Time)
		mapGameInfo[strconv.Itoa(gameinfo.ID)] = gameinfo
	}
	if len(mapGameInfo) == 0 {
		glog.Info("mapGameInfo[1] is nil!!!")
		return nil
	}
	fmt.Println()
	return mapGameInfo

}

// 搜题
func Read_ExamSearch_List(strType, strKeyword string) (mapGameInfo map[string]*ST_Data.ExamST) {

	mapGameInfo = make(map[string]*ST_Data.ExamST)
	strSql := ""
	if strType == "1" {
		strSql = "select * from t_ExamData where Type = '" + strKeyword + "'"

	} else if strType == "2" {
		// 模糊搜索 %%
		strSql = "select * from t_ExamData where Name LIKE '%" + strKeyword + "%'"
	}
	if len(strSql) == 0 {
		return
	}
	Rows, err := getMySQL().Query(strSql)
	if err != nil {
		glog.Info("Read_Exam_List err:" + err.Error())
		return nil
	}

	var iTmp int64
	iTmp = 0
	for Rows.Next() {
		iTmp++
		gameinfo := new(ST_Data.ExamST)
		Rows.Scan(&gameinfo.ID, &gameinfo.Name, &gameinfo.Type, &gameinfo.Price, &gameinfo.Msg, &gameinfo.CreateTime, &gameinfo.Time)
		mapGameInfo[strconv.Itoa(gameinfo.ID)] = gameinfo
	}
	if len(mapGameInfo) == 0 {
		glog.Info("mapGameInfo[1] is nil!!!")
		return nil
	}
	fmt.Println()
	return mapGameInfo
}

// 获取用户的考试列表
func Read_UserExamList_List(strType, stropenid string) (mapGameInfo map[string]*ST_Data.ExamST) {

	return nil
}
