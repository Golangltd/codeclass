/*

第十节  Go语言 变量的作用域

概念：

已经声明标识符所表示的常量、类型、变量、函数或者包在源代码中的作用范围

变量 》》？？
1 局部变量
2 全局变量
3 函数定义中的变量（形式参数）


*/

package main

import (
	"fmt"
)

var b int = 10

func main() {
	var a, c = 0, 100
	fmt.Println(a, b)
	ret := sum(a, c)
	fmt.Println(ret)
}

func sum(a, b int) int {

	fmt.Println(a, b)
	return a + b
}
