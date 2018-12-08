package socks

import (
	"fmt"
	"io"
	"net"
	"net/http"
	"net/http/httputil"
	"net/url"
)

// HTTPProxy is an HTTP Handler that serve CONNECT method and
// route request to proxy server by Router.
type HTTPProxy struct {
	*httputil.ReverseProxy
	forward Dialer
}

// NewHTTPProxy constructs one HTTPProxy
func NewHTTPProxy(forward Dialer) *HTTPProxy {
	return &HTTPProxy{
		ReverseProxy: &httputil.ReverseProxy{
			Director: director,
			Transport: &http.Transport{
				Dial: func(network, addr string) (net.Conn, error) {
					return forward.Dial(network, addr)
				},
			},
		},
		forward: forward,
	}
}

func director(request *http.Request) {
	u, err := url.Parse(request.RequestURI)
	if err != nil {
		return
	}
	request.RequestURI = u.RequestURI()
	v := request.Header.Get("Proxy-Connection")
	if v != "" {
		request.Header.Del("Proxy-Connection")
		request.Header.Del("Connection")
		request.Header.Add("Connection", v)
	}
}

// ServeHTTPTunnel serve incoming request with CONNECT method, then route data to proxy server
func (h *HTTPProxy) ServeHTTPTunnel(response http.ResponseWriter, request *http.Request) {
	var conn net.Conn
	if hj, ok := response.(http.Hijacker); ok {
		var err error
		if conn, _, err = hj.Hijack(); err != nil {
			http.Error(response, err.Error(), http.StatusInternalServerError)
			return
		}
	} else {
		http.Error(response, "Hijacker failed", http.StatusInternalServerError)
		return
	}
	defer conn.Close()

	dest, err := h.forward.Dial("tcp", request.Host)
	if err != nil {
		fmt.Fprintf(conn, "HTTP/1.0 500 NewRemoteSocks failed, err:%s\r\n\r\n", err)
		return
	}
	defer dest.Close()

	if request.Body != nil {
		if _, err = io.Copy(dest, request.Body); err != nil {
			fmt.Fprintf(conn, "%d %s", http.StatusBadGateway, err.Error())
			return
		}
	}
	fmt.Fprintf(conn, "HTTP/1.0 200 Connection established\r\n\r\n")

	go func() {
		defer conn.Close()
		defer dest.Close()
		io.Copy(dest, conn)
	}()
	io.Copy(conn, dest)
}

// ServeHTTP implements HTTP Handler
func (h *HTTPProxy) ServeHTTP(response http.ResponseWriter, request *http.Request) {
	if request.Method == "CONNECT" {
		h.ServeHTTPTunnel(response, request)
	} else {
		h.ReverseProxy.ServeHTTP(response, request)
	}
}
