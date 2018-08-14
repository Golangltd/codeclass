/*

type interface_name interface{
   method_name1[return_type]
   method_name2[return_type]
   ...
   method_namen[return_type]
}

type struct_name struct{

   id int

}

*/

package main

import (
	"fmt"
)

type Phone interface {
	call()
}

type NokiaPhone struct {
}

func (this NokiaPhone) call() {
	fmt.Println("I am Nokia, Ican call you!")
}

func main() {

	var phone Phone

	phone = new(NokiaPhone)
         _=phone
	phone.call()

}
