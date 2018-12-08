package main

import (
	"bytes"
	"fmt"
	"sort"
	"text/template"
)

var pacTemplate = `
var proxy = "{{if .Socks5}}SOCKS5 {{.Socks5}};{{end}}{{if .Proxy}}PROXY {{.Proxy}}{{end}};";

var domains = {
  {{.Rules}}
};

var hasOwnProperty = Object.hasOwnProperty;

function FindProxyForURL(url, host) {
	if (isPlainHostName(host) || host === '127.0.0.1' || host === 'localhost') {
        return 'DIRECT';
    }
    var suffix;
    var pos = host.lastIndexOf('.');
    while(1) {
        suffix = host.substring(pos + 1);
        if (hasOwnProperty.call(domains, suffix)) {
            return proxy;
        }
        if (pos <= 0) {
            break;
        }
        pos = host.lastIndexOf('.', pos - 1);
    }
    return 'DIRECT';
}
`

type PACGenerator struct {
	filter map[string]bool
	rules  []string
	proxy  string
	socks5 string
}

func NewPACGenerator(proxy, socks5 string) *PACGenerator {
	return &PACGenerator{
		filter: make(map[string]bool),
		proxy:  proxy,
		socks5: socks5,
	}
}

func (p *PACGenerator) Generate(rules []string) ([]byte, error) {
	for _, v := range rules {
		if _, ok := p.filter[v]; !ok {
			p.filter[v] = true
			p.rules = append(p.rules, v)
		}
	}
	sort.Strings(p.rules)

	s := ""
	for n, v := range p.rules {
		s += fmt.Sprintf("'%s' : 1", v)
		if n+1 != len(p.rules) {
			s += ","
		}
	}

	data := struct {
		Proxy  string
		Socks5 string
		Rules  string
	}{
		Proxy:  p.proxy,
		Socks5: p.socks5,
		Rules:  s,
	}
	t, err := template.New("proxy.pac").Parse(pacTemplate)
	if err != nil {
		ErrLog.Println("failed to parse pacTempalte, err:", err)
	}
	buff := bytes.NewBuffer(nil)
	err = t.Execute(buff, &data)
	if err != nil {
		return nil, err
	}
	return buff.Bytes(), nil
}
