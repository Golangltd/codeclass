// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

import (
	"testing"
)

func TestCVStatus_IsOK(t *testing.T) {
	if ok := CV_StsOk.IsOK(); !ok {
		t.Fatalf("expect = %v, got = %v", true, ok)
	}
}

func TestCVStatus_Name(t *testing.T) {
	for k, v := range _CVStatusNameTable {
		if a, b := k.Name(), v; a != b {
			t.Fatalf("expect = %v, got = %v", b, a)
		}
	}
	if a, b := CVStatus(CV_StsOk).Name(), "CV_StsOk"; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
	if a, b := CVStatus(CV_BadNumChannel1U).Name(), "CV_BadNumChannel1U"; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
	if a, b := CVStatus(CV_StsObjectNotFound).Name(), "CV_StsObjectNotFound"; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
	if a, b := CVStatus(CV_StsBadMemBlock).Name(), "CV_StsBadMemBlock"; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
}

func TestCVStatus_Name_unknown(t *testing.T) {
	if a, b := CVStatus(9527).Name(), "CVStatus_Unknown(9527)"; a != b {
		t.Fatalf("expect = %v, got = %v", b, a)
	}
}
