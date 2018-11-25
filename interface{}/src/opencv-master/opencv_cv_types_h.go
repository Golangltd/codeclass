// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

/*
#cgo CFLAGS: -I./opencv110/cv/include

#include <cv.h>
*/
import "C"

type CvMoments C.CvMoments

type CvHuMoments C.CvHuMoments
type CvConnectedComp C.CvConnectedComp
type CvContourScanner C.CvContourScanner
type CvChainPtReader C.CvChainPtReader
type CvContourTree C.CvContourTree
type CvConvexityDefect C.CvConvexityDefect
type CvSubdiv2DEdge C.CvSubdiv2DEdge
type CvQuadEdge2D C.CvQuadEdge2D
type CvSubdiv2DPoint C.CvSubdiv2DPoint
type CvSubdiv2D C.CvSubdiv2D
type CvSubdiv2DPointLocation C.CvSubdiv2DPointLocation
type CvNextEdgeType C.CvNextEdgeType

const (
	CV_DIST_USER   = C.CV_DIST_USER
	CV_DIST_L1     = C.CV_DIST_L1
	CV_DIST_L2     = C.CV_DIST_L2
	CV_DIST_C      = C.CV_DIST_C
	CV_DIST_L12    = C.CV_DIST_L12
	CV_DIST_FAIR   = C.CV_DIST_FAIR
	CV_DIST_WELSCH = C.CV_DIST_WELSCH
	CV_DIST_HUBER  = C.CV_DIST_HUBER
)

type CvFilter C.CvFilter

const CV_GAUSSIAN_5x5 = C.CV_GAUSSIAN_5x5

type CvConDensation C.CvConDensation
type CvKalman C.CvKalman
type CvHaarFeature C.CvHaarFeature
type CvHaarStageClassifier C.CvHaarStageClassifier
type CvHaarClassifierCascade C.CvHaarClassifierCascade
type CvAvgComp C.CvAvgComp
