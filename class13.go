/*

Go语言指针

1 变量的地址 如何取？
2 什么是指针？指针和变量地址的联系？
指针的概念：
指向一个值的内存地址
3 指针的一个格式？
var var_name *var_type
var ip *int  指向整形的*
4 如何使用指针？
 <1> 定义指针变量
 <2> 为指针变量赋值
 <3> 访问指针变量中指向的地址的数据或者值
5 指针如何取地址？
  *var_name
补充知识：
空指针
 1 什么是空指针？
  当一个指针被定义后没有分配到任何变量时，他的值为nil
  nil 指针称为空指针
  nil 在概念上和其他语言的 null,none,nil NULL 一样；都是指代零值或空值
  一个指针变量通常缩写为 ptr
*/

package main

import (
	"fmt"
)

func main() {
	var a int = 10 // 声明实际变量
	var ip *int    // 声明的指针变量
	ip = &a        // 指针的变量的存储地址

	fmt.Println("a 变量的地址：%x", &a)
	fmt.Println("ip 变量的地址：%x", ip)
	fmt.Println("ip 变量的值：%d", *ip)

}
