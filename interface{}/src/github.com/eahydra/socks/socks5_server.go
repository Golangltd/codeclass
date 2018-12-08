package socks

import "net"

// Socks5Server implements Socks5 Proxy Protocol(RFC 1928), just support CONNECT command.
type Socks5Server struct {
	forward Dialer
}

// NewSocks5Server return a new Socks5Server
func NewSocks5Server(forward Dialer) (*Socks5Server, error) {
	return &Socks5Server{
		forward: forward,
	}, nil
}

// Serve with net.Listener for new incoming clients.
func (s *Socks5Server) Serve(listener net.Listener) error {
	for {
		conn, err := listener.Accept()
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Temporary() {
				continue
			} else {
				return err
			}
		}

		go serveSocks5Client(conn, s.forward)
	}
}
