// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build ingore

package main

import (
	"crypto/tls"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
)

// https://github.com/Itseez/opencv/tree/master/3rdparty/ffmpeg
// https://raw.githubusercontent.com/Itseez/opencv/master/3rdparty/ffmpeg/opencv_ffmpeg.dll
// https://raw.githubusercontent.com/Itseez/opencv/master/3rdparty/ffmpeg/opencv_ffmpeg_64.dll
const baseURL = "https://raw.githubusercontent.com/Itseez/opencv/master/3rdparty/ffmpeg/"

func main() {
	var err error

	if err = GetFfmpegDll("opencv_ffmpeg.dll"); err != nil {
		log.Fatal(err)
	}
	fmt.Println("Download opencv_ffmpeg.dll ok")

	if err = GetFfmpegDll("opencv_ffmpeg_64.dll"); err != nil {
		log.Fatal(err)
	}
	fmt.Println("Download opencv_ffmpeg_64.dll ok")

	fmt.Println("Done")
}

func GetFfmpegDll(filename string) (errRet error) {
	f, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("failed to create %s: %v", filename, err)
	}
	defer f.Close()
	defer func() {
		if errRet != nil {
			os.Remove(filename)
		}
	}()

	tr := &http.Transport{
		TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
	}
	client := &http.Client{Transport: tr}

	resp, err := client.Get(baseURL + filename)
	if err != nil {
		return fmt.Errorf("failed to download %s: %v", baseURL+filename, err)
	}
	defer resp.Body.Close()

	_, err = io.Copy(f, resp.Body)
	if err != nil {
		return fmt.Errorf("failed to write %s: %v", filename, err)
	}
	return nil
}
