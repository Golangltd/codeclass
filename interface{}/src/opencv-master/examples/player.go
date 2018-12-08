// Copyright 2011 <chaishushan@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build ingore

package main

import (
	"fmt"
	"log"
	"os"

	"github.com/chai2010/opencv"
)

func main() {
	filename := "../testdata/test.avi"
	if len(os.Args) >= 2 {
		filename = os.Args[1]
	}

	cap := opencv.NewFileCapture(filename)
	if cap == nil {
		log.Fatalf("can not open video %s", filename)
	}
	defer cap.Release()

	win := opencv.NewWindow("GoOpenCV: VideoPlayer")
	defer win.Destroy()

	fps := int(cap.GetProperty(opencv.CV_CAP_PROP_FPS))
	frames := int(cap.GetProperty(opencv.CV_CAP_PROP_FRAME_COUNT))
	stop := false

	win.SetMouseCallback(func(event, x, y, flags int) {
		if flags&opencv.CV_EVENT_LBUTTONDOWN != 0 {
			stop = !stop
			if stop {
				fmt.Printf("status: stop\n")
			} else {
				fmt.Printf("status: palying\n")
			}
		}
	})
	win.CreateTrackbar("Seek", 1, frames, func(pos int) {
		cur_pos := int(cap.GetProperty(opencv.CV_CAP_PROP_POS_FRAMES))

		if pos != cur_pos {
			cap.SetProperty(opencv.CV_CAP_PROP_POS_FRAMES, float64(pos))
			fmt.Printf("Seek to %d(%d)\n", pos, frames)
		}
	})

	for {
		if !stop {
			img := cap.QueryFrame()
			if img == nil {
				break
			}

			frame_pos := int(cap.GetProperty(opencv.CV_CAP_PROP_POS_FRAMES))
			if frame_pos >= frames {
				break
			}
			win.SetTrackbarPos("Seek", frame_pos)

			win.ShowImage(img)

			if key := opencv.WaitKey(1000 / fps); key == 27 {
				break
			}
		} else {
			if key := opencv.WaitKey(20); key == 27 {
				break
			}
		}
	}
}
