/*


    第十八节 Go语言 接口

1 接口？
Go设计哲学，不是传统的面向对象的语言，因此就没有类继承的感念。
接口：就是一组方法的集合。

2 接口定义？

type st_name struct{

}

type i_name interface{
   method1(param_list)return_ist
   method2(param_list)return_ist
   ... ... 
}

3 如何调用接口？

 var I_name I_ST
 var ST_name ST_
  I_name = new(ST_name)
  ST_name 要求必须实现我们 I_ST 的方法

4 接口拓展？

func func_name(i interface{}){

}

interface == *void 

*/


package main

import(
"fmt"
)


// 定义接口
type Igolang interface{
     GET()
     POST()
}

// 定义结构体
type STgolang struct{

}

func (this *STgolang)GET(){

}

func (this *STgolang)POST(){

}

func GolangLTD(i interface{}){
   _=i

   fmt.Println(i)
}

func main(){
    // 调用
    var  igolang  Igolang
    igolang = new(STgolang)
    _=igolang
    igolang.GET()
    igolang.POST()
      GolangLTD(102020)
      GolangLTD("www.golang.ltd")
}
