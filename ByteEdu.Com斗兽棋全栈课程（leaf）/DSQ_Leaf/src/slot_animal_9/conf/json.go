package conf

import (
	"encoding/json"
	"io/ioutil"

	"fmt"

	"github.com/name5566/leaf/log"
)

// 服务器配置
var Server struct {
	LogLevel    string
	LogPath     string
	WSAddr      string
	CertFile    string
	KeyFile     string
	TCPAddr     string
	MaxConnNum  int
	ConsolePort int
	ProfilePath string
}

// 获取服务器地址
func init() {
	fmt.Println("init:------:")
	data, err := ioutil.ReadFile("conf/server.json")
	if err != nil {
		log.Fatal("%v", err)
	}
	err = json.Unmarshal(data, &Server)
	if err != nil {
		log.Fatal("%v", err)
	}
	fmt.Println("init:------:", data)
}
