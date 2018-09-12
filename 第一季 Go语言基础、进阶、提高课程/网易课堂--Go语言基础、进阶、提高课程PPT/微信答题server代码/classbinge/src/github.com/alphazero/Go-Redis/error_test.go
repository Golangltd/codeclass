// REVU - whitebox testing of internal comps -- OK.

package redis

/*
import (
	"log"
	"testing"
)

type simpleError struct {
}

func (e simpleError) Error() string {
	return "a simple error"
}

type tspec_et struct {
	msg      string
	category ErrorCategory
	cause    error
}

func testspec_et() tspec_et {
	var spec tspec_et
	spec.msg = "this is a test error message"
//	spec.category = SYSTEM_ERR
	return spec
}

func commonErrorTest(t *testing.T, e Error, category ErrorCategory) {
	if e.Error() == "" {
		t.Error("BUG: nil results for e.Error()")
	}
	if e.Message() == "" {
		t.Error("BUG: nil results for e.Message()")
	}
	if e.Category() != category {
		t.Errorf("BUG: category not set correctly (exp:%s | got:%s)", category, e.Category())
	}
}

func TestNewError(t *testing.T) {

	spec := testspec_et()
	e := NewError(spec.category, spec.msg)
	commonErrorTest(t, e, spec.category)
}

func TestPrivateNewRedisError(t *testing.T) {

	spec := testspec_et()
	e := _newRedisError(spec.category, spec.msg)
	commonErrorTest(t, e, spec.category)
}

func TestNewRedisError(t *testing.T) {

	spec := testspec_et()
	e := NewRedisError(spec.msg)
	commonErrorTest(t, e, REDIS_ERR)

	if !e.IsRedisError() {
		t.Error("BUG: IsRedisError")
	}
}

func TestNewSystemError(t *testing.T) {

	spec := testspec_et()
	e := NewSystemError(spec.msg)
	commonErrorTest(t, e, SYSTEM_ERR)

	if e.IsRedisError() {
		t.Error("BUG: IsRedisError")
	}
}

func TestNewErrorWithCause(t *testing.T) {

	spec := testspec_et()

	var cause simpleError
	e := NewErrorWithCause(spec.category, spec.msg, cause)

	commonErrorTest(t, e, spec.category)

	if e.Cause() == nil {
		t.Error("BUG: cause is nil")
	}
	if e.Cause() != cause {
		t.Error("BUG: cause not set correctly")
	}
}

func TestEnd_et(t *testing.T) {
	log.Println("-- error test completed")
}
*/
