package proxyclient

import (
	"errors"
	"fmt"
	"net"
	"net/url"
	"strconv"
	"strings"
	"time"
)

// Conn 用来表示连接
type Conn interface {
	net.Conn
}

// TCPConn 用来表示 TCP 连接
// 提供 net.TcpConn 结构的全部方法
// 但是部分方法由于代理协议的限制可能不能获得正确的结果。例如：LocalAddr 、RemoteAddr 方法不被很多代理协议支持。
type TCPConn interface {
	Conn

	/*
		SetLinger设定当连接中仍有数据等待发送或接受时的Close方法的行为。
		如果sec < 0（默认），Close方法立即返回，操作系统停止后台数据发送；如果 sec == 0，Close立刻返回，操作系统丢弃任何未发送或未接收的数据；如果sec > 0，Close方法阻塞最多sec秒，等待数据发送或者接收，在一些操作系统中，在超时后，任何未发送的数据会被丢弃。
	*/
	SetLinger(sec int) error

	// SetNoDelay设定操作系统是否应该延迟数据包传递，以便发送更少的数据包（Nagle's算法）。默认为真，即数据应该在Write方法后立刻发送。
	SetNoDelay(noDelay bool) error

	//SetReadBuffer设置该连接的系统接收缓冲
	SetReadBuffer(bytes int) error

	//SetWriteBuffer设置该连接的系统发送缓冲
	SetWriteBuffer(bytes int) error
}

// ProxyTCPConn 用来表示通过代理访问的TCP连接
type ProxyTCPConn interface {
	TCPConn
	ProxyClient() ProxyClient // 获得所属的代理
}

// UDPConn 表示 UDP 连接
type UDPConn interface {
	Conn
}

// ProxyUDPConn 用来表示通过代理访问的TCP连接
type ProxyUDPConn interface {
	UDPConn
	ProxyClient() ProxyClient // 获得所属的代理
}

// ProxyClient 仿 net 库接口的代理客户端
// 支持级联代理功能，可以通过 SetUpProxy 设置上级代理。
type ProxyClient interface {
	// 返回本代理的上层级联代理
	UpProxy() ProxyClient
	// 设置本代理的上层代理
	SetUpProxy(upProxy ProxyClient) error

	// Dial 在网络network上连接地址address，并返回一个Conn接口。可用的网络类型有：
	// "tcp"、"tcp4"、"tcp6"、"udp"、"udp4"、"udp6"
	// 对TCP和UDP网络，地址格式是host:port或[host]:port，参见函数JoinHostPort和SplitHostPort。
	// 如果代理服务器支持远端DNS解析，那么会使用远端DNS解析。
	Dial(network, address string) (net.Conn, error)

	DialTimeout(network, address string, timeout time.Duration) (net.Conn, error)

	// DialTCP在网络协议net上连接本地地址laddr和远端地址raddr。net必须是"tcp"、"tcp4"、"tcp6"；如果laddr不是nil，将使用它作为本地地址，否则自动选择一个本地地址。
	// 由于 net.TCPAddr 内部保存的是IP地址及端口，所以使用本函数无法使用远端DNS解析，要想使用远端DNS解析，请使用 Dial 或 DialTCPSAddr 函数。
	DialTCP(net string, laddr, raddr *net.TCPAddr) (net.Conn, error)

	// DialTCPSAddr 同 DialTCP 函数，主要区别是如果代理支持远端dns解析，那么会使用远端dns解析。
	DialTCPSAddr(network string, raddr string) (ProxyTCPConn, error)

	// DialTCPSAddrTimeout 同 DialTCPSAddr 函数，增加了超时功能
	DialTCPSAddrTimeout(network string, raddr string, timeour time.Duration) (ProxyTCPConn, error)

	//ListenTCP在本地TCP地址laddr上声明并返回一个*TCPListener，net参数必须是"tcp"、"tcp4"、"tcp6"，如果laddr的端口字段为0，函数将选择一个当前可用的端口，可以用Listener的Addr方法获得该端口。
	//ListenTCP(net string, laddr *TCPAddr) (*TCPListener, error)
	//DialTCP在网络协议net上连接本地地址laddr和远端地址raddr。net必须是"udp"、"udp4"、"udp6"；如果laddr不是nil，将使用它作为本地地址，否则自动选择一个本地地址。
	DialUDP(net string, laddr, raddr *net.UDPAddr) (net.Conn, error)

	// 获得 Proxy 代理地址的 Query
	// 为了大小写兼容，key全部是转换成小写的。
	GetProxyAddrQuery() map[string][]string
}

// NewProxyClient 用来创建代理客户端
//
// 参数格式：允许使用 ?参数名1=参数值1&参数名2=参数值2指定参数
// 例如：https://123.123.123.123:8088?insecureskipverify=true
//     全体协议可选参数： upProxy=http://145.2.1.3:8080 用于指定代理的上层代理，即代理嵌套。默认值：direct://0.0.0.0:0000
//
// http 代理 http://123.123.123.123:8088
//     可选功能： 用户认证功能。格式：http://user:password@123.123.123:8080
//     可选参数：standardheader=false true表示 CONNNET 请求包含标准的 Accept、Accept-Encoding、Accept-Language、User-Agent等头。默认值：false
//
// https 代理 https://123.123.123.123:8088
//     可选功能： 用户认证功能，同 http 代理。
//     可选参数：standardheader=false 同上 http 代理
//     可选参数：insecureskipverify=false true表示跳过 https 证书验证。默认false。
//     可选参数：domain=域名 指定https验证证书时使用的域名，默认为 host:port
//
// socks4 代理 socks4://123.123.123.123:5050
//     注意：socks4 协议不支持远端 dns 解析
//
// socks4a 代理 socks4a://123.123.123.123:5050
//
// socks5 代理 socks5://123.123.123.123:5050
//     可选功能：用户认证功能。支持无认证、用户名密码认证，格式同 http 代理。
//
// ss 代理 ss://method:passowd@123.123.123:5050
//
// 直连 direct://0.0.0.0:0000
//     可选参数： LocalAddr=0.0.0.0:0 表示tcp连接绑定的本地ip及端口，默认值 0.0.0.0:0。
//     可选参数： SplitHttp=false true 表示拆分 http 请求(分多个tcp包发送)，可以解决简单的运营商 http 劫持。默认值：false 。
//              原理是：当发现目标地址为 80 端口，发送的内容包含 GET、POST、HTTP、HOST 等关键字时，会将关键字拆分到两个包在发送出去。
//              注意： Web 防火墙类软件、设备可能会重组 HTTP 包，造成拆分无效。目前已知 ESET Smart Security 会造成这个功能无效，即使暂停防火墙也一样无效。
//              G|ET /pa|th H|TTTP/1.0
//              HO|ST:www.aa|dd.com
//     可选参数： sleep=0  建立连接后延迟多少毫秒发送数据，配合 ttl 反劫持系统时建议设置为10置50。默认值 0 .

func NewProxyClient(addr string) (ProxyClient, error) {
	u, err := url.Parse(addr)
	if err != nil {
		return nil, errors.New("addr 错误的格式")
	}

	// 将 query key 转换成为小写
	_query := u.Query()
	query := make(map[string][]string, len(_query))
	for k, v := range _query {
		query[strings.ToLower(k)] = v
	}

	queryGet := func(key string) string {
		if query == nil {
			return ""
		}
		v, ok := query[key]
		if !ok || len(v) == 0 {
			return ""
		}
		return v[0]
	}
	scheme := strings.ToLower(strings.TrimSpace(u.Scheme))

	var upProxy ProxyClient
	if up, ok := query["upproxy"]; ok == true {
		if upProxy, err = NewProxyClient(up[0]); err != nil {
			return nil, fmt.Errorf("upProxy 创建失败：%v", err)
		}
	}

	switch scheme {
	case "direct":
		localAddr := queryGet("localaddr")
		if localAddr == "" {
			localAddr = ":0"
		}

		splitHttp := false
		if strings.ToLower(queryGet("splithttp")) == "true" {
			splitHttp = true
		}

		sleep := 0 * time.Millisecond
		if queryGet("sleep") != "" {
			if s, err := strconv.Atoi(queryGet("sleep")); err != nil {
				return nil, fmt.Errorf("sleep 参数错误：%v", err)
			} else {
				sleep = time.Duration(s) * time.Millisecond
			}
		}

		return newDriectProxyClient(localAddr, splitHttp, sleep, query)

	case "socks4", "socks4a", "socks5":
		username := ""
		password := ""
		if u.User != nil {
			username = u.User.Username()
			password, _ = u.User.Password()
		}

		return newSocksProxyClient(scheme, u.Host, username, password, upProxy, query)
	case "http", "https":
		auth := ""
		if u.User != nil {
			auth = u.User.String()
		}

		standardHeader := false
		if strings.ToLower(queryGet("standardheader")) == "true" {
			standardHeader = true
		}

		insecureSkipVerify := false
		if strings.ToLower(queryGet("insecureskipverify")) == "true" {
			insecureSkipVerify = true
		}

		domain := queryGet("domain")

		return newHTTPProxyClient(scheme, u.Host, domain, auth, insecureSkipVerify, standardHeader, upProxy, query)
	case "ss":
		password, ok := u.User.Password()
		if ok == false {
			return nil, fmt.Errorf("ss 代理 method, password 格式错误。")
		}
		return newSsProxyClient(u.Host, u.User.Username(), password, upProxy, query)
	default:
		return nil, fmt.Errorf("未识别的代理类型：%v", scheme)
	}
}
