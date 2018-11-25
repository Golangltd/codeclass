package main

import (
	"net"

	"github.com/eahydra/socks"
)

type DecorateDirect struct {
	dnsCache *DNSCache
}

func NewDecorateDirect(dnsCacheTime int) *DecorateDirect {
	var dnsCache *DNSCache
	if dnsCacheTime != 0 {
		dnsCache = NewDNSCache(dnsCacheTime)
	}
	return &DecorateDirect{
		dnsCache: dnsCache,
	}
}

func parseAddress(address string) (interface{}, string, error) {
	host, port, err := net.SplitHostPort(address)
	if err != nil {
		return nil, "", err
	}
	ip := net.ParseIP(address)
	if ip != nil {
		return ip, port, nil
	} else {
		return host, port, nil
	}
}

func (d *DecorateDirect) Dial(network, address string) (net.Conn, error) {
	host, port, err := parseAddress(address)
	if err != nil {
		return nil, err
	}
	var dest string
	var ipCached bool
	switch h := host.(type) {
	case net.IP:
		{
			dest = h.String()
			ipCached = true
		}
	case string:
		{
			dest = h
			if d.dnsCache != nil {
				if p, ok := d.dnsCache.Get(h); ok {
					dest = p.String()
					ipCached = true
				}
			}
		}
	}
	address = net.JoinHostPort(dest, port)
	destConn, err := socks.Direct.Dial(network, address)
	if err != nil {
		return nil, err
	}

	if d.dnsCache != nil && !ipCached {
		d.dnsCache.Set(host.(string), destConn.RemoteAddr().(*net.TCPAddr).IP)
	}
	return destConn, nil
}
