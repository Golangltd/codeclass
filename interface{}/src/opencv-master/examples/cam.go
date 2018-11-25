// Copyright 2011 <chaishushan@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build ingore

package main

import (
	"flag"
	"fmt"
	"log"

	"github.com/chai2010/opencv"
)

var (
	flagCamNum = flag.Int("cam-num", 1, "set camera num")
)

func main() {
	flag.Parse()

	cameras := make([]*opencv.Capture, *flagCamNum)
	for i := 0; i < len(cameras); i++ {
		cameras[i] = opencv.NewCameraCapture(i)
		if cameras[i] == nil {
			log.Fatalf("can not open camera %d", i)
		}
		defer cameras[i].Release()
	}

	writers := make([]*opencv.VideoWriter, *flagCamNum)
	for i := 0; i < len(writers); i++ {
		m := cameras[i].QueryFrame()
		writers[i] = opencv.NewVideoWriter(
			fmt.Sprintf("cam%d_output.avi", i),
			opencv.FOURCC('M', 'J', 'P', 'G'), 30,
			m.GetWidth(), m.GetHeight(),
			1,
		)
		if writers[i] == nil {
			log.Fatalf("can not create writer %d", i)
		}
		defer writers[i].Release()
	}

	windows := make([]*opencv.Window, *flagCamNum)
	for i := 0; i < len(windows); i++ {
		windows[i] = opencv.NewWindow(fmt.Sprintf("Camera: %d", i))
		defer windows[i].Destroy()
	}

	for i := 0; ; i++ {
		for idx := 0; idx < *flagCamNum; idx++ {
			if m := cameras[idx].QueryFrame(); m != nil {
				fmt.Printf("camera(%d): Frame %d\n", idx, i)
				windows[idx].ShowImage(m)
				writers[idx].WriteFrame(m)
			}
		}
		if key := opencv.WaitKey(30); key == 27 {
			break
		}
	}
	fmt.Println("Done")
}
