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

#include "_cv.h"

static void
icvAdaptiveThreshold_MeanC( const CvMat* src, CvMat* dst, int method,
                            int maxValue, int type, int size, double delta )
{
    CvMat* mean = 0;
    CV_FUNCNAME( "icvAdaptiveThreshold_MeanC" );

    __BEGIN__;

    int i, j, rows, cols;
    int idelta = type == CV_THRESH_BINARY ? cvCeil(delta) : cvFloor(delta);
    uchar tab[768];

    if( size <= 1 || (size&1) == 0 )
        CV_ERROR( CV_StsOutOfRange, "Neighborhood size must be >=3 and odd (3, 5, 7, ...)" );

    if( maxValue < 0 )
    {
        CV_CALL( cvSetZero( dst ));
        EXIT;
    }

    rows = src->rows;
    cols = src->cols;

    if( src->data.ptr != dst->data.ptr )
        mean = dst;
    else
        CV_CALL( mean = cvCreateMat( rows, cols, CV_8UC1 ));

    CV_CALL( cvSmooth( src, mean, method == CV_ADAPTIVE_THRESH_MEAN_C ?
                       CV_BLUR : CV_GAUSSIAN, size, size ));
    if( maxValue > 255 )
        maxValue = 255;

    if( type == CV_THRESH_BINARY )
        for( i = 0; i < 768; i++ )
            tab[i] = (uchar)(i - 255 > -idelta ? maxValue : 0);
    else
        for( i = 0; i < 768; i++ )
            tab[i] = (uchar)(i - 255 <= -idelta ? maxValue : 0);

    for( i = 0; i < rows; i++ )
    {
        const uchar* s = src->data.ptr + i*src->step;
        const uchar* m = mean->data.ptr + i*mean->step;
        uchar* d = dst->data.ptr + i*dst->step;

        for( j = 0; j < cols; j++ )
            d[j] = tab[s[j] - m[j] + 255];
    }

    __END__;

    if( mean != dst )
        cvReleaseMat( &mean );
}


CV_IMPL void
cvAdaptiveThreshold( const void *srcIm, void *dstIm, double maxValue,
                     int method, int type, int blockSize, double param1 )
{
    CvMat src_stub, dst_stub;
    CvMat *src = 0, *dst = 0;

    CV_FUNCNAME( "cvAdaptiveThreshold" );

    __BEGIN__;

    if( type != CV_THRESH_BINARY && type != CV_THRESH_BINARY_INV )
        CV_ERROR( CV_StsBadArg, "Only CV_TRESH_BINARY and CV_THRESH_BINARY_INV "
                                "threshold types are acceptable" );

    CV_CALL( src = cvGetMat( srcIm, &src_stub ));
    CV_CALL( dst = cvGetMat( dstIm, &dst_stub ));

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_TYPE(dst->type) != CV_8UC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    switch( method )
    {
    case CV_ADAPTIVE_THRESH_MEAN_C:
    case CV_ADAPTIVE_THRESH_GAUSSIAN_C:
        CV_CALL( icvAdaptiveThreshold_MeanC( src, dst, method, cvRound(maxValue),type,
                                             blockSize, param1 ));
        break;
    default:
        CV_ERROR( CV_BADCOEF_ERR, "" );
    }

    __END__;
}

/* End of file. */
