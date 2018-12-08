package main

import "net"

type ConnDecorator func(net.Conn) (net.Conn, error)

func DecorateConn(conn net.Conn, ds ...ConnDecorator) (net.Conn, error) {
	decorated := conn
	var err error
	for _, decorate := range ds {
		decorated, err = decorate(decorated)
		if err != nil {
			return nil, err
		}
	}
	return decorated, nil
}
