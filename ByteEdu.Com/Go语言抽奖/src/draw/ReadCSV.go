package main

import (
	"encoding/csv"
	"fmt"
	"io/ioutil"
	"strconv"
	"strings"
)

/*
   作者：彬哥
   来自：LollipopGo游戏服务器架构
*/

// 结构信息
type ByteEdu struct {
	ID   string
	Name string
}

// 保存到数据的结构
var G_ByteEdu map[int]*ByteEdu

func init() {
	// 初始化 map
	G_ByteEdu = make(map[int]*ByteEdu)
	// 读取配置表数据
	ReadCsv_BaoMingData_Fun()
}

// 读取报名人的信息
func ReadCsv_BaoMingData_Fun() bool {
	fileName := "ByteEdu.csv"
	fileName = "./csv/" + fileName
	cntb, err := ioutil.ReadFile(fileName)
	if err != nil {
		panic("读取配置文件出错!")
		return false
	}
	// 读取文件数据
	r2 := csv.NewReader(strings.NewReader(string(cntb)))
	ss, _ := r2.ReadAll()
	sz := len(ss)
	for i := 1; i < sz; i++ {
		Infotmp := new(ByteEdu)
		Infotmp.ID = ss[i][0]
		Infotmp.Name = ss[i][1]
		iid, err := strconv.Atoi(Infotmp.ID)
		if err != nil {
			continue
		}
		G_ByteEdu[iid] = Infotmp
	}
	fmt.Println("抽奖名单：", G_ByteEdu)
	return true
}
