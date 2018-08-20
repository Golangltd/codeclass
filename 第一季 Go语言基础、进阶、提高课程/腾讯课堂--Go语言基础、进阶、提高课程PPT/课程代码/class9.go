/*

第九节  Go语言  函数

函数 定义：
基本的代码块，用于执行一个任务。

注意点：

1 Go语言编程过程，如果项目或者工程执行，最少有一个main()
2 可以每个package 下面可以有一个main（）

知识点一 函数的定义：

func  func_name(parameter list) return_types{

 // 函数体

  return return_types
}

*/

package main

import (
	"fmt"
)

func main() {
	ret := Golang_Ltd(10)
	fmt.Println(ret)
}

func Golang_Ltd(a int) int {
	return a
}
