package proxyclient

import (
	"errors"
	"fmt"
	"net"
	"strings"
	"time"

	ss "github.com/shadowsocks/shadowsocks-go/shadowsocks"
)

type ssTCPConn struct {
	TCPConn
	sc          Conn
	proxyClient ProxyClient
}

type ssUDPConn struct {
	net.UDPConn
	proxyClient ProxyClient
}
type ssProxyClient struct {
	proxyAddr string
	cipher    *ss.Cipher
	upProxy   ProxyClient
	query     map[string][]string
}

func newSsProxyClient(proxyAddr, method, password string, upProxy ProxyClient, query map[string][]string) (ProxyClient, error) {
	p := ssProxyClient{}

	cipher, err := ss.NewCipher(method, password)
	if err != nil {
		return nil, fmt.Errorf("创建代理错误：%v", err)
	}

	if upProxy == nil {
		nUpProxy, err := newDriectProxyClient("", false, 0, make(map[string][]string))
		if err != nil {
			return nil, fmt.Errorf("创建直连代理错误：%v", err)
		}
		upProxy = nUpProxy
	}

	p.proxyAddr = proxyAddr
	p.cipher = cipher
	p.upProxy = upProxy
	p.query = query

	return &p, nil
}

func (p *ssProxyClient) Dial(network, address string) (net.Conn, error) {
	if strings.HasPrefix(strings.ToLower(network), "tcp") {
		return p.DialTCPSAddr(network, address)
	} else if strings.HasPrefix(strings.ToLower(network), "udp") {
		addr, err := net.ResolveUDPAddr(network, address)
		if err != nil {
			return nil, fmt.Errorf("地址解析错误:%v", err)
		}
		return p.DialUDP(network, nil, addr)
	} else {
		return nil, errors.New("未知的 network 类型。")
	}
}

func (p *ssProxyClient) DialTimeout(network, address string, timeout time.Duration) (net.Conn, error) {
	switch network {
	case "tcp", "tcp4", "tcp6":
		return p.DialTCPSAddrTimeout(network, address, timeout)
	case "udp", "udp4", "udp6":
		return nil, errors.New("暂不支持UDP协议")
	default:
		return nil, errors.New("未知的协议")
	}
}

func (p *ssProxyClient) DialTCP(network string, laddr, raddr *net.TCPAddr) (net.Conn, error) {
	if laddr != nil || laddr.Port != 0 {
		return nil, errors.New("代理协议不支持指定本地地址。")
	}

	return p.DialTCPSAddr(network, raddr.String())
}

func (p *ssProxyClient) DialTCPSAddr(network string, raddr string) (ProxyTCPConn, error) {
	return p.DialTCPSAddrTimeout(network, raddr, 0)
}

func (p *ssProxyClient) DialTCPSAddrTimeout(network string, raddr string, timeout time.Duration) (rconn ProxyTCPConn, rerr error) {
	// 截止时间
	finalDeadline := time.Time{}
	if timeout != 0 {
		finalDeadline = time.Now().Add(timeout)
	}

	ra, err := ss.RawAddr(raddr)
	if err != nil {
		return
	}

	c, err := p.upProxy.DialTCPSAddrTimeout(network, p.proxyAddr, timeout)
	if err != nil {
		return nil, fmt.Errorf("无法连接代理服务器 %v ，错误：%v", p.proxyAddr, err)
	}
	ch := make(chan int)
	defer close(ch)

	// 实际执行部分
	run := func() {
		sc := ss.NewConn(c, p.cipher.Copy())

		closed := false
		// 当连接不被使用时，ch<-1会引发异常，这时将关闭连接。
		defer func() {
			e := recover()
			if e != nil && closed == false {
				sc.Close()
			}
		}()

		if _, err := sc.Write(ra); err != nil {
			closed = true
			sc.Close()
			rerr = err
			ch <- 0
			return
		}

		r := ssTCPConn{TCPConn: c, sc: sc, proxyClient: p} //{c,net.ResolveTCPAddr("tcp","0.0.0.0:0"),net.ResolveTCPAddr("tcp","0.0.0.0:0"),"","",0,0  p}

		rconn = &r
		ch <- 1
	}

	if timeout == 0 {
		go run()

		select {
		case <-ch:
			return
		}
	} else {
		c.SetDeadline(finalDeadline)

		ntimeout := finalDeadline.Sub(time.Now())
		if ntimeout <= 0 {
			return nil, fmt.Errorf("timeout")
		}
		t := time.NewTimer(ntimeout)
		defer t.Stop()

		go run()

		select {
		case <-t.C:
			return nil, fmt.Errorf("连接超时。")
		case <-ch:
			if rerr == nil {
				c.SetDeadline(time.Time{})
			}
			return
		}
	}
}

func (p *ssProxyClient) DialUDP(network string, laddr, raddr *net.UDPAddr) (conn net.Conn, err error) {
	return nil, errors.New("暂不支持 UDP 协议")
}

func (p *ssProxyClient) UpProxy() ProxyClient {
	return p.upProxy
}

func (p *ssProxyClient) SetUpProxy(upProxy ProxyClient) error {
	p.upProxy = upProxy
	return nil
}

func (c *ssTCPConn) ProxyClient() ProxyClient {
	return c.proxyClient
}
func (c *ssTCPConn) Write(b []byte) (n int, err error) {
	return c.sc.Write(b)
}
func (c *ssTCPConn) Read(b []byte) (n int, err error) {
	return c.sc.Read(b)
}

func (c *ssTCPConn) Close() error {
	return c.sc.Close()
}

func (c *ssUDPConn) ProxyClient() ProxyClient {
	return c.proxyClient
}

func (p *ssProxyClient) GetProxyAddrQuery() map[string][]string {
	return p.query
}
