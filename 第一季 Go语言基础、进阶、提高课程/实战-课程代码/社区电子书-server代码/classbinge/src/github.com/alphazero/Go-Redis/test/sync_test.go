package test

import (
	"fmt"
	"log"
	"redis"
	"testing"
)

func flushAndQuitOnCompletion(t *testing.T, client redis.Client) {
	// flush it
	e := client.Flushdb()
	if e != nil {
		t.Errorf("on Flushdb - %s", e)
	}
	e = client.Quit()
	if e != nil {
		t.Errorf("on Quit - %s", e)
	}
}

// Check that connection is actually passing passwords from spec
// and catching AUTH ERRs.
func TestSyncClientConnectWithBadSpec(t *testing.T) {
	spec := getTestConnSpec()
	spec.Password("bad-password")
	client, expected := redis.NewSynchClientWithSpec(spec)
	if expected == nil {
		t.Error("BUG: Expected a RedisError")
	}
	if client != nil {
		t.Error("BUG: sync client reference on error MUST be nil")
	}
}

// Check that connection is actually passing passwords from spec
func TestSyncClientConnectWithSpec(t *testing.T) {
	spec := getTestConnSpec()

	client, err := redis.NewSynchClientWithSpec(spec)
	if err != nil {
		t.Fatalf("failed to create client with spec. Error: %s", err)
	} else if client == nil {
		t.Fatal("BUG: client is nil")
	}
	client.Quit()
}

func TestPing(t *testing.T) {
	client := NewClient(t)

	if e := client.Ping(); e != nil {
		t.Errorf("on Ping() - %s", e)
	}

	flushAndQuitOnCompletion(t, client)
}

func TestQuit(t *testing.T) {
	client := NewClient(t)

	if e := client.Quit(); e != nil {
		t.Errorf("on Quit() - %s", e)
	}

	// now ping
	if e := client.Ping(); e == nil {
		t.Fatal("post Quit Ping() did not raise error")
	} else {
		log.Printf("expected - %s", e)
	}
}

func TestSet(t *testing.T) {
	client := NewClient(t)

	for k, v := range testdata[_testdata_kv].(map[string][]byte) {
		if e := client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
	}

	flushAndQuitOnCompletion(t, client)
}

func TestSetnx(t *testing.T) {
	client := NewClient(t)

	for k, v := range testdata[_testdata_kv].(map[string][]byte) {
		ok, e := client.Setnx(k, v)
		if e != nil {
			t.Errorf("on Setnx(%s, %s) - %s", k, v, e)
		}
		if !ok {
			t.Errorf("unexpected false returned by Setnx(%s, %s) - %s", k, v)
		}
	}
	for k, v := range testdata[_testdata_kv].(map[string][]byte) {
		ok, e := client.Setnx(k, v)
		if e != nil {
			t.Errorf("on Setnx(%s, %s) - %s", k, v, e)
		}
		if ok {
			t.Errorf("on Setnx(%s, %s) expected false on existing key- %s", k, v)
		}
	}

	flushAndQuitOnCompletion(t, client)
}

func TestGetset(t *testing.T) {
	client := NewClient(t)

	for k, v := range testdata[_testdata_kv].(map[string][]byte) {
		// set some
		e := client.Set(k, v)
		if e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
		// Getset with new new value newv
		newv := []byte(fmt.Sprint("newone_%s", k))
		prev, e := client.Getset(k, newv)
		if e != nil {
			t.Errorf("on Getset(%s, %s) - %s", k, newv, e)
		}
		// check previous prev value against expected v
		if prev == nil || !_test_compareByteArrays(prev, v) {
			t.Errorf("on Getset(%s, %s) - got: %s", k, newv, prev)
		}
		// now check that newvalue was correctly set as well
		got, e := client.Get(k)
		if e != nil {
			t.Errorf("on Getset(%s, %s) - %s", k, newv, e)
		}
		if !_test_compareByteArrays(got, newv) {
			t.Errorf("on Get(%s) - got: %s expected:%s", k, got, newv)
		}

	}

	flushAndQuitOnCompletion(t, client)
}

func TestSetThenGet(t *testing.T) {
	client := NewClient(t)

	for k, v := range testdata[_testdata_kv].(map[string][]byte) {
		if e := client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
		got, e := client.Get(k)
		if e != nil {
			t.Errorf("on Get(%s) - %s", k, e)
		}
		if got == nil {
			t.Errorf("on Get(%s) - got nil", k)
		}
		if !_test_compareByteArrays(got, v) {
			t.Errorf("on Get(%s) - got:%s expected:%s", k, got, v)
		}
	}

	flushAndQuitOnCompletion(t, client)
}

func TestMget(t *testing.T) {
	client := NewClient(t)

	vprefix := "the-value"
	key := "mget-test"
	if e := client.Set(key, []byte(vprefix)); e != nil {
		t.Errorf("on Set(%s, %s) - %s", key, vprefix, e)
	}
	vset, e := client.Mget(key, nil)
	if e != nil {
		t.Errorf("on Mget(%s) - %s", key, e)
	}
	if vset == nil {
		t.Errorf("on Mget(%s) - got nil", key)
	}
	if len(vset) != 1 {
		t.Errorf("on Mget(%s) - expected len(vset) == 1, got: %d", key, len(vset))
	}
	if !_test_compareByteArrays(vset[0], []byte(vprefix)) {
		t.Errorf("on Mget(%s) - expected %s, got: %s", key, vprefix, vset[0])
	}

	keys := testdata[_testdata_keys].([]string)
	for i, k := range keys {
		if i > 10 {
			break
		}
		v := []byte(fmt.Sprintf("%s_%03d", vprefix, i))
		if e := client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
		vset, e := client.Mget(key, keys[0:i])
		if e != nil {
			t.Errorf("on Mget(%s) - %s", key, e)
		}
		if vset == nil {
			t.Errorf("on Mget(%s) - got nil", key)
		}
		if len(vset) != 1+i {
			t.Errorf("on Mget(%s) - expected len(vset) == %d, got: %d", key, i+1, len(vset))
		}
		for j := 1; j < len(vset); j++ {
			expected := []byte(fmt.Sprintf("%s_%03d", vprefix, j-1))
			if !_test_compareByteArrays(vset[j], expected) {
				t.Errorf("on Mget(%s) - expected %s, got: %s", key, expected, vset[j])
			}
		}
	}

	flushAndQuitOnCompletion(t, client)
}

func TestGetType(t *testing.T) {
	client := NewClient(t)

	var expected, got redis.KeyType
	var key string

	key = "string-key"
	if e := client.Set(key, []byte("woof")); e != nil {
		t.Errorf("on Set - %s", e)
	}
	expected = redis.RT_STRING
	got, e := client.Type(key)
	if e != nil {
		t.Errorf("on Type(%s) - %s", key, e)
	}
	if got != expected {
		t.Errorf("on Type() expected:%d got:%d - %s", expected, got)
	}

	key = "set-key"
	if _, e := client.Sadd(key, []byte("woof")); e != nil {
		t.Errorf("on Sadd - %s", e)
	}
	expected = redis.RT_SET
	got, e = client.Type(key)
	if e != nil {
		t.Errorf("on Type(%s) - %s", key, e)
	}
	if got != expected {
		t.Errorf("on Type() expected:%d got:%d - %s", expected, got)
	}

	key = "list-key"
	if e = client.Lpush(key, []byte("woof")); e != nil {
		t.Errorf("on Lpush - %s", e)
	}
	expected = redis.RT_LIST
	got, e = client.Type(key)
	if e != nil {
		t.Errorf("on Type(%s) - %s", key, e)
	}
	if got != expected {
		t.Errorf("on Type() expected:%d got:%d - %s", expected, got)
	}

	key = "zset-key"
	if _, e = client.Zadd(key, float64(0), []byte("woof")); e != nil {
		t.Errorf("on Zadd - %s", e)
	}
	expected = redis.RT_ZSET
	got, e = client.Type(key)
	if e != nil {
		t.Errorf("on Type(%s) - %s", key, e)
	}
	if got != expected {
		t.Errorf("on Type() expected:%d got:%d - %s", expected, got)
	}

	flushAndQuitOnCompletion(t, client)
}

func TestSave(t *testing.T) {
	client := NewClient(t)

	if e := client.Save(); e != nil {
		t.Errorf("on Save() - %s", e)
	}

	flushAndQuitOnCompletion(t, client)
}

func TestAllKeys(t *testing.T) {
	client := NewClient(t)

	kvmap := testdata[_testdata_kv].(map[string][]byte)
	for k, v := range kvmap {
		if e := client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
	}
	got, e := client.AllKeys()
	if e != nil {
		t.Errorf("on AllKeys() - %s", e)
	}
	if got == nil {
		t.Errorf("on AllKeys() - got nil")
	}

	// if same length and all elements in kvmap, its ok
	if len(got) != len(kvmap) {
		t.Errorf("on AllKeys() - Len mismatch - got:%d expected:%d", len(got), len(kvmap))
	}
	for _, k := range got {
		if kvmap[k] == nil {
			t.Errorf("on AllKeys() - key %s is not in original kvmap", k)
		}
	}

	flushAndQuitOnCompletion(t, client)
}

func TestKeys(t *testing.T) {
	client := NewClient(t)

	prefix := "prefix_"
	kvmap := testdata[_testdata_kv].(map[string][]byte)
	// add dataset keys and keys prefixed
	for key, v := range kvmap {
		k := fmt.Sprintf("%s%s", prefix, key)
		if e := client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
		if e := client.Set(key, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", key, v, e)
		}
	}
	got, e := client.Keys(prefix + "*")
	if e != nil {
		t.Errorf("on Keys() - %s", e)
	}
	if got == nil {
		t.Errorf("on Keys() - got nil")
	}

	// if same length and all elements in kvmap, its ok
	if len(got) != len(kvmap) {
		t.Errorf("on Keys() - Len mismatch - got:%d expected:%d", len(got), len(kvmap))
	}
	for _, k := range got {
		if kvmap[k[len(prefix):]] == nil {
			t.Errorf("on Keys() - key %s is not in original kvmap", k)
		}
	}

	flushAndQuitOnCompletion(t, client)
}

func TestExists(t *testing.T) {
	client := NewClient(t)

	kvmap := testdata[_testdata_kv].(map[string][]byte)
	for k, v := range kvmap {
		res, e := client.Exists(k)
		if e != nil {
			t.Errorf("on Exists(%s) - %s", k, e)
		}
		if res {
			t.Errorf("on Exists(%s) - unexpected res True", k)
		}
		if e = client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
		res, e = client.Exists(k)
		if e != nil {
			t.Errorf("on Exists(%s) - %s", k, e)
		}
		if !res {
			t.Errorf("on Exists(%s)", k)
		}
	}

	flushAndQuitOnCompletion(t, client)
}

func TestRename(t *testing.T) {
	client := NewClient(t)

	prefix := "renamed_"
	kvmap := testdata[_testdata_kv].(map[string][]byte)
	// add dataset keys and keys prefixed
	for k, v := range kvmap {
		if e := client.Set(k, v); e != nil {
			t.Errorf("on Set(%s, %s) - %s", k, v, e)
		}
	}
	for key, _ := range kvmap {
		newkey := fmt.Sprintf("%s%s", prefix, key)
		if e := client.Rename(key, newkey); e != nil {
			t.Errorf("on Rename(%s, %s) - %s", key, newkey, e)
		}
	}

	got, e := client.Keys(prefix + "*")
	if e != nil {
		t.Errorf("on Keys() - %s", e)
	}
	if got == nil {
		t.Errorf("on Keys() - got nil")
	}

	// if same length and all elements in kvmap, its ok
	if len(got) != len(kvmap) {
		t.Errorf("on Keys() testing Rename() - Len mismatch - got:%d expected:%d", len(got), len(kvmap))
	}
	for _, k := range got {
		if kvmap[k[len(prefix):]] == nil {
			t.Errorf("on Keys() testing Rename() - key %s is not in original kvmap", k)
		}
	}

	flushAndQuitOnCompletion(t, client)
}

/* --------------- KEEP THIS AS LAST FUNCTION -------------- */
func TestEnd_sct(t *testing.T) {
	log.Println("-- synchclient test completed")
}
