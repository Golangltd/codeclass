/*

  第十二节  Go语言指针

 变量是一种使用方便的占位符，用于引用计算机内存地址。
Go 语言中 取地址符号 &，放到变量的前面 例如 &a ; 取到a在内存中的地址。

知识点一：
1 什么叫指针？
一个指针变量指向了一个值的内存地址。
2 指针的声明？
类似变量和常量，格式：
var var_name *vat_type
var a *int
3 指针的使用
<1> 定义指针变量
<2> 为指针赋值
<3> 访问指针变量中指向地址的值
4 指针如何去内存地址的数据
*var_name

*/

package main

import (
	"fmt"
)

func main() {

	var a int = 10           // 声明实际变量
        var Golang语言社区 *int  // 声明指针变量
	fmt.Println("变量的地址：%x", &a)
        Golang语言社区 = &a      // 指针变量的存储地址
        fmt.Println("变量的地址：%x", Golang语言社区)
        fmt.Println("变量的地址：%x", *Golang语言社区)
}











