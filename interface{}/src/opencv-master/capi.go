// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

/*
#cgo CFLAGS : -I./opencv110/cxcore/include -I./opencv110/cv/include -I./opencv110/cvaux/include  -I./opencv110/ml/include -I./opencv110/highgui/include -DCV_NO_BACKWARD_COMPATIBILITY
#cgo linux CXXFLAGS: -Wunused -DHAVE_CAMV4L2 -DHAVE_CAMV4L
#cgo windows LDFLAGS: -L. -lopencv110
#cgo linux LDFLAGS: -L. -lopencv110 -lm -ldl -lstdc++ -lgstapp-0.10

#cgo linux pkg-config: gtk+-2.0 gstreamer-0.10 libxine libavdevice libavformat libavfilter libavcodec libswscale libavutil

#include "opencv.h"
*/
import "C"
