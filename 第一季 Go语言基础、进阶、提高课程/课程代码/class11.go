/*

 第十一节 Go语言 数组


知识点：

1  数组的概念：
   具有相同唯一类型的一组已编号且长度固定的数据项序列。

2 声明数组：
var var_name [size]var_type

3 数组初始化：
var var_name =  [size]var_type{1,2,3,4}
 
4 访问数组

 方式：通过索引来读取，
 格式：var_name1 :=var_name[0]

知识拓展：
1 数组长度：
len(var_name)
*/

package main

import(

"fmt"
)

var Golang语言社区 = [100]string{"www.Golang.Ltd","url"}


func main(){

  for i:=0;i<len(Golang语言社区);i++{
  fmt.Println(Golang语言社区[i])
}

}

