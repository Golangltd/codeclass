package main

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/des"
	"crypto/md5"
	"crypto/rand"
	"crypto/rc4"
	"io"

	"github.com/codahale/chacha20"
)

func md5sum(d []byte) []byte {
	h := md5.New()
	h.Write(d)
	return h.Sum(nil)
}

func evpBytesToKey(password []byte, keylen int) (key []byte) {
	const md5len = 16
	cnt := (keylen-1)/md5len + 1
	m := make([]byte, cnt*md5len)
	copy(m, md5sum(password))

	d := make([]byte, md5len+len(password))
	start := 0
	for i := 1; i < cnt; i++ {
		start += md5len
		copy(d, m[start-md5len:start])
		copy(d[md5len:], password)
		copy(m[start:], md5sum(d))
	}
	return m[:keylen]
}

type RC4Cipher struct {
	*cipher.StreamReader
	*cipher.StreamWriter
}

func NewRC4Cipher(rwc io.ReadWriteCloser, password []byte) (*RC4Cipher, error) {
	decryptCipher, err := rc4.NewCipher(password)
	if err != nil {
		return nil, err
	}
	encryptCipher, err := rc4.NewCipher(password)
	if err != nil {
		return nil, err
	}
	return &RC4Cipher{
		StreamReader: &cipher.StreamReader{
			S: decryptCipher,
			R: rwc,
		},
		StreamWriter: &cipher.StreamWriter{
			S: encryptCipher,
			W: rwc,
		},
	}, nil
}

type Chacha20Cipher struct {
	password []byte
	rwc      io.ReadWriteCloser
	*cipher.StreamReader
	*cipher.StreamWriter
}

func NewChacha20Cipher(rwc io.ReadWriteCloser, password []byte) (*Chacha20Cipher, error) {
	password = evpBytesToKey(password, chacha20.KeySize)
	return &Chacha20Cipher{
		rwc:      rwc,
		password: password,
	}, nil
}

func (c *Chacha20Cipher) Read(p []byte) (n int, err error) {
	if c.StreamReader == nil {
		iv := make([]byte, chacha20.NonceSize)
		n, err = io.ReadFull(c.rwc, iv)
		if err != nil {
			return n, err
		}
		stream, err := chacha20.New(c.password, iv)
		if err != nil {
			return n, err
		}

		c.StreamReader = &cipher.StreamReader{
			S: stream,
			R: c.rwc,
		}
	}
	return c.StreamReader.Read(p)
}

func (c *Chacha20Cipher) Write(p []byte) (n int, err error) {
	if c.StreamWriter == nil {
		iv := make([]byte, chacha20.NonceSize)
		_, err = rand.Read(iv)
		if err != nil {
			return 0, err
		}
		stream, err := chacha20.New(c.password, iv)
		if err != nil {
			return n, err
		}
		c.StreamWriter = &cipher.StreamWriter{
			S: stream,
			W: c.rwc,
		}
		n, err := c.rwc.Write(iv)
		if err != nil {
			return n, err
		}
	}
	return c.StreamWriter.Write(p)
}

type DESCFBCipher struct {
	block cipher.Block
	rwc   io.ReadWriteCloser
	*cipher.StreamReader
	*cipher.StreamWriter
}

func NewDESCFBCipher(rwc io.ReadWriteCloser, password []byte) (*DESCFBCipher, error) {
	block, err := des.NewCipher(password)
	if err != nil {
		return nil, err
	}

	return &DESCFBCipher{
		block: block,
		rwc:   rwc,
	}, nil
}

func (d *DESCFBCipher) Read(p []byte) (n int, err error) {
	if d.StreamReader == nil {
		iv := make([]byte, d.block.BlockSize())
		n, err = io.ReadFull(d.rwc, iv)
		if err != nil {
			return n, err
		}
		stream := cipher.NewCFBDecrypter(d.block, iv)
		d.StreamReader = &cipher.StreamReader{
			S: stream,
			R: d.rwc,
		}
	}
	return d.StreamReader.Read(p)
}

func (d *DESCFBCipher) Write(p []byte) (n int, err error) {
	if d.StreamWriter == nil {
		iv := make([]byte, d.block.BlockSize())
		_, err = rand.Read(iv)
		if err != nil {
			return 0, err
		}
		stream := cipher.NewCFBEncrypter(d.block, iv)
		d.StreamWriter = &cipher.StreamWriter{
			S: stream,
			W: d.rwc,
		}
		n, err := d.rwc.Write(iv)
		if err != nil {
			return n, err
		}
	}
	return d.StreamWriter.Write(p)
}

func (d *DESCFBCipher) Close() error {
	if d.StreamWriter != nil {
		d.StreamWriter.Close()
	}
	if d.rwc != nil {
		d.rwc.Close()
	}
	return nil
}

type AESCFBCipher struct {
	rwc   io.ReadWriteCloser
	iv    []byte
	block cipher.Block
	*cipher.StreamReader
	*cipher.StreamWriter
}

func NewAESCFGCipher(rwc io.ReadWriteCloser, password []byte, bit int) (*AESCFBCipher, error) {
	block, err := aes.NewCipher(evpBytesToKey(password, bit))
	if err != nil {
		return nil, err
	}
	return &AESCFBCipher{
		block: block,
		rwc:   rwc,
	}, nil
}

func (a *AESCFBCipher) Read(p []byte) (n int, err error) {
	if a.StreamReader == nil {
		iv := make([]byte, a.block.BlockSize())
		n, err = io.ReadFull(a.rwc, iv)
		if err != nil {
			return n, err
		}
		stream := cipher.NewCFBDecrypter(a.block, iv)
		a.StreamReader = &cipher.StreamReader{
			S: stream,
			R: a.rwc,
		}
	}
	return a.StreamReader.Read(p)
}

func (a *AESCFBCipher) Write(p []byte) (n int, err error) {
	if a.StreamWriter == nil {
		iv := make([]byte, a.block.BlockSize())
		_, err = rand.Read(iv)
		if err != nil {
			return 0, err
		}
		stream := cipher.NewCFBEncrypter(a.block, iv)
		a.StreamWriter = &cipher.StreamWriter{
			S: stream,
			W: a.rwc,
		}
		n, err := a.rwc.Write(iv)
		if err != nil {
			return n, err
		}
	}
	return a.StreamWriter.Write(p)
}

func (a *AESCFBCipher) Close() error {
	if a.StreamWriter != nil {
		a.StreamWriter.Close()
	}
	if a.rwc != nil {
		a.rwc.Close()
	}
	return nil
}
