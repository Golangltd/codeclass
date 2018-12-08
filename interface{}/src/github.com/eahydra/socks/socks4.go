package socks

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"net"
	"strconv"
)

const (
	socks4Version       = 4
	socks4Connect       = 1
	socks4Granted       = 90
	socks4Rejected      = 91
	socks4ConnectFailed = 92
	socks4UserIDInvalid = 93
)

var socks4Errors = []string{
	"",
	"request rejected or failed",
	"request rejected because SOCKS server cannot connect to identd on the client",
	"request rejected because the client program and identd report different user-ids",
}

// Socks4Server implements Socks4 Proxy Protocol(http://www.openssh.com/txt/socks4.protocol).
// Just support CONNECT command.
type Socks4Server struct {
	forward Dialer
}

// NewSocks4Server returns a new Socks4Server that can serve from new clients.
func NewSocks4Server(forward Dialer) (*Socks4Server, error) {
	return &Socks4Server{
		forward: forward,
	}, nil
}

// Serve with net.Listener for clients.
func (s *Socks4Server) Serve(listener net.Listener) error {
	for {
		conn, err := listener.Accept()
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Temporary() {
				continue
			} else {
				return err
			}
		}

		go serveSOCKS4Client(conn, s.forward)
	}
}

// Socks4Client implements Socks4 Proxy Protocol(http://www.openssh.com/txt/socks4.protocol).
type Socks4Client struct {
	network string
	address string
	userID  string
	forward Dialer
}

// NewSocks4Client return a new Socks4Client that implements Dialer interface.
// network must be supported by forward, address is proxy server's address, userID can empty.
func NewSocks4Client(network, address, userID string, forward Dialer) (*Socks4Client, error) {
	return &Socks4Client{
		network: network,
		address: address,
		userID:  userID,
		forward: forward,
	}, nil
}

// Dial return a new net.Conn if succeeded. network must be tcp, tcp4 or tcp6, address only is IPV4.
func (s *Socks4Client) Dial(network, address string) (net.Conn, error) {
	switch network {
	case "tcp", "tcp4", "tcp6":
	default:
		return nil, errors.New("socks: no support for SOCKS4 proxy connections of type:" + network)
	}

	host, portStr, err := net.SplitHostPort(address)
	if err != nil {
		return nil, err
	}
	port, err := strconv.Atoi(portStr)
	if err != nil {
		return nil, errors.New("socks: failed to parse port:" + portStr)
	}
	if port < 1 || port > 0xffff {
		return nil, errors.New("socks: port number out of range:" + portStr)
	}
	ip := net.ParseIP(host)
	if ip == nil {
		return nil, errors.New("socks: destination host invalid:" + host)
	}
	ip4 := ip.To4()
	if ip4 == nil {
		return nil, errors.New("socks:destination ip must be ipv4:" + host)
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

	buff := make([]byte, 0, 8+len(s.userID)+1)
	buff = append(buff, socks4Version, socks4Connect)
	buff = append(buff, byte(port>>8), byte(port))
	buff = append(buff, ip4...)
	if len(s.userID) != 0 {
		buff = append(buff, []byte(s.userID)...)
	}
	buff = append(buff, 0)

	if _, err := conn.Write(buff); err != nil {
		return nil, errors.New("socks: failed to write connect request to SOCKS4 server at: " + s.address + ": " + err.Error())
	}
	buff = buff[:8]
	if _, err := io.ReadFull(conn, buff); err != nil {
		return nil, errors.New("socks: failed to read connect reply from SOCKS4 server at: " + s.address + ": " + err.Error())
	}
	if buff[1] != socks4Granted {
		cd := int(buff[1]) - socks4Granted
		failure := "unknown error"
		if cd < len(socks4Errors) && cd >= 0 {
			failure = socks4Errors[cd]
		}
		return nil, errors.New("socks: SOCKS4 server at " + s.address + " failed to connect: " + failure)
	}

	closeConn = nil
	return conn, nil
}

func serveSOCKS4Client(conn net.Conn, forward Dialer) {
	defer conn.Close()

	reader := bufio.NewReader(conn)
	buff, err := reader.Peek(9)
	if err != nil {
		return
	}
	if buff[8] != 0 {
		if _, err = reader.ReadSlice(0); err != nil {
			return
		}
	}

	reply := make([]byte, 8)
	if buff[0] != socks4Version {
		reply[1] = socks4Rejected
		conn.Write(reply)
		return
	}
	if buff[1] != socks4Connect {
		reply[1] = socks4Rejected
		conn.Write(reply)
		return
	}

	port := uint16(buff[2])<<8 | uint16(buff[3])
	ip := buff[4:8]

	host := fmt.Sprintf("%d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3], port)
	dest, err := forward.Dial("tcp4", host)
	if err != nil {
		reply[1] = socks4ConnectFailed
		conn.Write(reply)
		return
	}
	defer dest.Close()

	reply[1] = socks4Granted
	if _, err = conn.Write(reply); err != nil {
		return
	}

	go func() {
		defer conn.Close()
		defer dest.Close()
		io.Copy(dest, conn)
	}()
	io.Copy(conn, dest)
}
