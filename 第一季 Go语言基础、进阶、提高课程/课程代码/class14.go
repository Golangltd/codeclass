/*

本节知识点 如下：
1 什么叫结构体？
：是一系列具有相同类型或者不同类型的数据构成的数据集合

2 结构体的 格式：
 type struct_name struct{
  membername1 type // int bool
  membername2 type
  membername3 type
  ...
  membername4 type
}

3 如何访问一个结构体的成员：
结构体(name).成员名字

4 如何给结构体赋值？
构体(name).成员名字 = 数值（类型要与定义结构体的成员的类型相符）


知识拓展：
1 结构体作为函数参数

规则：
func func_Name( name  struct_name){

}

2 结构体指针
规则：
var struct_ptr *struct_name
struct_ptr = &struct_name

*/

package main

import (
	"fmt"
)

type Golang语言社区 struct {
	wwwgolangltd int
	golangltd    string
}

func init(){

}


func main() {
	var Golang语言 Golang语言社区 // 声明 Golang语言 为 Golang语言社区 类型的变量
	Golang语言.wwwgolangltd = 8888
	Golang语言.golangltd = "url:www.Golang.Ltd"
	fmt.Println("Golang语言社区网址：", Golang语言.golangltd)
        Golangltd(&Golang语言)
}

func Golangltd(golang *Golang语言社区) {

	fmt.Println("Golang语言社区吉利数字：", golang.wwwgolangltd)

}
