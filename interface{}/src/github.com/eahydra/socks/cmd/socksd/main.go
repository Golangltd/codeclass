package main

import (
	"bytes"
	"errors"
	"flag"
	"net"
	"net/http"
	"os"
	"os/signal"
	"strings"

	"github.com/eahydra/socks"
)

func main() {
	var configFile string
	flag.StringVar(&configFile, "c", "socks.config", "config file path")
	flag.Parse()

	conf, err := LoadConfig(configFile)
	if err != nil {
		ErrLog.Println("initGlobalConfig failed, err:", err)
		return
	}
	InfoLog.Println("load config succeeded")

	for _, c := range conf.Proxies {
		router := BuildUpstreamRouter(c)
		runHTTPProxyServer(c, router)
		runSOCKS4Server(c, router)
		runSOCKS5Server(c, router)
	}
	runPACServer(conf.PAC)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Kill, os.Interrupt)
	<-sigChan
}

func BuildUpstream(upstream Upstream, forward socks.Dialer) (socks.Dialer, error) {
	cipherDecorator := NewCipherConnDecorator(upstream.Crypto, upstream.Password)
	forward = NewDecorateClient(forward, cipherDecorator)

	switch strings.ToLower(upstream.Type) {
	case "socks5":
		{
			return socks.NewSocks5Client("tcp", upstream.Address, "", "", forward)
		}
	case "shadowsocks":
		{
			return socks.NewShadowSocksClient("tcp", upstream.Address, forward)
		}
	}
	return nil, errors.New("unknown upstream type" + upstream.Type)
}

func BuildUpstreamRouter(conf Proxy) socks.Dialer {
	var allForward []socks.Dialer
	for _, upstream := range conf.Upstreams {
		var forward socks.Dialer
		var err error
		forward = NewDecorateDirect(conf.DNSCacheTimeout)
		forward, err = BuildUpstream(upstream, forward)
		if err != nil {
			ErrLog.Println("failed to BuildUpstream, err:", err)
			continue
		}
		allForward = append(allForward, forward)
	}
	if len(allForward) == 0 {
		router := NewDecorateDirect(conf.DNSCacheTimeout)
		allForward = append(allForward, router)
	}
	return NewUpstreamDialer(allForward)
}

func runHTTPProxyServer(conf Proxy, router socks.Dialer) {
	if conf.HTTP != "" {
		listener, err := net.Listen("tcp", conf.HTTP)
		if err != nil {
			ErrLog.Println("net.Listen at ", conf.HTTP, " failed, err:", err)
			return
		}
		go func() {
			defer listener.Close()
			httpProxy := socks.NewHTTPProxy(router)
			http.Serve(listener, httpProxy)
		}()
	}
}

func runSOCKS4Server(conf Proxy, forward socks.Dialer) {
	if conf.SOCKS4 != "" {
		listener, err := net.Listen("tcp", conf.SOCKS4)
		if err != nil {
			ErrLog.Println("net.Listen failed, err:", err, conf.SOCKS4)
			return
		}
		cipherDecorator := NewCipherConnDecorator(conf.Crypto, conf.Password)
		listener = NewDecorateListener(listener, cipherDecorator)
		socks4Svr, err := socks.NewSocks4Server(forward)
		if err != nil {
			listener.Close()
			ErrLog.Println("socks.NewSocks4Server failed, err:", err)
		}
		go func() {
			defer listener.Close()
			socks4Svr.Serve(listener)
		}()
	}
}

func runSOCKS5Server(conf Proxy, forward socks.Dialer) {
	if conf.SOCKS5 != "" {
		listener, err := net.Listen("tcp", conf.SOCKS5)
		if err != nil {
			ErrLog.Println("net.Listen failed, err:", err, conf.SOCKS5)
			return
		}
		cipherDecorator := NewCipherConnDecorator(conf.Crypto, conf.Password)
		listener = NewDecorateListener(listener, cipherDecorator)
		socks5Svr, err := socks.NewSocks5Server(forward)
		if err != nil {
			listener.Close()
			ErrLog.Println("socks.NewSocks5Server failed, err:", err)
			return
		}
		go func() {
			defer listener.Close()
			socks5Svr.Serve(listener)
		}()
	}
}

func runPACServer(pac PAC) {
	pu, err := NewPACUpdater(pac)
	if err != nil {
		ErrLog.Println("failed to NewPACUpdater, err:", err)
		return
	}

	http.HandleFunc("/proxy.pac", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Add("Content-Type", "application/x-ns-proxy-autoconfig")
		data, time := pu.get()
		reader := bytes.NewReader(data)
		http.ServeContent(w, r, "proxy.pac", time, reader)
	})
	go http.ListenAndServe(pac.Address, nil)
}
