// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
Package opencv provides Go bindings for OpenCV 1.1.

Install `GCC` or `MinGW` (http://tdm-gcc.tdragon.net/download) at first,
and then run these commands:

	1. `go get -d github.com/chai2010/opencv`
	2. `go generate` and `go install`
	3. `go run hello.go`

Example:

	// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
	// Use of this source code is governed by a BSD-style
	// license that can be found in the LICENSE file.

	package main

	import (
		"fmt"
		"log"
		"os"

		"github.com/chai2010/opencv"
	)

	func main() {
		filename := "../images/lena.jpg"
		if len(os.Args) == 2 {
			filename = os.Args[1]
		}

		image := opencv.LoadImage(filename)
		if image == nil {
			log.Fatalf("LoadImage %s failed!", filename)
		}
		defer image.Release()

		win := opencv.NewWindow("Go-OpenCV")
		defer win.Destroy()

		win.SetMouseCallback(func(event, x, y, flags int) {
			fmt.Printf("event = %d, x = %d, y = %d, flags = %d\n",
				event, x, y, flags,
			)
		})
		win.CreateTrackbar("Thresh", 1, 100, func(pos int) {
			fmt.Printf("pos = %d\n", pos)
		})

		win.ShowImage(image)
		opencv.WaitKey(0)
	}

Report bugs to <chaishushan@gmail.com>.

Thanks!
*/
package opencv
