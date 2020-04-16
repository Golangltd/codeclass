package main

import (
	"LollipopGo/LollipopGo/util"
	"XZ_server/Proto"
	"fmt"
)

/*

------------------------------------
|   [0,0]       [1,0]      [2,0]      [3,0]   |
|                                             |
|   [0,1]       [1,1]      [2,1]      [3,1]   |
|                                             |
|   [0,2]       [1,2]      [2,2]      [3,2]   |
|                                             |
|   [0,3]       [1,3]      [2,3]      [3,3]   |
|-----------------------------------
                服务器棋牌设计
*/

/*
  棋牌的初始化数据
  1-16   1-8 = A  9-18 = B

  初始化棋牌的思路 如下：
  1. 服务器产生随机数，0-16
  2. 通过第1步骤的随机数，去DSQ_QI数组获取值，例如 3  --> num := DSQ_QI[3]  （删除获取的位置的信息）
  3. 通过第2步骤 生成的棋子 num,填充 服务器二维数组

  注意点：
  1. 服务器要首先生成，二维数组（空的）
  2. 注意浅拷贝和深拷贝的问题 copy()
*/
var DSQ_QI = []int{
	Proto_DSQGame.Elephant,
	Proto_DSQGame.Lion,
	Proto_DSQGame.Tiger,
	Proto_DSQGame.Leopard,
	Proto_DSQGame.Wolf,
	Proto_DSQGame.Dog,
	Proto_DSQGame.Cat,
	Proto_DSQGame.Mouse,
	Proto_DSQGame.Elephant + Proto_DSQGame.Mouse,
	Proto_DSQGame.Lion + Proto_DSQGame.Mouse,
	Proto_DSQGame.Tiger + Proto_DSQGame.Mouse,
	Proto_DSQGame.Leopard + Proto_DSQGame.Mouse,
	Proto_DSQGame.Wolf + Proto_DSQGame.Mouse,
	Proto_DSQGame.Dog + Proto_DSQGame.Mouse,
	Proto_DSQGame.Cat + Proto_DSQGame.Mouse,
	Proto_DSQGame.Mouse + Proto_DSQGame.Mouse,
}

func init() {
	//InitDSQ(DSQ_QI)
}

// 初始化函数
// 作业： 如何填充二维数据
func InitDSQ(data1 []int) [4][4]int {

	datatmp, x, y := [4][4]int{}, 0, 0
	// data := data1
	data := make([]int, len(data1))
	copy(data, data1)

	for i := 0; i < 2*Proto_DSQGame.Mouse; i++ {
		icount := util.RandInterval_LollipopGo(0, int32(len(data)-1))
		//		fmt.Println("随机数：------", icount)
		/* icount == 1   len(data) == 16
		   开发过程中：可以将二维数组--理解一维数组操作
		   x,y
		   当y的数值增加到我们定义的数组长度后x才累加
		*/
		if int(icount) < len(data) {
			datatmp[x][y] = data[icount]
			y++
			if y%4 == 0 {
				x++
				y = 0
			}
			// 前闭后开区间
			// data:=[]int{0,1,2,3,4,5,6}
			// num = 2
			// data[:num] ={0,1}
			// data[num+1:] ={3,4,5,6}
			// append --> {0,1,3,4,5,6}
			data = append(data[:icount], data[icount+1:]...)
		} else {
			datatmp[x][y] = data[icount]
			y++
			if y%4 == 0 {
				x++
				y = 0
			}
			data = data[:icount-1]
		}
	}
	fmt.Println("datatmp------", datatmp)
	//	fmt.Println("datatmp[0][0]------", datatmp[0][0])
	//	fmt.Println("datatmp[0][0]------", datatmp[1][0])
	/*
		[
		  [6 8 11 5]
		  [12 10 15 3]
		  [16 1 2 14]
		  [4 7 13 9]
		]
	*/
	return datatmp
}
