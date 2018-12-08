package main

import (
	"net"
	"sync"
	"time"
)

type DNSCache struct {
	lock         sync.RWMutex
	cacheTimeout time.Duration
	dns          map[string]DNSElement
}

type DNSElement struct {
	ip        net.IP
	startTime time.Time
}

func NewDNSCache(cacheTimeout int) *DNSCache {
	if cacheTimeout <= 0 {
		cacheTimeout = 30
	}
	return &DNSCache{
		cacheTimeout: time.Duration(cacheTimeout) * time.Minute,
		dns:          make(map[string]DNSElement),
	}
}

func (c *DNSCache) Get(domain string) (net.IP, bool) {
	c.lock.RLock()
	e, ok := c.dns[domain]
	c.lock.RUnlock()
	if ok && time.Since(e.startTime) > c.cacheTimeout {
		c.lock.Lock()
		delete(c.dns, domain)
		c.lock.Unlock()
		return nil, false
	}
	return e.ip, ok
}

func (c *DNSCache) Set(domain string, ip net.IP) {
	c.lock.Lock()
	c.dns[domain] = DNSElement{ip: ip, startTime: time.Now()}
	c.lock.Unlock()
}
