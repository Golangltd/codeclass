# SOCKS
[![Build Status](https://travis-ci.org/eahydra/socks.svg?branch=master)](https://travis-ci.org/eahydra/socks)  [![GoDoc](https://godoc.org/github.com/eahydra/socks?status.svg)](https://godoc.org/github.com/eahydra/socks)

SOCKS implements SOCKS4/5 Proxy Protocol and HTTP Tunnel which can help you get through firewall.
The [cmd/socksd](https://github.com/eahydra/socks/blob/master/cmd/socksd) build with package SOCKS, supports cipher connection which crypto method is rc4, des, aes-128-cfb, aes-192-cfb and aes-256-cfb, upstream which can be shadowsocks or socsk5.

# Install
Assume you have go installed, you can install from source.
```
go get github.com/eahydra/socks
```

If you want to install [cmd/socksd](https://github.com/eahydra/socks/blob/master/cmd/socksd), please read the [README.md](https://github.com/eahydra/socks/blob/master/cmd/socksd/README.md)