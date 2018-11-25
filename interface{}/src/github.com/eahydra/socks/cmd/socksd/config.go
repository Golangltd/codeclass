package main

import (
	"encoding/json"
	"io/ioutil"
)

type Upstream struct {
	Type     string `json:"type"`
	Crypto   string `json:"crypto"`
	Password string `json:"password"`
	Address  string `json:"address"`
}

type PAC struct {
	Address     string   `json:"address"`
	Proxy       string   `json:"proxy"`
	SOCKS5      string   `json:"socks5"`
	LocalRules  string   `json:"local_rule_file"`
	RemoteRules string   `json:"remote_rule_file"`
	Upstream    Upstream `json:"upstream"`
}

type Proxy struct {
	HTTP            string     `json:"http"`
	SOCKS4          string     `json:"socks4"`
	SOCKS5          string     `json:"socks5"`
	Crypto          string     `json:"crypto"`
	Password        string     `json:"password"`
	DNSCacheTimeout int        `json:"dnsCacheTimeout"`
	Upstreams       []Upstream `json:"upstreams"`
}

type Config struct {
	PAC     PAC     `json:"pac"`
	Proxies []Proxy `json:"proxies"`
}

func LoadConfig(s string) (*Config, error) {
	data, err := ioutil.ReadFile(s)
	if err != nil {
		return nil, err
	}
	cfgGroup := &Config{}
	if err = json.Unmarshal(data, cfgGroup); err != nil {
		return nil, err
	}
	return cfgGroup, nil
}
