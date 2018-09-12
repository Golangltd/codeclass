# what is it

This is an experimental specification and interface based blackbox tests for Go-Redis clients.  The generator creates individual
test files for each method on a given client type.  The test files for each client type are written to an eponymous sub-dir of
<distro>/test.  For example, files for AsyncClient end up in <distro-root>/test/AsyncClient.

The generator has dependency on redis and redis/test packages.  redis.Spefication's MethodSpec and friends provide the canonical
spec for each method (wip).  redis/test/support.go provides some basic helpers used by the generated tests (and in future hand-written
tests as well.)

It is a work in progress.  (See stat-log below).

## generate tests
 
Generate tests:

	go run generate.go

## run the tests

Run the tests:

	cd <distro-root>/test/<client-type>
	go test 

# stat-log

### SEPT-19-2012

* QuickTests in the minimal are done and actually catching bugs.
* (Who ever contributed the Blpop, etc. wrote buggy code - Client (sync) tests hang on these so rm them after gen'ing the suite.
* Think time on how to formally spec known-state tests for methods e.g. Get("foo") should return value IFF Set("foo", "woof") preceeded it.




