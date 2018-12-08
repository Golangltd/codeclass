// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

/*
#cgo CFLAGS: -I./opencv110/cxcore/include

#include <cxcore.h>
*/
import "C"
import (
	"image"
	"image/color"
)

func (p *IplImage) ColorModel() color.Model {
	return color.RGBAModel
}

func (p *IplImage) Bounds() image.Rectangle {
	return image.Rect(0, 0, p.GetWidth(), p.GetHeight())
}

func (p *IplImage) At(x, y int) color.Color {
	if !(image.Point{x, y}.In(p.Bounds())) {
		return color.RGBA{}
	}
	i := p.PixOffset(x, y)
	d := p.GetImageData()
	switch p.GetChannels() {
	case 1:
		return color.RGBA{
			R: d[i+0],
			G: d[i+0],
			B: d[i+0],
			A: 0xFF,
		}
	case 3:
		return color.RGBA{
			R: d[i+2],
			G: d[i+1],
			B: d[i+0],
			A: 0xFF,
		}
	case 4:
		return color.RGBA{
			R: d[i+0],
			G: d[i+1],
			B: d[i+2],
			A: d[i+3],
		}
	}
	return color.RGBA{}
}

func (p *IplImage) PixOffset(x, y int) int {
	return y*p.GetWidthStep() + x
}
