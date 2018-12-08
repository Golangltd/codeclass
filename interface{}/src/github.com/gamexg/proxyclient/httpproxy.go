package proxyclient

import (
	"bufio"
	"crypto/rand"
	"crypto/tls"
	"encoding/base64"
	"errors"
	"fmt"
	"io"
	"math/big"
	srand "math/rand"
	"net"
	"net/http"
	"strings"
	"sync"
	"time"
)

type httpTCPConn struct {
	Conn                            //http 协议时是原始链接、https协议时是tls.Conn
	rawConn               TCPConn   //原始链接
	tlsConn               *tls.Conn //tls链接
	localAddr, remoteAddr net.TCPAddr
	localHost, remoteHost string
	LocalPort, remotePort uint16
	proxyClient           ProxyClient
	r                     io.ReadCloser // http Body
}

type httpProxyClient struct {
	proxyAddr          string
	proxyDomain        string // 用于ssl证书验证
	proxyType          string // socks4 socks5
	auth               string
	insecureSkipVerify bool
	standardHeader     bool
	upProxy            ProxyClient
	query              map[string][]string
}

// 创建代理客户端
// ProxyType				http https
// ProxyAddr 				127.0.0.1:5555
// proxyDomain				ssl 验证域名，"" 则使用 proxyAddr 部分的域名
// insecureSkipVerify		使用https代理时是否忽略证书检查
// UpProxy
func newHTTPProxyClient(proxyType string, proxyAddr string, proxyDomain string, auth string, insecureSkipVerify bool, StandardHeader bool, upProxy ProxyClient, query map[string][]string) (ProxyClient, error) {
	proxyType = strings.ToLower(strings.Trim(proxyType, " \r\n\t"))
	if proxyType != "http" && proxyType != "https" {
		return nil, errors.New("ProxyType 错误的格式，只支持http、https代理。")
	}

	if upProxy == nil {
		nUpProxy, err := newDriectProxyClient("", false, 0, make(map[string][]string))
		if err != nil {
			return nil, fmt.Errorf("创建直连代理错误：%v", err)
		}
		upProxy = nUpProxy
	}

	if proxyDomain == "" {
		host, _, err := net.SplitHostPort(proxyAddr)
		if err != nil {
			return nil, fmt.Errorf("proxyAddr 格式错误：%v", err)
		}
		proxyDomain = host
	}

	return &httpProxyClient{proxyAddr, proxyDomain, proxyType, auth, insecureSkipVerify, StandardHeader, upProxy, query}, nil
}

func (p *httpProxyClient) Dial(network, address string) (net.Conn, error) {
	if strings.HasPrefix(strings.ToLower(network), "tcp") {
		return p.DialTCPSAddr(network, address)
	} else if strings.HasPrefix(strings.ToLower(network), "udp") {
		return nil, errors.New("不支持UDP协议。")
	} else {
		return nil, errors.New("未知的 network 类型。")
	}
}

func (p *httpProxyClient) DialTimeout(network, address string, timeout time.Duration) (net.Conn, error) {
	switch network {
	case "tcp", "tcp4", "tcp6":
		return p.DialTCPSAddrTimeout(network, address, timeout)
	default:
		return nil, fmt.Errorf("不支持的协议")
	}
}

func (p *httpProxyClient) DialTCP(network string, laddr, raddr *net.TCPAddr) (net.Conn, error) {
	if laddr != nil || laddr.Port != 0 {
		return nil, errors.New("代理协议不支持指定本地地址。")
	}

	return p.DialTCPSAddr(network, raddr.String())
}

func (p *httpProxyClient) DialTCPSAddr(network string, raddr string) (ProxyTCPConn, error) {
	return p.DialTCPSAddrTimeout(network, raddr, 0)
}

func (p *httpProxyClient) DialTCPSAddrTimeout(network string, raddr string, timeout time.Duration) (ProxyTCPConn, error) {
	var rconn ProxyTCPConn
	var rerr error
	rMutex := sync.Mutex{}

	// 截止时间
	finalDeadline := time.Time{}
	if timeout != 0 {
		finalDeadline = time.Now().Add(timeout)
	}

	var tlsConn *tls.Conn
	rawConn, err := p.upProxy.DialTCPSAddrTimeout(network, p.proxyAddr, timeout)
	if err != nil {
		return nil, fmt.Errorf("无法连接代理服务器 %v ，错误：%v", p.proxyAddr, err)
	}

	c := Conn(rawConn)

	ch := make(chan int)

	// 实际执行部分
	run := func() {
		closed := false
		// 当连接不被使用时，ch<-1会引发异常，这时将关闭连接。
		defer func() {
			e := recover()
			if e != nil && closed == false {
				c.Close()
			}
		}()

		if p.proxyType == "https" {
			tlsConn = tls.Client(c, &tls.Config{ServerName: p.proxyDomain, InsecureSkipVerify: p.insecureSkipVerify})
			if err := tlsConn.Handshake(); err != nil {
				closed = true
				c.Close()
				rMutex.Lock()
				rerr = fmt.Errorf("TLS 协议握手错误：%v", err)
				rMutex.Unlock()
				ch <- 0
				return
			}
			if p.insecureSkipVerify == false && tlsConn.VerifyHostname(p.proxyDomain) != nil {
				closed = true
				tlsConn.Close()
				rMutex.Lock()
				rerr = fmt.Errorf("TLS 协议域名验证失败：%v", err)
				rMutex.Unlock()
				ch <- 0
				return
			}

			c = tlsConn
		}

		req, err := http.NewRequest("CONNECT", "", nil)
		if err != nil {
			closed = true
			c.Close()
			rMutex.Lock()
			rerr = fmt.Errorf("创建请求错误：%v", err)
			rMutex.Unlock()
			ch <- 0
			return
		}
		//req.URL.Path = raddr
		req.Host = raddr
		//req.URL.RawPath = raddr

		if p.standardHeader {
			xpath := "/"
			rInt, err := rand.Int(rand.Reader, big.NewInt(20))
			var rInt64 int64
			if err != nil {
				rInt64 = srand.Int63n(20)
			} else {
				rInt64 = rInt.Int64()
			}

			for i := int64(-10); i < rInt64; i++ {
				xpath += "X"
			}

			req.Header.Add("Ac0ept", "text/html, application/xhtml+xml, image/jxr, */*")
			req.Header.Add("Acc000-Encoding", "gzip, deflate")
			req.Header.Add("Ac000t-Language", "zh-CN")
			req.Header.Add("XXnnection", "000p-0000")
			req.Header.Add("Us0000gent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/000.00 (KHTML, like Gecko) Chrome/00.0.0000.0 Safari/000.00 Edge/00.00000")
			req.Header.Add("Cookie+path", xpath)

		}

		if p.auth != "" {
			auth := base64.StdEncoding.EncodeToString([]byte(p.auth))
			auth = fmt.Sprintf("Basic %v", auth)
			req.Header.Add("Proxy-Authorization", auth)
		}

		if err := req.Write(c); err != nil {
			closed = true
			c.Close()
			rMutex.Lock()
			rerr = fmt.Errorf("写请求错误：%v", err)
			rMutex.Unlock()
			ch <- 0
			return
		}

		br := bufio.NewReader(c)

		res, err := http.ReadResponse(br, req)
		if err != nil {
			closed = true
			c.Close()
			rMutex.Lock()
			rerr = fmt.Errorf("响应格式错误：%v", err)
			rMutex.Unlock()
			ch <- 0
			return
		}

		if res.StatusCode != 200 {
			closed = true
			c.Close()
			rMutex.Lock()
			rerr = fmt.Errorf("响应错误：%v", res)
			rMutex.Unlock()
			ch <- 0
			return
		}

		rMutex.Lock()
		rconn = &httpTCPConn{c, rawConn, tlsConn, net.TCPAddr{}, net.TCPAddr{}, "", "", 0, 0, p, res.Body}
		rMutex.Unlock()
		ch <- 1
		return
	}

	if timeout == 0 {
		go run()

		select {
		case <-ch:
			rMutex.Lock()
			defer rMutex.Unlock()
			return rconn, rerr
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
			rMutex.Lock()
			defer rMutex.Unlock()
			if rerr == nil {
				c.SetDeadline(time.Time{})
			}
			return rconn, rerr
		}
	}
}

// 重写了 Read 接口
// 由于 http 协议问题，解析响应需要读缓冲，所以必须重写 Read 来兼容读缓冲功能。
func (c *httpTCPConn) Read(b []byte) (n int, err error) {
	return c.r.Read(b)
}

// 重写了 Read 接口
// 由于 http 协议问题，解析响应需要读缓冲，所以必须重写 Read 来兼容读缓冲功能。
func (c *httpTCPConn) Close() error {
	c.r.Close()
	return c.Conn.Close()
}

func (c *httpTCPConn) SetLinger(sec int) error {
	return c.rawConn.SetLinger(sec)
}

func (c *httpTCPConn) SetNoDelay(noDelay bool) error {
	return c.rawConn.SetNoDelay(noDelay)
}
func (c *httpTCPConn) SetReadBuffer(bytes int) error {
	return c.rawConn.SetReadBuffer(bytes)
}
func (c *httpTCPConn) SetWriteBuffer(bytes int) error {
	return c.rawConn.SetWriteBuffer(bytes)
}

func (p *httpProxyClient) DialUDP(network string, laddr, raddr *net.UDPAddr) (net.Conn, error) {
	return nil, fmt.Errorf("%v 代理不支持 UDP 转发。", p.proxyType)
}

func (p *httpProxyClient) UpProxy() ProxyClient {
	return p.upProxy
}

func (p *httpProxyClient) SetUpProxy(upProxy ProxyClient) error {
	p.upProxy = upProxy
	return nil
}

func (c *httpTCPConn) ProxyClient() ProxyClient {
	return c.proxyClient
}

func (p *httpProxyClient) GetProxyAddrQuery() map[string][]string {
	return p.query
}
