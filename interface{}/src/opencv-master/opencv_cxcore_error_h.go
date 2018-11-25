// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

/*
#cgo CFLAGS  : -I./opencv110/cxcore/include

#include <cxerror.h>
*/
import "C"
import "fmt"

/* this part of CVStatus is compatible with IPLStatus
Some of below symbols are not [yet] used in OpenCV
*/

type CVStatus C.int

func (s CVStatus) IsOK() bool {
	return s == CV_StsOk
}

func (s CVStatus) Name() string {
	if name, ok := _CVStatusNameTable[s]; ok {
		return name
	}
	return fmt.Sprintf("CVStatus_Unknown(%d)", int(s))
}

const (
	CV_StsOk                     = CVStatus(C.CV_StsOk)                     /* everithing is ok                */
	CV_StsBackTrace              = CVStatus(C.CV_StsBackTrace)              /* pseudo error for back trace     */
	CV_StsError                  = CVStatus(C.CV_StsError)                  /* unknown /unspecified error      */
	CV_StsInternal               = CVStatus(C.CV_StsInternal)               /* internal error (bad state)      */
	CV_StsNoMem                  = CVStatus(C.CV_StsNoMem)                  /* insufficient memory             */
	CV_StsBadArg                 = CVStatus(C.CV_StsBadArg)                 /* function arg/param is bad       */
	CV_StsBadFunc                = CVStatus(C.CV_StsBadFunc)                /* unsupported function            */
	CV_StsNoConv                 = CVStatus(C.CV_StsNoConv)                 /* iter. didn't converge           */
	CV_StsAutoTrace              = CVStatus(C.CV_StsAutoTrace)              /* tracing                         */
	CV_HeaderIsNull              = CVStatus(C.CV_HeaderIsNull)              /* image header is NULL            */
	CV_BadImageSize              = CVStatus(C.CV_BadImageSize)              /* image size is invalid           */
	CV_BadOffset                 = CVStatus(C.CV_BadOffset)                 /* offset is invalid               */
	CV_BadDataPtr                = CVStatus(C.CV_BadDataPtr)                /**/
	CV_BadStep                   = CVStatus(C.CV_BadStep)                   /**/
	CV_BadModelOrChSeq           = CVStatus(C.CV_BadModelOrChSeq)           /**/
	CV_BadNumChannels            = CVStatus(C.CV_BadNumChannels)            /**/
	CV_BadNumChannel1U           = CVStatus(C.CV_BadNumChannel1U)           /**/
	CV_BadDepth                  = CVStatus(C.CV_BadDepth)                  /**/
	CV_BadAlphaChannel           = CVStatus(C.CV_BadAlphaChannel)           /**/
	CV_BadOrder                  = CVStatus(C.CV_BadOrder)                  /**/
	CV_BadOrigin                 = CVStatus(C.CV_BadOrigin)                 /**/
	CV_BadAlign                  = CVStatus(C.CV_BadAlign)                  /**/
	CV_BadCallBack               = CVStatus(C.CV_BadCallBack)               /**/
	CV_BadTileSize               = CVStatus(C.CV_BadTileSize)               /**/
	CV_BadCOI                    = CVStatus(C.CV_BadCOI)                    /**/
	CV_BadROISize                = CVStatus(C.CV_BadROISize)                /**/
	CV_MaskIsTiled               = CVStatus(C.CV_MaskIsTiled)               /**/
	CV_StsNullPtr                = CVStatus(C.CV_StsNullPtr)                /* null pointer */
	CV_StsVecLengthErr           = CVStatus(C.CV_StsVecLengthErr)           /* incorrect vector length */
	CV_StsFilterStructContentErr = CVStatus(C.CV_StsFilterStructContentErr) /* incorr. filter structure content */
	CV_StsKernelStructContentErr = CVStatus(C.CV_StsKernelStructContentErr) /* incorr. transform kernel content */
	CV_StsFilterOffsetErr        = CVStatus(C.CV_StsFilterOffsetErr)        /* incorrect filter ofset value */
	CV_StsBadSize                = CVStatus(C.CV_StsBadSize)                /* the input/output structure size is incorrect  */
	CV_StsDivByZero              = CVStatus(C.CV_StsDivByZero)              /* division by zero */
	CV_StsInplaceNotSupported    = CVStatus(C.CV_StsInplaceNotSupported)    /* in-place operation is not supported */
	CV_StsObjectNotFound         = CVStatus(C.CV_StsObjectNotFound)         /* request can't be completed */
	CV_StsUnmatchedFormats       = CVStatus(C.CV_StsUnmatchedFormats)       /* formats of input/output arrays differ */
	CV_StsBadFlag                = CVStatus(C.CV_StsBadFlag)                /* flag is wrong or not supported */
	CV_StsBadPoint               = CVStatus(C.CV_StsBadPoint)               /* bad CvPoint */
	CV_StsBadMask                = CVStatus(C.CV_StsBadMask)                /* bad format of mask (neither 8uC1 nor 8sC1)*/
	CV_StsUnmatchedSizes         = CVStatus(C.CV_StsUnmatchedSizes)         /* sizes of input/output structures do not match */
	CV_StsUnsupportedFormat      = CVStatus(C.CV_StsUnsupportedFormat)      /* the data format/type is not supported by the function*/
	CV_StsOutOfRange             = CVStatus(C.CV_StsOutOfRange)             /* some of parameters are out of range */
	CV_StsParseError             = CVStatus(C.CV_StsParseError)             /* invalid syntax/structure of the parsed file */
	CV_StsNotImplemented         = CVStatus(C.CV_StsNotImplemented)         /* the requested function/feature is not implemented */
	CV_StsBadMemBlock            = CVStatus(C.CV_StsBadMemBlock)            /* an allocated block has been corrupted */
)

var _CVStatusNameTable = map[CVStatus]string{
	CV_StsOk:                     "CV_StsOk",
	CV_StsBackTrace:              "CV_StsBackTrace",
	CV_StsError:                  "CV_StsError",
	CV_StsInternal:               "CV_StsInternal",
	CV_StsNoMem:                  "CV_StsNoMem",
	CV_StsBadArg:                 "CV_StsBadArg",
	CV_StsBadFunc:                "CV_StsBadFunc",
	CV_StsNoConv:                 "CV_StsNoConv",
	CV_StsAutoTrace:              "CV_StsAutoTrace",
	CV_HeaderIsNull:              "CV_HeaderIsNull",
	CV_BadImageSize:              "CV_BadImageSize",
	CV_BadOffset:                 "CV_BadOffset",
	CV_BadDataPtr:                "CV_BadDataPtr",
	CV_BadStep:                   "CV_BadStep",
	CV_BadModelOrChSeq:           "CV_BadModelOrChSeq",
	CV_BadNumChannels:            "CV_BadNumChannels",
	CV_BadNumChannel1U:           "CV_BadNumChannel1U",
	CV_BadDepth:                  "CV_BadDepth",
	CV_BadAlphaChannel:           "CV_BadAlphaChannel",
	CV_BadOrder:                  "CV_BadOrder",
	CV_BadOrigin:                 "CV_BadOrigin",
	CV_BadAlign:                  "CV_BadAlign",
	CV_BadCallBack:               "CV_BadCallBack",
	CV_BadTileSize:               "CV_BadTileSize",
	CV_BadCOI:                    "CV_BadCOI",
	CV_BadROISize:                "CV_BadROISize",
	CV_MaskIsTiled:               "CV_MaskIsTiled",
	CV_StsNullPtr:                "CV_StsNullPtr",
	CV_StsVecLengthErr:           "CV_StsVecLengthErr",
	CV_StsFilterStructContentErr: "CV_StsFilterStructContentErr",
	CV_StsKernelStructContentErr: "CV_StsKernelStructContentErr",
	CV_StsFilterOffsetErr:        "CV_StsFilterOffsetErr",
	CV_StsBadSize:                "CV_StsBadSize",
	CV_StsDivByZero:              "CV_StsDivByZero",
	CV_StsInplaceNotSupported:    "CV_StsInplaceNotSupported",
	CV_StsObjectNotFound:         "CV_StsObjectNotFound",
	CV_StsUnmatchedFormats:       "CV_StsUnmatchedFormats",
	CV_StsBadFlag:                "CV_StsBadFlag",
	CV_StsBadPoint:               "CV_StsBadPoint",
	CV_StsBadMask:                "CV_StsBadMask",
	CV_StsUnmatchedSizes:         "CV_StsUnmatchedSizes",
	CV_StsUnsupportedFormat:      "CV_StsUnsupportedFormat",
	CV_StsOutOfRange:             "CV_StsOutOfRange",
	CV_StsParseError:             "CV_StsParseError",
	CV_StsNotImplemented:         "CV_StsNotImplemented",
	CV_StsBadMemBlock:            "CV_StsBadMemBlock",
}
