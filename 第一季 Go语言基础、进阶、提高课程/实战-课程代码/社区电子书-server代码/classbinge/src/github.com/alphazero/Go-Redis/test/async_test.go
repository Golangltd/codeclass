// REVU - only whitebox testing in redis package
//		- freeze adding any more test code for client here
// TODO - move to blackbox redis/test package

package test

import (
	"fmt"
	"log"
	"redis"
	"testing"
)

func asyncFlushAndQuitOnCompletion(t *testing.T, client redis.AsyncClient) {
	// flush it
	fStat, e := client.Flushdb()
	if e != nil {
		t.Errorf("on Flushdb - %s", e)
	}
	ok, fe := fStat.Get()
	if fe != nil {
		t.Fatalf("BUG - non-Error future result get must never return error - got: %s", fe)
	}
	if !ok {
		t.Fatalf("BUG - non-Error flushdb future result must always be true ")
	}

	fStat, e = client.Quit()
	if e != nil {
		t.Errorf("on Quit - %s", e)
	}
	ok, fe = fStat.Get()
	if fe != nil {
		t.Fatalf("BUG - non-Error future result get must never return error - got: %s", fe)
	}
	if !ok {
		t.Fatalf("BUG - non-Error quit future result must always be true ")
	}
}

// Check that connection is actually passing passwords from spec
// and catching AUTH ERRs.
func TestAsyncClientConnectWithBadSpec(t *testing.T) {
	spec := getTestConnSpec()
	spec.Password("bad-password")
	client, expected := redis.NewAsynchClientWithSpec(spec)
	if expected == nil {
		t.Error("BUG: Expected a RedisError")
	}
	if client != nil {
		t.Error("BUG: async client reference on error MUST be nil")
	}
}

// Check that connection is actually passing passwords from spec
func TestAsyncClientConnectWithSpec(t *testing.T) {
	spec := getTestConnSpec()

	client, err := redis.NewAsynchClientWithSpec(spec)
	if err != nil {
		t.Fatalf("failed to create client with spec. Error: %s ", err)
	} else if client == nil {
		t.Fatal("BUG: client is nil")
	}

	// quit once -- OK
	futureBool, err := client.Quit()
	if err != nil {
		t.Errorf("BUG - initial Quit on asyncClient should not return error - %s ", err)
	}
	if futureBool == nil {
		t.Errorf("BUG - non-error asyncClient response should not return nil future")
	}
	// block until we get results
	ok, fe := futureBool.Get()
	if fe != nil {
		t.Errorf("BUG - non-Error Quit future result get must never return error - got: %s", fe)
	}
	if !ok {
		t.Errorf("BUG - non-Error Quit future result must always be true ")
	}

	// subsequent quit should raise error
	for i := 0; i < 10; i++ {
		futureBool, err = client.Quit()
		if err == nil {
			t.Errorf("BUG - Quit on shutdown asyncClient should return error")
		}
		if futureBool != nil {
			t.Errorf("BUG - Quit on shutdown asyncClient should not return future. got: %s", futureBool)
		}
	}
}

func TestAsyncMget(t *testing.T) {
	client := NewAsyncClient(t)

	vprefix := "the-value"
	key := "mget-test"
	fSet, e := client.Set(key, []byte(vprefix))
	if e != nil {
		t.Fatalf("on Set(%s, %s) - %s", key, vprefix, e)
	}
	// REVU - this call to wait for Get res is strictly speaking not required ..
	if _, fe := fSet.Get(); fe != nil {
		t.Fatalf("on fset.Get - %s", fe)
	}
	var fMget redis.FutureBytesArray
	fMget, e = client.Mget(key, nil)
	if e != nil {
		t.Fatalf("on Set(%s, %s) - %s", key, vprefix, e)
	}
	if fMget == nil { // check not necessary for normal usage
		t.Fatal("nil future returned on non-error MGet")
	}
	//	var vset [][]byte
	vset, fe := fMget.Get()
	if fe != nil {
		t.Fatalf("on fset.Get - %s", fe)
	}
	if len(vset) != 1 {
		t.Fatalf("len(vset) expected: 1 got: %d", len(vset))
	}
	expected := []byte(vprefix)
	got := vset[0]
	if !compareByteArrays(expected, got) {
		t.Fatalf("Mget res [0] - expected: %s got: %s", expected, got)
	}

	keys := testdata[_testdata_keys].([]string)
	for i, k := range keys {
		if i > 10 {
			break
		}
		v := []byte(fmt.Sprintf("%s_%03d", vprefix, i))
		// ignoring future stat on Get
		if _, e := client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
		fMget, e := client.Mget(key, keys[0:i])
		if e != nil {
			t.Errorf("on Mget(%s) - %s", key, e)
		}
		vset, fe = fMget.Get()
		if fe != nil {
			t.Fatalf("on fset.Get - %s", fe)
		}
		if vset == nil {
			t.Errorf("on Mget(%s) - got nil", key)
		}
		if len(vset) != 1+i {
			t.Errorf("on Mget(%s) - expected len(vset) == %d, got: %d", key, i+1, len(vset))
		}
		for j := 1; j < len(vset); j++ {
			expected := []byte(fmt.Sprintf("%s_%03d", vprefix, j-1))
			if !compareByteArrays(vset[j], expected) {
				t.Errorf("on Mget(%s) - expected %s, got: %s", key, expected, vset[j])
			}
		}
	}

	asyncFlushAndQuitOnCompletion(t, client)

}

/* --------------- KEEP THIS AS LAST FUNCTION -------------- */
func TestEnd_asct(t *testing.T) {
	log.Println("-- asynchclient test completed")
}
