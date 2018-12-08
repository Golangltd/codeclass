package main

import (
	"fmt"
	"math/rand"
	"time"
)

// 四川麻将，普通的
// 胡牌类型

const (
	NoHU        = 0  //不胡
	PingHU      = 1  //平胡；四坎牌，加一对将，得一分。
	DuiDuiHU    = 2  //对对胡；每坎牌都是三张一样的牌，即111万、222条，得两分。
	QiangGangHU = 3  //抢杠胡；它人杠的时候，抢牌胡，得两分。
	QingYiSe    = 4  //清一色；全手牌是一种花色，即123条、567条、234 条、888条、99条，得三分。
	YaoJiuPai   = 5  //幺九牌；每坎牌都是一或九，即123条、123万、789条、789万、99万，得三分。
	QiDui       = 6  //七对；胡牌的时候是七对牌，即11万、22万、99万、44筒、66筒、88筒、99筒，得三分。
	QingQiDui   = 7  //清七对；在七对的基础上，有两对牌是四张一样的。（注意:此四张牌并不是杠的牌） 即11万、11万、99万、44筒、66筒、88筒、99筒，得四分。
	QiDui2      = 8  //清对；一种花色的大对子。即111万、444万、222万、999万、66万，得四分。
	JiangDui    = 9  //将对；即二、五、八的大对子，即222万 555万 888万 222条 55筒，的四分。
	TianHU      = 10 //天胡；即刚码好牌就自然成胡，得8分。
	DiHU        = 11 //地胡；即刚打第一张牌就成胡，得8分。
)

//四川麻将牌
var (
	SiChuangMJArr = []int{
		101, 102, 103, 104, 105, 106, 107, 108, 109, //#万
		101, 102, 103, 104, 105, 106, 107, 108, 109,
		101, 102, 103, 104, 105, 106, 107, 108, 109,
		101, 102, 103, 104, 105, 106, 107, 108, 109,
		201, 202, 203, 204, 205, 206, 207, 208, 209, //#饼
		201, 202, 203, 204, 205, 206, 207, 208, 209,
		201, 202, 203, 204, 205, 206, 207, 208, 209,
		201, 202, 203, 204, 205, 206, 207, 208, 209,
		301, 302, 303, 304, 305, 306, 307, 308, 309, //#条
		301, 302, 303, 304, 305, 306, 307, 308, 309,
		301, 302, 303, 304, 305, 306, 307, 308, 309,
		301, 302, 303, 304, 305, 306, 307, 308, 309,
	}
)

// 函　数：生成随机数
// 概　要：
// 参　数：
//      min: 最小值
//      max: 最大值
// 返回值：
//      int64: 生成的随机数
func RandInt64(min, max int64) int64 {
	if min >= max || min == 0 || max == 0 {
		return max
	}
	return rand.Int63n(max-min) + min
}

// 玩家结构体
type PlayerST struct {
	PlayerUID int64   // 玩家的UID 唯一
	PaiData   [20]int // 玩家拥有的牌的数组,暂时可以定义大小20
	RoomUID   int     // 房间的ID，这个ID可以是临时也是暂时的
}

var PlayerMapData map[int64]*PlayerST // 定义玩家的存储结构

// 初始化发牌
func initFaPai() {

	FaPaiCiShu := 53              // 初始化牌型 为循环随机53次;可能实际要大于53次，随机的次数在具体根据实际情况
	SuiJiMap := make(map[int]int) // 记录随机数
	for i := 0; i < FaPaiCiShu; i++ {
		WeiZhi := int(RandInt64(1, 108))
		//WeiZhi := rand.Intn(107)
		_, ok := SuiJiMap[WeiZhi]
		if ok {
			fmt.Println("随机重复：", WeiZhi)
			FaPaiCiShu++
			continue
		}
		fmt.Println(WeiZhi)
		SuiJiMap[WeiZhi] = WeiZhi
	}

	fmt.Println("循环次数", FaPaiCiShu)
}

func init() {
	fmt.Println("entry init")
	//记录开始时间
	t1 := time.Now()
	initFaPai()
	//记录结束时间
	elapsed := time.Since(t1)
	fmt.Println("App elapsed: ", elapsed)
	return
}

func main() {

	return
}
