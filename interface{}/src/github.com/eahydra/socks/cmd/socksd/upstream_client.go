package main

import (
	"net"
	"sync/atomic"

	"github.com/eahydra/socks"
)

type UpstreamDialer struct {
	nextRouter     uint64
	forwardDialers []socks.Dialer
}

func NewUpstreamDialer(forwardDialers []socks.Dialer) *UpstreamDialer {
	return &UpstreamDialer{
		forwardDialers: forwardDialers,
	}
}

func (u *UpstreamDialer) getNextDialer() socks.Dialer {
	old := atomic.AddUint64(&u.nextRouter, 1)
	return u.forwardDialers[old%uint64(len(u.forwardDialers))]
}

func (u *UpstreamDialer) Dial(network, address string) (net.Conn, error) {
	router := u.getNextDialer()
	conn, err := router.Dial(network, address)
	if err != nil {
		ErrLog.Println("UpstreamDialer router.Dial failed, err:", err, network, address)
		return nil, err
	}
	return conn, nil
}
