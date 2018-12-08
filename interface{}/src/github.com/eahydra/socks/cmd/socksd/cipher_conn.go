package main

import (
	"io"
	"net"
	"strings"
)

type CipherConn struct {
	net.Conn
	rwc io.ReadWriteCloser
}

func (c *CipherConn) Read(data []byte) (int, error) {
	return c.rwc.Read(data)
}

func (c *CipherConn) Write(data []byte) (int, error) {
	return c.rwc.Write(data)
}

func (c *CipherConn) Close() error {
	err := c.Conn.Close()
	c.rwc.Close()
	return err
}

func NewCipherConn(conn net.Conn, cryptMethod string, password []byte) (*CipherConn, error) {
	var rwc io.ReadWriteCloser
	var err error
	switch strings.ToLower(cryptMethod) {
	default:
		rwc = conn
	case "rc4":
		rwc, err = NewRC4Cipher(conn, password)
	case "des":
		rwc, err = NewDESCFBCipher(conn, password)
	case "aes-128-cfb":
		rwc, err = NewAESCFGCipher(conn, password, 16)
	case "aes-192-cfb":
		rwc, err = NewAESCFGCipher(conn, password, 24)
	case "aes-256-cfb":
		rwc, err = NewAESCFGCipher(conn, password, 32)
	case "chacha20":
		rwc, err = NewChacha20Cipher(conn, password)
	}
	if err != nil {
		return nil, err
	}

	return &CipherConn{
		Conn: conn,
		rwc:  rwc,
	}, nil
}

func NewCipherConnDecorator(cryptoMethod, password string) ConnDecorator {
	return func(conn net.Conn) (net.Conn, error) {
		return NewCipherConn(conn, cryptoMethod, []byte(password))
	}
}
