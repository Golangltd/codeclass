package main

import (
	"fmt"
	"net/http"
)

/*
  登录服务器的函数
*/

func IndexHandler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintln(w, "hello world")
	// 需要处理 get 请求等
	// 1 base64 操作 ---
}
