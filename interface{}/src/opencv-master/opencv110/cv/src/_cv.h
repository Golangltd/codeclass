/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef _CV_INTERNAL_H_
#define _CV_INTERNAL_H_

#if defined _MSC_VER && _MSC_VER >= 1200
    /* disable warnings related to inline functions */
    #pragma warning( disable: 4711 4710 4514 )
#endif

#include "cv.h"
#include "cxmisc.h"
#include "cvcompat.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>

typedef unsigned char uchar;
typedef unsigned short ushort;

#ifdef __BORLANDC__
    #define     WIN32
    #define     CV_DLL
    #undef      _CV_ALWAYS_PROFILE_
    #define     _CV_ALWAYS_NO_PROFILE_
#endif

/* helper tables */
extern const uchar icvSaturate8u_cv[];
#define CV_FAST_CAST_8U(t)  (assert(-256 <= (t) || (t) <= 512), icvSaturate8u_cv[(t)+256])
#define CV_CALC_MIN_8U(a,b) (a) -= CV_FAST_CAST_8U((a) - (b))
#define CV_CALC_MAX_8U(a,b) (a) += CV_FAST_CAST_8U((b) - (a))

// -256.f ... 511.f
extern const float icv8x32fTab_cv[];
#define CV_8TO32F(x)  icv8x32fTab_cv[(x)+256]

// (-128.f)^2 ... (255.f)^2
extern const float icv8x32fSqrTab[];
#define CV_8TO32F_SQR(x)  icv8x32fSqrTab[(x)+128]

CV_INLINE  CvDataType icvDepthToDataType( int type );
CV_INLINE  CvDataType icvDepthToDataType( int type )
{
    return (CvDataType)(
            ((((int)cv8u)|((int)cv8s << 4)|((int)cv16u << 8)|
              ((int)cv16s << 12)|((int)cv32s << 16)|((int)cv32f << 20)|
              ((int)cv64f << 24)) >> CV_MAT_DEPTH(type)*4) & 15);
}

#define CV_HIST_DEFAULT_TYPE CV_32F

CV_EXTERN_C_FUNCPTR( void (CV_CDECL * CvWriteNodeFunction)(void* seq,void* node) )

#define _CvConvState CvFilterState

typedef struct CvPyramid
{
    uchar **ptr;
    CvSize *sz;
    double *rate;
    int *step;
    uchar *state;
    int level;
}
CvPyramid;

#include "_cvipp.h"
#include "_cvmatrix.h"
#include "_cvgeom.h"
#include "_cvimgproc.h"

// default face cascade
//extern const char* icvDefaultFaceCascade[];

#endif /*_CV_INTERNAL_H_*/
