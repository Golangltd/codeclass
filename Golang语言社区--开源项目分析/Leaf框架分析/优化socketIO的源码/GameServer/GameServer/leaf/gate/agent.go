package gate

import (
	"net"
)

type Agent interface {
	WriteMsg(msg interface{})
	Emit(method string, args interface{}) error
	LocalAddr() net.Addr
	RemoteAddr() net.Addr
	Close()
	Destroy()
	UserData() interface{}
	SetUserData(data interface{})
}
