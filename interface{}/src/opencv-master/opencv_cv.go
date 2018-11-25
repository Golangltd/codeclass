// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

/*
#cgo CFLAGS: -I./opencv110/cv/include

#include <cv.h>
*/
import "C"
import (
	"unsafe"
)

const (
	CV_BGR2GRAY = C.CV_BGR2GRAY

	CV_BLUR = C.CV_BLUR
)

/* Smoothes array (removes noise) */
func Smooth(src, dst *IplImage, smoothtype,
	param1, param2 int, param3, param4 float64) {
	C.cvSmooth(unsafe.Pointer(src), unsafe.Pointer(dst), C.int(smoothtype),
		C.int(param1), C.int(param2), C.double(param3), C.double(param4),
	)
}

/* Converts input array pixels from one color space to another */
func CvtColor(src, dst *IplImage, code int) {
	C.cvCvtColor(unsafe.Pointer(src), unsafe.Pointer(dst), C.int(code))
}

/* Runs canny edge detector */
func Canny(image, edges *IplImage, threshold1, threshold2 float64, aperture_size int) {
	C.cvCanny(unsafe.Pointer(image), unsafe.Pointer(edges),
		C.double(threshold1), C.double(threshold2),
		C.int(aperture_size),
	)
}

const (
	CV_INPAINT_NS    = C.CV_INPAINT_NS
	CV_INPAINT_TELEA = C.CV_INPAINT_TELEA
)

/* Inpaints the selected region in the image */
func Inpaint(src, inpaint_mask, dst *IplImage, inpaintRange float64, flags int) {
	C.cvInpaint(
		unsafe.Pointer(src),
		unsafe.Pointer(inpaint_mask),
		unsafe.Pointer(dst),
		C.double(inpaintRange),
		C.int(flags),
	)
}
