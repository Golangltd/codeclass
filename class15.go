/*


Go语言 切片（slice）
知识点：
1 何为 切片？？
<1> 本身是对数组的抽象
<2> 切片大小可以改变的，而数组是不可以改变的
<3> 追加元素

2 如何定义切片？
规则：

var var_name []type_name  int bool

3 切片使用？

 <1> 初始化操作
  make
  var_name = make([]type_name，len)
 <2> 切片的数据存储
  append()
 <3> 切片大小
 len()

*/

package main

import (
	"fmt"
)

func main() {
	var golang语言社区 []string
	golang语言社区 = make([]string, 1000)
	fmt.Println("增加前：", len(golang语言社区))
	golang语言社区 = append(golang语言社区, "Goalng.Ltd")
	fmt.Println(golang语言社区)
	fmt.Println("增加后：", len(golang语言社区))
}
