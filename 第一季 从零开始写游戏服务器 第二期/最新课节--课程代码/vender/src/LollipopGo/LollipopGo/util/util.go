package util

import (
	"strconv"
	"time"
)

// package main

// import (
//     "fmt"
//     "strconv"
//     "time"
// )

// func main() {
//     t := time.Now()
//     fmt.Println(t)

//     fmt.Println(t.UTC().Format(time.UnixDate))

//     fmt.Println(t.Unix())

//     timestamp := strconv.FormatInt(t.UTC().UnixNano(), 10)
//     fmt.Println(timestamp)
//     timestamp = timestamp[:10]
//     fmt.Println(timestamp)
// }

// 输出：
// 2017-06-21 11:52:29.0826692 + 0800 CST
// Wed Jun 21 03:52:29 UTC 2017
// 1498017149
// 1498017149082669200
// 1498017149

// 生成时间戳的函数
func CreateTime() string {
	t := time.Now()
	return strconv.FormatInt(t.UTC().UnixNano(), 10)
}
