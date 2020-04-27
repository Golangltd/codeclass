package internal

import (
	"reflect"

	_ "github.com/name5566/leaf/gate"
)

func init() {
}

func handleMsg(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)
}
