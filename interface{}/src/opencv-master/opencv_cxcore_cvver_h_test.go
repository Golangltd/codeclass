// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

import (
	"testing"
)

func TestVersion(t *testing.T) {
	if a, b := CV_MAJOR_VERSION, _CV_MAJOR_VERSION; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
	if a, b := CV_MINOR_VERSION, _CV_MINOR_VERSION; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
	if a, b := CV_SUBMINOR_VERSION, _CV_SUBMINOR_VERSION; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
	if a, b := CV_VERSION, _CV_VERSION; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
}
