/*
变量定义：
计算机语言中存储结果或者表示值的抽象感念，变量可以通过变量名访问。
 GO语言变量的规则：
字母，数字，下划线，其中首字母不能为数字
var var_name var_type

1 指定变量的类型，声明后若不复制，使用默认值。
var var_name var_type
var_name = value

2 根据值自动判定变量的类型
var var_name = value

多变量的声明

var var_name1 var_name2 var_name3 var_type
var var_name1 var_name2 var_name3 = 1,"22",true
var (
 var_name1 var_type1
 var_name2 var_type2
 var_name3 var_type3
)

*/

package main

import (
	"fmt"
)

var a = "Golang语言社区"
var b = "www.Golang.LTD"
var c bool
var d = "123456789"
var f,g,h int

func main() {
	fmt.Println(a, b, c, d)

}
