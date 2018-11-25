// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv_test

import (
	"fmt"

	opencv "github.com/chai2010/opencv"
)

func Example_version() {
	fmt.Printf("OpenCV %v\n", opencv.CV_VERSION)
	// Output:
	// OpenCV 1.1.0
}
