package socks

import (
	"errors"
	"net"
	"strconv"
)

// ShadowSocksClient implements ShadowSocks Proxy Protocol
type ShadowSocksClient struct {
	network string
	address string
	forward Dialer
}

// NewShadowSocksClient return a new ShadowSocksClient that implements Dialer interface.
func NewShadowSocksClient(network, address string, forward Dialer) (*ShadowSocksClient, error) {
	return &ShadowSocksClient{
		network: network,
		address: address,
		forward: forward,
	}, nil
}

// Dial return a new net.Conn that through proxy server establish with address
func (s *ShadowSocksClient) Dial(network, address string) (net.Conn, error) {
	switch network {
	case "tcp", "tcp4", "tcp6":
	default:
		return nil, errors.New("socks: no support ShadowSocks proxy connections of type: " + network)
	}

	host, portStr, err := net.SplitHostPort(address)
	if err != nil {
		return nil, err
	}
	port, err := strconv.Atoi(portStr)
	if err != nil {
		return nil, errors.New("socks: failed to parse port number:" + portStr)
	}
	if port < 1 || port > 0xffff {
		return nil, errors.New("socks5: port number out of range:" + portStr)
	}

	conn, err := s.forward.Dial(s.network, s.address)
	if err != nil {
		return nil, err
	}
	closeConn := &conn
	defer func() {
		if closeConn != nil {
			(*closeConn).Close()
		}
	}()

	buff := make([]byte, 0, 266)
	if ip := net.ParseIP(host); ip != nil {
		if ip4 := ip.To4(); ip4 != nil {
			buff = append(buff, 1)
			ip = ip4
		} else {
			buff = append(buff, 4)
		}
		buff = append(buff, ip...)
	} else {
		if len(host) > 255 {
			return nil, errors.New("socks: destination hostname too long: " + host)
		}
		buff = append(buff, 3)
		buff = append(buff, uint8(len(host)))
		buff = append(buff, host...)
	}
	buff = append(buff, uint8(port>>8), uint8(port))

	_, err = conn.Write(buff)
	if err != nil {
		return nil, err
	}

	closeConn = nil
	return conn, nil
}
