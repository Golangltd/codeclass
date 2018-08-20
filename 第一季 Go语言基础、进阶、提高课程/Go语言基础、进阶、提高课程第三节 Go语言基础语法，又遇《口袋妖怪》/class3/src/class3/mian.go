 package structTest  
      
    //普通函数与方法的区别（在接收者分别为值类型和指针类型的时候）  
      
    import (  
        "fmt"  
    )  
      
    func StructTest06Base() {  
        structTest0601()  
        structTest0602()  
    }  
      
    //1.普通函数  
    //接收值类型参数的函数  
    func valueIntTest(a int) int {  
        return a + 10  
    }  
      
    //接收指针类型参数的函数  
    func pointerIntTest(a *int) int {  
        return *a + 10  
    }  
      
    func structTest0601() {  
        a := 2  
        fmt.Println("valueIntTest:", valueIntTest(a))  
        //函数的参数为值类型，则不能直接将指针作为参数传递  
        //fmt.Println("valueIntTest:", valueIntTest(&a))  
        //compile error: cannot use &a (type *int) as type int in function argument  
      
        b := 5  
        fmt.Println("pointerIntTest:", pointerIntTest(&b))  
        //同样，当函数的参数为指针类型时，也不能直接将值类型作为参数传递  
        //fmt.Println("pointerIntTest:", pointerIntTest(b))  
        //compile error:cannot use b (type int) as type *int in function argument  
    }  
      
    //2.方法  
    type PersonD struct {  
        id   int  
        name string  
    }  
      
    //接收者为值类型  
    func (p PersonD) valueShowName() {  
        fmt.Println(p.name)  
    }  
      
    //接收者为指针类型  
    func (p *PersonD) pointShowName() {  
        fmt.Println(p.name)  
    }  
      
    func structTest0602() {  
        //值类型调用方法  
        personValue := PersonD{101, "Will Smith"}  
        personValue.valueShowName()  
        personValue.pointShowName()  
      
        //指针类型调用方法  
        personPointer := &PersonD{102, "Paul Tony"}  
        personPointer.valueShowName()  
        personPointer.pointShowName()  
      
        //与普通函数不同，接收者为指针类型和值类型的方法，指针类型和值类型的变量均可相互调用  
    }
End