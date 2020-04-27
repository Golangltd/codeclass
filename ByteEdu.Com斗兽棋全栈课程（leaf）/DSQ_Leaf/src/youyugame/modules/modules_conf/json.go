package conf_modules

import (
	"encoding/json"
	"io/ioutil"

	"fmt"

	"github.com/name5566/leaf/log"
)

var Modules struct {
	Slot      string
	MsgCenter string
	Login     string
	Gate      string
	Db        string
}

func init() {
	fmt.Println("init:------:")
	data, err := ioutil.ReadFile("modules/modules_conf/modules_conf.json")
	if err != nil {
		log.Fatal("%v", err)
	}
	err = json.Unmarshal(data, &Modules)
	if err != nil {
		log.Fatal("%v", err)
	}
	fmt.Println("init:------:", data)
}
