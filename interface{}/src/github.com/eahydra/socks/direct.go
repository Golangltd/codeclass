package socks

import "net"

// A Dialer is a means to establish a connection.
type Dialer interface {
	// Dial connects to the given address via the proxy.
	Dial(network, address string) (net.Conn, error)
}

type direct struct{}

// Direct is a direct proxy which implements Dialer interface: one that makes connections directly.
var Direct = direct{}

func (direct) Dial(network, address string) (net.Conn, error) {
	return net.Dial(network, address)
}
