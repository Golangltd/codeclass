/*

第八节  Go语言 循环语句

for 作为例子

// 1 循环
for i:= 0;i<100;i++{

}

// 2 死循环

for {

}

// 3 map slice and so on

*/

package main

import (
	"fmt"
)

func main() {

	for i := 0; i < 100; i++ {
		fmt.Println("Golang语言社区",i)
	}

}
