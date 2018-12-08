// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Windows Only !!!

// +build ingore

package main

import (
	"bytes"
	"flag"
	"fmt"
	"image/png"
	"io/ioutil"
	"log"
	"os"
	"sync"
	"time"

	"github.com/chai2010/opencv"
)

var (
	flagVerbose     = flag.Bool("verbose", false, "turns on/off console messages")
	flagMulti       = flag.Bool("multi", false, "allows for multithreaded")
	flagUseCallback = flag.Bool("use-callback", false, "use callback based capture - or single threaded")
	flagShowSetting = flag.Bool("show-settings", false, "show settings windows")
)

func main() {
	flag.Parse()

	vi := opencv.NewVideoInput()
	defer vi.Release()

	divices := vi.ListDevices()
	fmt.Printf("divices num: %d\n", len(divices))

	vi.SetVerbose(*flagVerbose)
	vi.SetComMultiThreaded(*flagMulti)
	vi.SetUseCallback(*flagUseCallback)

	for i, v := range divices {
		fmt.Printf("SetupDevice: divicesId = %d, name = %v\n", i, v)
		vi.SetupDevice(i)
		defer vi.StopDevice(i)

		if *flagShowSetting {
			fmt.Printf("ShowSettingsWindow: divicesId = %d, name = %v\n", i, v)
			vi.ShowSettingsWindow(i, true)
		}
	}

	os.RemoveAll("output")

	var wg sync.WaitGroup
	for i, v := range divices {
		wg.Add(1)
		go func(deviceId int, deviceName string) {
			defer wg.Done()

			endTime := time.Now().Add(time.Second * 30)
			for k := 0; ; k++ {
				for {
					if vi.HasNewFrame(deviceId) {
						log.Printf("saveFrame: name = %s, frame = %d\n", deviceName, k)
						saveFrame(deviceName, k, vi.GetFrame(deviceId))
						break
					}
				}
				if time.Now().After(endTime) {
					break
				}
				time.Sleep(50 * time.Millisecond)
			}
		}(i, v)
	}
	wg.Wait()

	fmt.Printf("Done\n")
}

func saveFrame(deviceName string, iFrame int, m *opencv.IplImage) {
	dir := fmt.Sprintf("./output/%s", deviceName)
	os.MkdirAll(dir, 0666)

	buf := new(bytes.Buffer)
	err := png.Encode(buf, m)
	if err != nil {
		log.Fatal(err)
	}
	err = ioutil.WriteFile(
		fmt.Sprintf("./output/%s/frame-%d.png", deviceName, iFrame),
		buf.Bytes(),
		0666,
	)
	if err != nil {
		log.Fatal(err)
	}
}
