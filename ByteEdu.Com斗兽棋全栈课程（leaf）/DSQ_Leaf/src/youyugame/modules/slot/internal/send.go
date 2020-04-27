package internal

import (
	"youyugame/modules/modules_conf"
)

// 同步发送，等待数据返回
func SyncSendOtherModules(key string, args ...interface{}) (interface{}, error) {
	if key == conf_modules.Modules.Slot {
		data, err := ChanRPC.Call1("slot_NewAgent", args)
		return data, err
	} else if key == conf_modules.Modules.Db {
		data, err := ChanRPC.Call1("db_NewAgent", args)
		return data, err
	}
	return nil, nil
}

// 异步发送，需要回调函数
func AsynSendOtherModules(key string, args ...interface{}) (interface{}, error) {

	if key == conf_modules.Modules.Slot {
		data, err := ChanRPC.Call1("slot_NewAgent", args)
		return data, err
	} else if key == conf_modules.Modules.Db {
		data, err := ChanRPC.Call1("db_NewAgent", args)
		return data, err
	}
	return nil, nil
}

// 异步发送,不等待返回
func GoSendOtherModules(key string, args ...interface{}) {
	if key == conf_modules.Modules.Slot {
		ChanRPC.Go("slot_NewAgent", args)
	} else if key == conf_modules.Modules.Db {
		ChanRPC.Go("db_NewAgent", args)
	}
}
