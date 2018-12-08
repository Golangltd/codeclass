// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
  definition of the current version of OpenCV
  Usefull to test in user programs
*/

package opencv

/*
#cgo CFLAGS  : -I./opencv110/cxcore/include

#include <cvver.h>
*/
import "C"

const (
	CV_MAJOR_VERSION    = 1
	CV_MINOR_VERSION    = 1
	CV_SUBMINOR_VERSION = 0
	CV_VERSION          = "1.1.0"
)

// private, just for test
const (
	_CV_MAJOR_VERSION    = int(C.CV_MAJOR_VERSION)
	_CV_MINOR_VERSION    = int(C.CV_MINOR_VERSION)
	_CV_SUBMINOR_VERSION = int(C.CV_SUBMINOR_VERSION)
	_CV_VERSION          = string(C.CV_VERSION)
)
