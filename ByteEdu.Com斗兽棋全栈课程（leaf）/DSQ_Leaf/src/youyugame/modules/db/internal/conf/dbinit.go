package db_mod

import (
	"encoding/json"
	"io/ioutil"

	"github.com/name5566/leaf/log"
)

// 数据库配置
var Db_conf struct {
	Host string
}

func init() {
	data, err := ioutil.ReadFile("modules/db/internal/conf/conf_db.json")
	if err != nil {
		log.Fatal("%v", err)
	}
	err = json.Unmarshal(data, &Db_conf)
	if err != nil {
		log.Fatal("%v", err)
	}
}
