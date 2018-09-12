// REVU - whitebox testing of internal comps -- OK.

package redis

import (
	"bytes"
	"log"
	"testing"
	"time"
)

// REVU: nice to test timing accuracy but it isn't really necessary.

// basic test data
type tspec_ft struct {
	data  []byte
	delay time.Duration
}

// create the test spec (test data for now)
func testspec_ft() tspec_ft {
	var testspec tspec_ft

	// []byte data to be used
	data := "Hello there!"
	testspec.data = bytes.NewBufferString(data).Bytes()

	// using a timeout of 100 (msecs)
	delaysecs := 100
	testspec.delay = time.Duration(delaysecs) * time.Millisecond

	return testspec
}

// test FutureBytes - all aspect, expect blocking Get
// are tested.  A future object is created (but not set)
// and timeout and error tests are made.  Then value is set
// and test repeated.
func TestFutureContract(t *testing.T) {

	// prep data
	tspec := testspec_ft()

	// using basic FutureBytes
	fb := newFutureBytes()

	/* TEST timed call to uninitialized (not Set) future value,
	 * expecting a timeout.
	 * MUST return timeout of true
	 * MUST return error value of nil
	 * MUST return value of nil
	 */
	fvalue1, e1, timedout1 := fb.TryGet(tspec.delay)
	if !timedout1 {
		t.Error("BUG: timeout expected")
	}
	if e1 != nil {
		t.Error("Bug: unexpected error %s", e1)
	}
	if fvalue1 != nil {
		t.Error("Bug: value returned: %s", fvalue1)
	}

	// set the future result
	fb.set(tspec.data)

	/* TEST timed call to initialized (set) future value
	 * expecting data, no error and no timeout
	 * MUST return timeout of false
	 * MUST return error of nil
	 * MUST return value equal to data
	 */
	fvalue2, e2, timedout2 := fb.TryGet(tspec.delay)
	if timedout2 {
		t.Error("BUG: should not timeout")
	}
	if e2 != nil {
		t.Error("Bug: unexpected error %s", e2)
	}
	if fvalue2 == nil {
		t.Error("Bug: should not return future nil")
	} else if bytes.Compare(fvalue2, tspec.data) != 0 {
		t.Error("Bug: future value not equal to data set")
	}
}

func TestFutureWithBlockingGet(t *testing.T) {

	// prep data
	// prep data
	tspec := testspec_ft()

	// using basic FutureBytes
	fb := newFutureBytes()

	// test go routine will block on Get until
	// value is set.
	sig := make(chan bool, 1)
	go func() {
		/* TEST timed call to initialized (set) future value
		 * expecting data, no error and no timeout
		 * MUST return error of nil
		 * MUST return value equal to data
		 */
		fvalue, e := fb.Get()
		if e != nil {
			t.Error("Bug: unexpected error %s", e)
		}
		if fvalue == nil {
			t.Error("Bug: should not return future nil")
		} else if bytes.Compare(fvalue, tspec.data) != 0 {
			t.Error("Bug: future value not equal to data set")
		}
		sig <- true
	}()

	// set the data
	fb.set(tspec.data)

	<-sig

}

func TestFutureTimedBlockingGet(t *testing.T) {

	// prep data
	// prep data
	tspec := testspec_ft()

	// using basic FutureBytes
	fb := newFutureBytes()

	// test go routine will block on Get until
	// value is set or timeout expires
	sig := make(chan bool, 1)
	go func() {
		/* TEST timed call to initialized (set) future value
		 * expecting data, no error and no timeout
		 * MUST return error of nil
		 * MUST return value equal to data
		 */
		fvalue, e, timedout := fb.TryGet(tspec.delay)
		if timedout {
			t.Error("BUG: should not timeout")
		}
		if e != nil {
			t.Error("Bug: unexpected error %s", e)
		}
		if fvalue == nil {
			t.Error("Bug: should not return future nil")
		} else if bytes.Compare(fvalue, tspec.data) != 0 {
			t.Error("Bug: future value not equal to data set")
		}
		sig <- true
	}()

	// set the data
	fb.set(tspec.data)

	<-sig

}

func TestEnd_future(t *testing.T) {
	// nop
	log.Println("-- future test completed")
}
