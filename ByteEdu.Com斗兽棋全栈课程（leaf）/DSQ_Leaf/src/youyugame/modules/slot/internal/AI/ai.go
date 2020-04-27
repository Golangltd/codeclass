package ai

import (
	"fmt"
	"math/rand"
	"time"
)

// 每次都需要给前端
var slot_pan = [15]int{}
var slot_pan_Bak = [3][5]int{}

// 类型
const (
	invalid  = iota // 初始化
	weapon          // 武器
	being           // 人物
	property        // 道具
	wild            // 通用牌
)

// 道具
const (
	GOAT      = iota // 0  goat weapon   times: [0, 0, 2, 5, 20, 50]
	ZEBRA            // 1  zebra weapon   times: [0, 0, 3, 10, 40, 100]
	BULL             // 2  bull weapon   times: [0, 0, 5, 15, 60, 200]
	FOX              // 3  fox being    times: [0, 0, 7, 20, 150, 250]
	CROCODILE        // 4  crocodile being    times: [0, 0, 10, 30, 160, 400]
	SNAKE            // 5  snake being    times: [0, 0, 15, 40, 200, 500]
	LEOPARD          // 6  leopard property times: [0, 0, 20, 80, 400, 1000]
	ELEPGANT         // 7  elephant property times: [0, 0, 50, 200, 1000, 2500]
	LION             // 8  lion wild     times: [0, 0, 0, 0, 2000, 5000]
)

// 获取随机数
func init() {
	rand1 := rand.New(rand.NewSource(time.Now().UnixNano()))
	for i := 0; i < 15; i++ {
		irand := rand1.Intn(10)
		slot_pan[i] = irand
	}
	fmt.Println("Slot:", slot_pan)
	ij := 0
	for ii := 0; ii < 3; ii++ {
		for iii := 0; iii < 5; iii++ {
			slot_pan_Bak[ii][iii] = slot_pan[ij]
			ij++
		}
	}
	fmt.Println("slot_pan_Bak:", slot_pan_Bak)
}

// 创建开始后的结果
func CreateChessData() [15]int {
	rand := rand.New(rand.NewSource(time.Now().UnixNano()))
	for i := 0; i < 15; i++ {
		irand := rand.Intn(10)
		slot_pan[i] = irand
	}
	fmt.Println("Slot:", slot_pan)
	return slot_pan
}

// 获取倍率
func GetIconBeiLv(itype, ixian int) int {
	if itype == GOAT {
		iarry := []int{0, 0, 2, 5, 20, 50}
		return iarry[ixian-1]
	} else if itype == ZEBRA {
		iarry := []int{0, 0, 3, 10, 40, 100}
		return iarry[ixian-1]
	} else if itype == BULL {
		iarry := []int{0, 0, 5, 15, 60, 200}
		return iarry[ixian-1]
	} else if itype == FOX {
		iarry := []int{0, 0, 7, 20, 150, 250}
		return iarry[ixian-1]
	} else if itype == CROCODILE {
		iarry := []int{0, 0, 10, 30, 160, 400}
		return iarry[ixian-1]
	} else if itype == SNAKE {
		iarry := []int{0, 0, 15, 40, 200, 500}
		return iarry[ixian-1]
	} else if itype == LEOPARD {
		iarry := []int{0, 0, 20, 80, 400, 1000}
		return iarry[ixian-1]
	} else if itype == ELEPGANT {
		iarry := []int{0, 0, 50, 200, 1000, 2500}
		return iarry[ixian-1]
	} else if itype == LION {
		iarry := []int{0, 0, 0, 0, 2000, 5000}
		return iarry[ixian-1]
	}
	return 0
}

/*
  获取线数的算法
   0 0 0 0 0
   0 0 0 0 0
   0 0 0 0 0

*/

// 获取线数，例如是
// 一维数组，递归获取数据对比
func GetLineNUM() {
	// 获取数据
	ij := 0
	for ii := 0; ii < 3; ii++ {
		for iii := 0; iii < 5; iii++ {
			slot_pan_Bak[ii][iii] = slot_pan[ij]
			ij++
		}
	}
}
