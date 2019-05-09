package main

import (
	"fmt"
	"os"
)

/*
   作者：彬哥
   来自：LollipopGo游戏服务器架构
*/

func WriteFileData(data string) {

	f, err := os.OpenFile("ByteEdu.txt", os.O_APPEND|os.O_WRONLY, 0644)
	if err != nil {
		fmt.Println(err)
		f.Close()
		return
	}
	fmt.Fprintln(f, data)

	err = f.Close()
	if err != nil {
		fmt.Println(err)
		return
	}
}
