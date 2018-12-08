# SOCKS
[![Build Status](https://travis-ci.org/eahydra/socks.svg?branch=master)](https://travis-ci.org/eahydra/socks)

The cmd/socksd build with package SOCKS, supports cipher connection which crypto method is rc4, des, aes-128-cfb, aes-192-cfb and aes-256-cfb, upstream which can be shadowsocks or socsk5.

# Install
Assume you have go installed, you can install from source.
```
go get github.com/eahydra/socks/cmd/socksd
```

# Usage
Configuration file is in json format. The file must name **socks.config** and put it with socksd together.
Configuration parameters as follows:
```json
{
    "pac":{
        "address":"127.0.0.1:50000",
        "proxy":"127.0.0.1:40000",
        "socks5":"127.0.0.1:8000",
        "local_rule_file":"gfw.txt",
        "remote_rule_file":"https://raw.githubusercontent.com/Leask/BRICKS/master/gfw.bricks",
        "upstream":{
            "type":"shadowsocks",
            "crypto": "aes-256-cfb",
            "password": "111222333",
            "address": "106.182.12.24:10089"
        }
    },
    "proxies":[
        {
            "http":":40000",
	        "socks4": ":9000",
	        "socks5": ":8000",
	        "upstreams": [
		        {
                    "type":"shadowsocks",
			        "crypto": "aes-256-cfb",
			        "password": "111222333",
			        "address": "106.182.12.24:10089"
		        }
            ]
        }
    ]
}

```

*  **pac**	- PAC config
	* **address** - Specifies the PAC server (127.0.0.1:50000)
	* **proxy**	  - (OPTIONAL) Enable HTTP Proxy in PAC
	* **socks5**  - (OPTIONAL) Enable SOCKS5 Proxy in PAC
	* **local_rule_file** - (OPTIONAL) Local PAC rule file that per line is domain
	* **remote_rule_file** - (OPTIONAL) Remote PAC rule file like as [bricks](https://raw.githubusercontent.com/Leask/BRICKS/master/gfw.bricks)
	* **upstream**         - (OPTIONAL) Through upstream to get **remote_rule_file**
*  **proxies**             	- The array of proxy config item
	*  **http**       			- (OPTIONAL) Enable http proxy tunnel (127.0.0.1:8080 or :8080)
	*  **socks4**          	- (OPTIONAL) Enable SOCKS4 proxy (127.0.0.1:9090 or :9090)
	*  **socks5**          	- (OPTIONAL) Enable SOCKS5 proxy (127.0.0.1:9999 or :9999)
	*  **crypto**   		- (OPTIONAL) SOCKS5's crypto method, now supports rc4, des, aes-128-cfb, aes-192-cfb and aes-256-cfb
	*  **password**      	- If you set **crypto**, you must also set passsword
	*  **dnsCacheTimeout**     	- (OPTIONAL) Enable dns cache (unit is second)
	* **upstreams**				- The array of **upstream**
* **upstream**
    *  **type**         	- Specifies the type of upstream proxy server. Now supports shadowsocks and socks5
    *  **crypto**        	- Specifies the crypto method of upstream proxy server. The crypto method is same as **localCryptoMethod**
    *  **password**            	- Specifies the crypto password of upstream proxy server
    *  **address**                	- Specifies the address of upstream proxy server (8.8.8.8:1111)
