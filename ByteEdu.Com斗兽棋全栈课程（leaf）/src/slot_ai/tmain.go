package main

import (
	"fmt"
	"math/rand"
	"time"
)

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
	goat      = iota // 0  weapon
	zebra            // 1  weapon
	bull             // 2  weapon
	fox              // 3  being
	crocodile        // 4  being
	snake            // 5  being
	leopard          // 6  property
	elephant         // 7  property
	lion             // 8  wild
)

//------------------------------------------------------------------------------

var slot_pan = [15]int{}

func main() {

}

// 获取随机数
func init() {
	rand1 := rand.New(rand.NewSource(time.Now().UnixNano()))
	for i := 0; i < 15; i++ {
		irand := rand1.Intn(10)
		slot_pan[i] = irand
	}
	fmt.Println("Slot:", slot_pan)
}

/*
  0 0 0 0 0
  0 0 0 0 0
  0 0 0 0 0
*/

/*
全盘奖倍率
export let SlotsOverallTimes = {
icons: [50, 100, 150, 250, 400, 500, 1000, 2500, 5000],
types: [15, 50],
}
*/
