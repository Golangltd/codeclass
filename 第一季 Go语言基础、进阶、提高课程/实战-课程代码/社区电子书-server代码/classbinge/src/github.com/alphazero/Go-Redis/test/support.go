package test

import (
	"redis"
	"testing"
	"testing/quick"
)

func compareByteArrays(got, expected []byte) bool {
	if len(got) != len(expected) {
		return false
	}
	for i, b := range expected {
		if got[i] != b {
			return false
		}
	}
	return true
}

// ----------------------------------------------------------------------
// testing/quick/Config settings for client types
// Modify quickConf<Type> for specific test case, as necessary
// ----------------------------------------------------------------------

// redis.AsyncClient
func QuickConfAsyncClient(method string) *quick.Config {

	conf := quickConfAll(method)

	/* modify as necessary */

	return conf
}

// redis.Client
func QuickConfClient(method string) *quick.Config {

	conf := quickConfAll(method)

	/* modify as necessary */

	return conf
}

func quickConfAll(method string) *quick.Config {
	conf := &quick.Config{}
	conf.MaxCount = 100

	switch method {
	// Commands that only need to be tested once
	case "Quit",
		"Ping",
		"flushdb",
		"flushall",
		"Bgsave",
		"AllKeys":
		conf.MaxCount = 1
	}

	return conf
}

// ----------------------------------------------------------------------
// Client factories
// ----------------------------------------------------------------------

func NewAsyncClient(t *testing.T) redis.AsyncClient {
	return NewAsyncClientWithSpec(t, getTestConnSpec())
}

func NewAsyncClientWithSpec(t *testing.T, spec *redis.ConnectionSpec) redis.AsyncClient {
	client, err := redis.NewAsynchClientWithSpec(getTestConnSpec())
	if err != nil {
		t.Fatalf("NewAsynchClientWithSpec - ", err)
	}
	return client
}

func NewClient(t *testing.T) redis.Client {
	client, err := redis.NewSynchClientWithSpec(getTestConnSpec())
	if err != nil {
		t.Fatalf("TestGet - ", err)
	}
	return client
}

// Test ConnectionSpec uses redis db 13 and assumes AUTH password go-redis
func getTestConnSpec() *redis.ConnectionSpec {
	spec := redis.DefaultSpec()
	spec.Password("go-redis").Db(13)
	return spec
}

// ----------------------------------------------------------------------
// Test helper methods -
// ----------------------------------------------------------------------

func FlushAndQuitAsyncClient(t *testing.T, client redis.AsyncClient) {
	FlushAsyncClient(t, client)
	QuitAsyncClient(t, client)
}
func FlushAndQuitClient(t *testing.T, client redis.Client) {
	FlushClient(t, client)
	QuitClient(t, client)
}

func FlushAsyncClient(t *testing.T, client redis.AsyncClient) {
	// flush it
	fstat, e := client.Flushdb()
	if e != nil {
		t.Fatalf("on Flushdb - %s", e)
	}
	ok, fe := fstat.Get()
	if fe != nil {
		t.Fatalf("on fstat.Get() - %s", fe)
	}
	if !ok {
		t.Fatalf("fstat.Get() returned false")
	}
}
func FlushClient(t *testing.T, client redis.Client) {
	// flush it
	e := client.Flushdb()
	if e != nil {
		t.Fatalf("on Flushdb - %s", e)
	}
}

func QuitAsyncClient(t *testing.T, client redis.AsyncClient) {
	// flush it
	fstat, e := client.Quit()
	if e != nil {
		t.Fatalf("on Quit - %s", e)
	}
	ok, fe := fstat.Get()
	if fe != nil {
		t.Fatalf("on fstat.Get() - %s", fe)
	}
	if !ok {
		t.Fatalf("fstat.Get() returned false")
	}
}
func QuitClient(t *testing.T, client redis.Client) {
	// flush it
	e := client.Quit()
	if e != nil {
		t.Fatalf("on Quit - %s", e)
	}
}
