/*

第七节  Go语言 条件语句
概念：
需要开发者指定一个或者多个条件，并通过测试条件是否为true来决定
是否执行指定语句。

编码的规则：

if condition {

}else{

}

*/

package main

import (
	"fmt"
)

var Golang语言社区 string = "www.Golang.Ltd"

func main() {

	if Golang语言社区 == "Golang" {
		fmt.Println("非 Golang语言社区网址")
	} else {
		fmt.Println("Golang语言社区网址", Golang语言社区)

	}
}
