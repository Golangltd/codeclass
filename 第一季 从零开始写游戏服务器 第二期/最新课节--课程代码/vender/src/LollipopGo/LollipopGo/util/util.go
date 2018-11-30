package util

import (
	"crypto/md5"
	"encoding/hex"
	"strconv"
	"time"
)

//------------------------------------------------------------------------------

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
func UTCTime_LollipopGO() string {
	t := time.Now()
	return strconv.FormatInt(t.UTC().UnixNano(), 10)
}

//------------------------------------------------------------------------------
// package main

// import (
//     "crypto/md5"
//     "encoding/hex"
//     "fmt"
// )

// func main() {
//     h := md5.New()
//     h.Write([]byte("123456")) // 需要加密的字符串为 123456
//     cipherStr := h.Sum(nil)
//     fmt.Println(cipherStr)
//     fmt.Printf("%s\n", hex.EncodeToString(cipherStr)) // 输出加密结果
// }

// MD5 实现 :主要是针对 字符串的加密
func MD5_LollipopGO(data string) string {
	h := md5.New()
	h.Write([]byte(data))
	cipherStr := h.Sum(nil)
	return hex.EncodeToString(cipherStr)
}

//------------------------------------------------------------------------------

// package main

// import (
//     "encoding/json"
//     "fmt"
//     "os"
// )

// type ConfigStruct struct {
//     Host              string   `json:"host"`
//     Port              int      `json:"port"`
//     AnalyticsFile     string   `json:"analytics_file"`
//     StaticFileVersion int      `json:"static_file_version"`
//     StaticDir         string   `json:"static_dir"`
//     TemplatesDir      string   `json:"templates_dir"`
//     SerTcpSocketHost  string   `json:"serTcpSocketHost"`
//     SerTcpSocketPort  int      `json:"serTcpSocketPort"`
//     Fruits            []string `json:"fruits"`
// }

// type Other struct {
//     SerTcpSocketHost string   `json:"serTcpSocketHost"`
//     SerTcpSocketPort int      `json:"serTcpSocketPort"`
//     Fruits           []string `json:"fruits"`
// }

// func main() {
//     jsonStr := `{"host": "http://localhost:9090","port": 9090,"analytics_file": "","static_file_version": 1,"static_dir": "E:/Project/goTest/src/","templates_dir": "E:/Project/goTest/src/templates/","serTcpSocketHost": ":12340","serTcpSocketPort": 12340,"fruits": ["apple", "peach"]}`

//     //json str 转map
//     var dat map[string]interface{}
//     if err := json.Unmarshal([]byte(jsonStr), &dat); err == nil {
//         fmt.Println("==============json str 转map=======================")
//         fmt.Println(dat)
//         fmt.Println(dat["host"])
//     }

//     //json str 转struct
//     var config ConfigStruct
//     if err := json.Unmarshal([]byte(jsonStr), &config); err == nil {
//         fmt.Println("================json str 转struct==")
//         fmt.Println(config)
//         fmt.Println(config.Host)
//     }

//     //json str 转struct(部份字段)
//     var part Other
//     if err := json.Unmarshal([]byte(jsonStr), &part); err == nil {
//         fmt.Println("================json str 转struct==")
//         fmt.Println(part)
//         fmt.Println(part.SerTcpSocketPort)
//     }

//     //struct 到json str
//     if b, err := json.Marshal(config); err == nil {
//         fmt.Println("================struct 到json str==")
//         fmt.Println(string(b))
//     }

//     //map 到json str
//     fmt.Println("================map 到json str=====================")
//     enc := json.NewEncoder(os.Stdout)
//     enc.Encode(dat)

//     //array 到 json str
//     arr := []string{"hello", "apple", "python", "golang", "base", "peach", "pear"}
//     lang, err := json.Marshal(arr)
//     if err == nil {
//         fmt.Println("================array 到 json str==")
//         fmt.Println(string(lang))
//     }

//     //json 到 []string
//     var wo []string
//     if err := json.Unmarshal(lang, &wo); err == nil {
//         fmt.Println("================json 到 []string==")
//         fmt.Println(wo)
//     }
// }
