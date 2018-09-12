package main

import (
	"fmt"
	"net/http"
)

// 程序入口
func main() {
	//http 服务器启用
	http.HandleFunc("/", examfunc)
	err := http.ListenAndServe(":8080", nil)
	if err != nil {
		fmt.Println(err.Error())
		return
	}
	return
}
