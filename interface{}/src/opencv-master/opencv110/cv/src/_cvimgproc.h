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

#ifndef _CV_IMG_PROC_H_
#define _CV_IMG_PROC_H_

#define  CV_COPY( dst, src, len, idx ) \
    for( (idx) = 0; (idx) < (len); (idx)++) (dst)[idx] = (src)[idx]

#define  CV_SET( dst, val, len, idx )  \
    for( (idx) = 0; (idx) < (len); (idx)++) (dst)[idx] = (val)

/* performs convolution of 2d floating-point array with 3x1, 1x3 or separable 3x3 mask */
void icvSepConvSmall3_32f( float* src, int src_step, float* dst, int dst_step,
            CvSize src_size, const float* kx, const float* ky, float* buffer );

typedef CvStatus (CV_STDCALL * CvSobelFixedIPPFunc)
( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int aperture );

typedef CvStatus (CV_STDCALL * CvFilterFixedIPPFunc)
( const void* src, int srcstep, void* dst, int dststep, CvSize roi );

#undef   CV_CALC_MIN
#define  CV_CALC_MIN(a, b) if((a) > (b)) (a) = (b)

#undef   CV_CALC_MAX
#define  CV_CALC_MAX(a, b) if((a) < (b)) (a) = (b)

#define CV_MORPH_ALIGN  4

#define CV_WHOLE   0
#define CV_START   1
#define CV_END     2
#define CV_MIDDLE  4

void
icvCrossCorr( const CvArr* _img, const CvArr* _templ,
              CvArr* _corr, CvPoint anchor=cvPoint(0,0) );

CvStatus CV_STDCALL
icvCopyReplicateBorder_8u( const uchar* src, int srcstep, CvSize srcroi,
                           uchar* dst, int dststep, CvSize dstroi,
                           int left, int right, int cn, const uchar* value = 0 );

CvMat* icvIPPFilterInit( const CvMat* src, int stripe_size, CvSize ksize );

int icvIPPFilterNextStripe( const CvMat* src, CvMat* temp, int y,
                            CvSize ksize, CvPoint anchor );

int icvIPPSepFilter( const CvMat* src, CvMat* dst, const CvMat* kernelX,
                     const CvMat* kernelY, CvPoint anchor );

#define ICV_WARP_SHIFT          10
#define ICV_WARP_MASK           ((1 << ICV_WARP_SHIFT) - 1)

#define ICV_LINEAR_TAB_SIZE     (ICV_WARP_MASK+1)
extern float icvLinearCoeffs[(ICV_LINEAR_TAB_SIZE+1)*2];
void icvInitLinearCoeffTab();

#define ICV_CUBIC_TAB_SIZE   (ICV_WARP_MASK+1)
extern float icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE+1)*2];

void icvInitCubicCoeffTab();

CvStatus CV_STDCALL icvGetRectSubPix_8u_C1R
( const uchar* src, int src_step, CvSize src_size,
  uchar* dst, int dst_step, CvSize win_size, CvPoint2D32f center );
CvStatus CV_STDCALL icvGetRectSubPix_8u32f_C1R
( const uchar* src, int src_step, CvSize src_size,
  float* dst, int dst_step, CvSize win_size, CvPoint2D32f center );
CvStatus CV_STDCALL icvGetRectSubPix_32f_C1R
( const float* src, int src_step, CvSize src_size,
  float* dst, int dst_step, CvSize win_size, CvPoint2D32f center );

CvStatus CV_STDCALL icvGetQuadrangleSubPix_8u_C1R
( const uchar* src, int src_step, CvSize src_size,
  uchar* dst, int dst_step, CvSize win_size, const float *matrix );
CvStatus CV_STDCALL icvGetQuadrangleSubPix_8u32f_C1R
( const uchar* src, int src_step, CvSize src_size,
  float* dst, int dst_step, CvSize win_size, const float *matrix );
CvStatus CV_STDCALL icvGetQuadrangleSubPix_32f_C1R
( const float* src, int src_step, CvSize src_size,
  float* dst, int dst_step, CvSize win_size, const float *matrix );

#endif /*_CV_INTERNAL_H_*/
