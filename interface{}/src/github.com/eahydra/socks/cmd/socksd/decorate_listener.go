package main

import "net"

type DecorateListener struct {
	listener   net.Listener
	decorators []ConnDecorator
}

func NewDecorateListener(listener net.Listener, ds ...ConnDecorator) *DecorateListener {
	l := &DecorateListener{
		listener: listener,
	}
	l.decorators = append(l.decorators, ds...)
	return l
}

func (s *DecorateListener) Accept() (net.Conn, error) {
	conn, err := s.listener.Accept()
	if err != nil {
		return nil, err
	}
	dconn, err := DecorateConn(conn, s.decorators...)
	if err != nil {
		conn.Close()
		return nil, err
	}
	return dconn, nil
}

func (s *DecorateListener) Close() error {
	return s.listener.Close()
}

func (s *DecorateListener) Addr() net.Addr {
	return s.listener.Addr()
}
