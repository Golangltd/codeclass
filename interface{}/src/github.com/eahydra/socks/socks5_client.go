package socks

import (
	"errors"
	"io"
	"net"
	"strconv"
)

const (
	socks5Version = 5

	socks5AuthNone     = 0
	socks5AuthPassword = 2
	socks5AuthNoAccept = 0xff

	socks5AuthPasswordVer = 1

	socks5Connect = 1

	socks5IP4    = 1
	socks5Domain = 3
	socks5IP6    = 4
)

const (
	socks5Success                 = 0
	socks5GeneralFailure          = 1
	socks5ConnectNotAllowed       = 2
	socks5NetworkUnreachable      = 3
	socks5HostUnreachable         = 4
	socks5ConnectionRefused       = 5
	socks5TTLExpired              = 6
	socks5CommandNotSupported     = 7
	socks5AddressTypeNotSupported = 8
)

var socks5Errors = []string{
	"",
	"general SOCKS server failure",
	"connection not allowed by ruleset",
	"network unreachable",
	"Host unreachable",
	"Connection refused",
	"TTL expired",
	"Command not supported",
	"Address type not supported",
}

// Socks5Client implements Socks5 Proxy Protocol(RFC 1928) Client Protocol.
// Just support CONNECT command, and support USERNAME/PASSWORD authentication methods(RFC 1929)
type Socks5Client struct {
	network  string
	address  string
	user     string
	password string
	forward  Dialer
}

// NewSocks5Client return a new Socks5Client that implements Dialer interface.
func NewSocks5Client(network, address, user, password string, forward Dialer) (*Socks5Client, error) {
	return &Socks5Client{
		network:  network,
		address:  address,
		user:     user,
		password: password,
		forward:  forward,
	}, nil
}

// Dial return a new net.Conn that through the CONNECT command to establish connections with proxy server.
// address as RFC's requirements that can be IPV4, IPV6 and domain host, such as 8.8.8.8:999 or google.com:80
func (s *Socks5Client) Dial(network, address string) (net.Conn, error) {
	switch network {
	case "tcp", "tcp4", "tcp6":
	default:
		return nil, errors.New("socks: no support for SOCKS5 proxy connections of type:" + network)
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

	host, portStr, err := net.SplitHostPort(address)
	if err != nil {
		return nil, err
	}

	// check port
	port, err := strconv.Atoi(portStr)
	if err != nil {
		return nil, errors.New("socks: failed to parse port number: " + portStr)
	}
	if port < 1 || port > 0xffff {
		return nil, errors.New("socks: port number out of range: " + portStr)
	}

	buff := make([]byte, 0, 6+len(host))

	buff = append(buff, socks5Version)

	// set authentication methods
	if len(s.user) > 0 && len(s.user) < 256 && len(s.password) < 256 {
		buff = append(buff, 2, socks5AuthNone, socks5AuthPassword)
	} else {
		buff = append(buff, 1, socks5AuthNone)
	}

	// send authentication methods
	if _, err := conn.Write(buff); err != nil {
		return nil, errors.New("socks: failed to write handshake request at: " + s.address + ": " + err.Error())
	}
	if _, err := io.ReadFull(conn, buff[:2]); err != nil {
		return nil, errors.New("socks: failed to read handshake reply at: " + s.address + ": " + err.Error())
	}

	// handle authentication methods reply
	if buff[0] != socks5Version {
		return nil, errors.New("socks: SOCKS5 server at: " + s.address + " invalid version" + strconv.Itoa(int(buff[0])))
	}
	if buff[1] == socks5AuthNoAccept {
		return nil, errors.New("socks: SOCKS server at: " + s.address + " no acceptable methods")
	}

	if buff[1] == socks5AuthPassword {
		// build username/password authentication request
		buff = buff[:0]
		buff = append(buff, socks5AuthPasswordVer)
		buff = append(buff, uint8(len(s.user)))
		buff = append(buff, []byte(s.user)...)
		buff = append(buff, uint8(len(s.password)))
		buff = append(buff, []byte(s.password)...)

		if _, err := conn.Write(buff); err != nil {
			return nil, errors.New("socks: failed to write password authentication request to SOCKS5 server at: " + s.address + ": " + err.Error())
		}
		if _, err := io.ReadFull(conn, buff[:2]); err != nil {
			return nil, errors.New("socks: failed to read password authentication reply from SOCKS5 server at: " + s.address + ": " + err.Error())
		}
		// 0 indicates success
		if buff[1] != 0 {
			return nil, errors.New("socks: SOCKS5 server at: " + s.address + " reject username/password")
		}
	}

	// build connect request
	buff = buff[:0]
	buff = append(buff, socks5Version, socks5Connect, 0)

	if ip := net.ParseIP(host); ip != nil {
		if ip4 := ip.To4(); ip4 != nil {
			buff = append(buff, socks5IP4)
			ip = ip4
		} else {
			buff = append(buff, socks5IP6)
		}
		buff = append(buff, ip...)
	} else {
		if len(host) > 255 {
			return nil, errors.New("socks: destination hostname too long: " + host)
		}
		buff = append(buff, socks5Domain)
		buff = append(buff, uint8(len(host)))
		buff = append(buff, host...)
	}
	buff = append(buff, byte(port>>8), byte(port))

	if _, err := conn.Write(buff); err != nil {
		return nil, errors.New("socks: failed to write connect request to SOCKS5 server at: " + s.address + ": " + err.Error())
	}
	if _, err := io.ReadFull(conn, buff[:4]); err != nil {
		return nil, errors.New("socks: failed to read connect reply from SOCKS5 server at: " + s.address + ": " + err.Error())
	}

	failure := "unknown error"
	if int(buff[1]) < len(socks5Errors) {
		failure = socks5Errors[buff[1]]
	}
	if len(failure) > 0 {
		return nil, errors.New("socks: SOCKS5 server failed to connect: " + failure)
	}

	// read remain data include BIND.ADDRESS and BIND.PORT
	discardBytes := 0
	switch buff[3] {
	case socks5IP4:
		discardBytes = net.IPv4len
	case socks5IP6:
		discardBytes = net.IPv6len
	case socks5Domain:
		if _, err := io.ReadFull(conn, buff[:1]); err != nil {
			return nil, errors.New("socks: failed to read domain length from SOCKS5 server at: " + s.address + ": " + err.Error())
		}
		discardBytes = int(buff[0])
	default:
		return nil, errors.New("socks: got unknown address type " + strconv.Itoa(int(buff[3])) + " from SOCKS5 server at: " + s.address)
	}
	discardBytes += 2
	if cap(buff) < discardBytes {
		buff = make([]byte, discardBytes)
	} else {
		buff = buff[:discardBytes]
	}
	if _, err := io.ReadFull(conn, buff); err != nil {
		return nil, errors.New("socks: failed to read address and port from SOCKS5 server at: " + s.address + ": " + err.Error())
	}

	closeConn = nil
	return conn, nil
}

func serveSocks5Client(conn net.Conn, forward Dialer) {
	defer conn.Close()

	buff := make([]byte, 262)
	reply := []byte{0x05, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22}

	if _, err := io.ReadFull(conn, buff[:2]); err != nil {
		return
	}
	if buff[0] != socks5Version {
		reply[1] = socks5AuthNoAccept
		conn.Write(reply[:2])
		return
	}
	numMethod := buff[1]
	if _, err := io.ReadFull(conn, buff[:numMethod]); err != nil {
		return
	}
	reply[1] = socks5AuthNone
	if _, err := conn.Write(reply[:2]); err != nil {
		return
	}

	if _, err := io.ReadFull(conn, buff[:4]); err != nil {
		return
	}
	if buff[1] != socks5Connect {
		reply[1] = socks5CommandNotSupported
		conn.Write(reply)
		return
	}

	addressType := buff[3]
	addressLen := 0
	switch addressType {
	case socks5IP4:
		addressLen = net.IPv4len
	case socks5IP6:
		addressLen = net.IPv6len
	case socks5Domain:
		if _, err := io.ReadFull(conn, buff[:1]); err != nil {
			return
		}
		addressLen = int(buff[0])
	default:
		reply[1] = socks5AddressTypeNotSupported
		conn.Write(reply)
		return
	}
	host := make([]byte, addressLen)
	if _, err := io.ReadFull(conn, host); err != nil {
		return
	}
	if _, err := io.ReadFull(conn, buff[:2]); err != nil {
		return
	}
	hostStr := ""
	switch addressType {
	case socks5IP4, socks5IP6:
		ip := net.IP(host)
		hostStr = ip.String()
	case socks5Domain:
		hostStr = string(host)
	}
	port := uint16(buff[0])<<8 | uint16(buff[1])
	if port < 1 || port > 0xffff {
		reply[1] = socks5HostUnreachable
		conn.Write(reply)
		return
	}
	portStr := strconv.Itoa(int(port))

	hostStr = net.JoinHostPort(hostStr, portStr)
	dest, err := forward.Dial("tcp", hostStr)
	if err != nil {
		reply[1] = socks5ConnectionRefused
		conn.Write(reply)
		return
	}
	defer dest.Close()
	reply[1] = socks5Success
	if _, err := conn.Write(reply); err != nil {
		return
	}

	go func() {
		defer conn.Close()
		defer dest.Close()
		io.Copy(conn, dest)
	}()

	io.Copy(dest, conn)
}
