/*
常量的定义:
const var_name var_type = value

1 显式类型定义 ：
const golangltd string = "www.Golang.Ltd"
2 隐式类型定义 ：
const golangltd  = "www.Golang.Ltd"
3 常量可以作为枚举使用：
  const (
   Unknown = 0
   Golang = 1
   GolangLtd = 2
)
4 iota  特殊的常量 ，可以认为是一个可以被编译器修改的常量
特点： 每一个const关键字出现时，被重置为0，然后再下一个const出现之前
每次出现iota,其所代表的数字会自动增加1
 const (
  a = iota
  b = iota
  c = iota
)

 const (
  a = iota
  b
  c
)

*/

package main

import (
	"fmt"
)

func main() {
	const LENGTH int = 10
	const WIDTH int = 5

	var area int

	area = LENGTH * WIDTH
	fmt.Println("面积为：", area)

	const (
		a = iota
		b
		c
		d = "ha"
		e
		f = 100
		g
		h = iota
		i
	)
	fmt.Println(a, b, c, d, e, f, g, h, i)

}
