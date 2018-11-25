package proxyclient

import (
	"bytes"
	"errors"
	"fmt"
	"net"
	"strings"
	"time"
)

type directTCPConn struct {
	net.TCPConn
	splitHttp   bool      // 是否拆分HTTP包
	wTime       time.Time // 可写时间
	proxyClient *directProxyClient
}

type directUDPConn struct {
	net.UDPConn
	proxyClient ProxyClient
}
type directProxyClient struct {
	TCPLocalAddr net.TCPAddr
	UDPLocalAddr net.UDPAddr
	splitHttp    bool
	sleep        time.Duration
	query        map[string][]string
}

func directInit() {

}

// 创建代理客户端
// 直连 direct://0.0.0.0:0000/?LocalAddr=123.123.123.123:0
// SplitHttp                拆分http请求到多个TCP包
func newDriectProxyClient(localAddr string, splitHttp bool, sleep time.Duration, query map[string][]string) (ProxyClient, error) {
	if localAddr == "" {
		localAddr = ":0"
	}

	tcpAddr, err := net.ResolveTCPAddr("tcp", localAddr)
	if err != nil {
		return nil, errors.New("LocalAddr 错误的格式")
	}

	udpAddr, err := net.ResolveUDPAddr("udp", localAddr)
	if err != nil {
		return nil, errors.New("LocalAddr 错误的格式")
	}

	return &directProxyClient{*tcpAddr, *udpAddr, splitHttp, sleep, query}, nil
}

func (p *directProxyClient) Dial(network, address string) (net.Conn, error) {
	if strings.HasPrefix(network, "tcp") {
		addr, err := net.ResolveTCPAddr(network, address)
		if err != nil {
			return nil, fmt.Errorf("地址解析错误:%v", err)
		}
		return p.DialTCP(network, &p.TCPLocalAddr, addr)
	} else if strings.HasPrefix(network, "udp") {
		addr, err := net.ResolveUDPAddr(network, address)
		if err != nil {
			return nil, fmt.Errorf("地址解析错误:%v", err)
		}
		return p.DialUDP(network, &p.UDPLocalAddr, addr)
	} else {
		return nil, errors.New("未知的 network 类型。")
	}
}

func (p *directProxyClient) DialTimeout(network, address string, timeout time.Duration) (net.Conn, error) {
	switch network {
	case "tcp", "tcp4", "tcp6":
	case "udp", "udp4", "udp6":
	default:
		return nil, fmt.Errorf("不支持的 network 类型:%v", network)
	}

	d := net.Dialer{Timeout: timeout, LocalAddr: &p.TCPLocalAddr}
	conn, err := d.Dial(network, address)
	if err != nil {
		return nil, err
	}

	splitHttp := false
	if p.splitHttp {
		raddr, err := net.ResolveTCPAddr(network, address)
		if err == nil && raddr.Port == 80 {
			splitHttp = true
		}
	}
	wTime := time.Time{}
	if p.sleep != 0 {
		wTime = time.Now()
		wTime = wTime.Add(p.sleep)
	}

	switch conn := conn.(type) {
	case *net.TCPConn:
		return &directTCPConn{*conn, splitHttp, wTime, p}, nil
	case *net.UDPConn:
		return &directUDPConn{*conn, p}, nil
	default:
		return nil, fmt.Errorf("内部错误：未知的连接类型。")
	}
}

func (p *directProxyClient) DialTCP(network string, laddr, raddr *net.TCPAddr) (net.Conn, error) {
	if laddr == nil {
		laddr = &p.TCPLocalAddr
	}
	conn, err := net.DialTCP(network, laddr, raddr)
	if err != nil {
		return nil, err
	}

	splitHttp := false
	if p.splitHttp && raddr.Port == 80 {
		splitHttp = true
	}
	wTime := time.Time{}
	if p.sleep != 0 {
		wTime = time.Now()
		wTime = wTime.Add(p.sleep)
	}

	return &directTCPConn{*conn, splitHttp, wTime, p}, nil
}

func (p *directProxyClient) DialTCPSAddr(network string, raddr string) (ProxyTCPConn, error) {
	return p.DialTCPSAddrTimeout(network, raddr, 0)
}

// DialTCPSAddrTimeout 同 DialTCPSAddr 函数，增加了超时功能
func (p *directProxyClient) DialTCPSAddrTimeout(network string, raddr string, timeout time.Duration) (rconn ProxyTCPConn, rerr error) {
	switch network {
	case "tcp", "tcp4", "tcp6":
	default:
		return nil, fmt.Errorf("不支持的 network 类型:%v", network)
	}
	d := net.Dialer{Timeout: timeout, LocalAddr: &p.TCPLocalAddr}
	conn, err := d.Dial(network, raddr)
	if err != nil {
		return nil, err
	}

	splitHttp := false
	if p.splitHttp {
		raddr, err := net.ResolveTCPAddr(network, raddr)
		if err == nil && raddr.Port == 80 {
			splitHttp = true
		}
	}
	wTime := time.Time{}
	if p.sleep != 0 {
		wTime = time.Now()
		wTime = wTime.Add(p.sleep)
	}

	if tcpConn, ok := conn.(*net.TCPConn); ok {
		return &directTCPConn{*tcpConn, splitHttp, wTime, p}, nil
	}
	return nil, fmt.Errorf("内部错误")
}

func (p *directProxyClient) DialUDP(network string, laddr, raddr *net.UDPAddr) (net.Conn, error) {
	if laddr == nil {
		laddr = &p.UDPLocalAddr
	}
	conn, err := net.DialUDP(network, laddr, raddr)
	if err != nil {
		return nil, err
	}
	return &directUDPConn{*conn, p}, nil
}
func (p *directProxyClient) UpProxy() ProxyClient {
	return nil
}
func (p *directProxyClient) SetUpProxy(upProxy ProxyClient) error {
	return errors.New("直连不支持上层代理。")
}
func (c *directTCPConn) ProxyClient() ProxyClient {
	return c.proxyClient
}

// 拆分 http 请求
// 查找 'GET', 'HEAD', 'PUT', 'POST', 'TRACE', 'OPTIONS', 'DELETE', 'CONNECT' 及 HTTP、HOST
func SplitHttp(b []byte) (res [][]byte) {
	split := func(b []byte, i int) [][]byte {
		// 根据 i的值拆分成为 2 个 []byte 。
		// 注意，允许 i < len(b)
		if len(b) > i {
			return [][]byte{b[:i], b[i:]}
		}
		return [][]byte{b}
	}

	for i, v := range b {
		switch v {
		case 'G':
			if bytes.HasPrefix(b[i+1:], []byte("ET ")) {
				res = split(b, i+1)
				res = append([][]byte{res[0]}, split(res[1], 3)...)

				return append(res[:len(res)-1], SplitHttp(res[len(res)-1])...)
			}
		case 'P':
			if bytes.HasPrefix(b[i+1:], []byte("OST ")) {
				res = split(b, i+1)
				res = append([][]byte{res[0]}, split(res[1], 5)...)

				return append(res[:len(res)-1], SplitHttp(res[len(res)-1])...)
			}
		case 'C':
			if bytes.HasPrefix(b[i+1:], []byte("ONNECT ")) {
				res = split(b, i+1)
				res = append([][]byte{res[0]}, split(res[1], 8)...)

				return append(res[:len(res)-1], SplitHttp(res[len(res)-1])...)
			}
		case 'H':
			if bytes.HasPrefix(b[i+1:], []byte("OST:")) {
				res = split(b, i+1)
				res = append([][]byte{res[0]}, split(res[1], 8)...)

				return append(res[:len(res)-1], SplitHttp(res[len(res)-1])...)
			}
			if bytes.HasPrefix(b[i+1:], []byte("TTP")) {
				res = split(b, i+1)
				res = append([][]byte{res[0]}, split(res[1], 9)...)

				return append(res[:len(res)-1], SplitHttp(res[len(res)-1])...)
			}
		}
	}
	return [][]byte{b}
}

func (c *directTCPConn) Write(b []byte) (n int, err error) {
	if c.wTime.IsZero() == false {
		c.wTime = time.Time{}
		time.Sleep(c.wTime.Sub(time.Now()))
	}

	if c.splitHttp == false {
		return c.TCPConn.Write(b)
	}

	newBuffs := SplitHttp(b)
	for _, buf := range newBuffs {
		ln, lerr := c.TCPConn.Write(buf)
		n += ln
		if lerr != nil {
			return n, lerr
		}
	}
	return n, nil
}
func (c *directUDPConn) ProxyClient() ProxyClient {
	return c.proxyClient
}
func (p *directProxyClient) GetProxyAddrQuery() map[string][]string {
	return p.query
}
