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

#if 0

IPCVAPI(CvStatus, icvCalcContrastHist8uC1R, ( uchar** img, int step, CvSize size,
                                              CvHistogram* hist, int dont_clear ))

IPCVAPI(CvStatus, icvCalcContrastHistMask8uC1R, ( uchar** img, int step, 
                                                  uchar*  mask, int mask_step, CvSize size,
                                                  CvHistogram* hist, int dont_clear ))

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:       icvCalcContrastHist8uC1R
//    Purpose:    Calculating the histogram of contrast from one-channel images
//    Context:
//    Parameters:
//    Returns:
//    Notes:      if dont_clear parameter is NULL then histogram clearing before
//                calculating (all values sets to NULL)
//F*/
static CvStatus CV_STDCALL
icvCalcContrastHist8uC1R( uchar** img, int step, CvSize size,
                          CvHistogram* hist, int dont_clear )
{
    int i, j, t, x = 0, y = 0;
    int dims;

    if( !hist || !img )
        return CV_NULLPTR_ERR;

    dims = hist->c_dims;
    if( dims != 1 )
        return CV_BADSIZE_ERR;

    if( hist->type != CV_HIST_ARRAY )
        return CV_BADFLAG_ERR;

    for( i = 0; i < dims; i++ )
        if( !img[i] )
            return CV_NULLPTR_ERR;

    for( i = 0; i < hist->c_dims; i++ )
    {
        if( !hist->thresh[i] )
            return CV_NULLPTR_ERR;
        assert( hist->chdims[i] );
    }

    j = hist->dims[0] * hist->mdims[0];

    int *n = (int *)cvAlloc( (size_t)hist->dims[0] * sizeof( int ));

    if( hist->type == CV_HIST_ARRAY )
    {
        if( !dont_clear )
            for( i = 0; i < j; i++ )
            {
                hist->array[i] = 0;
                n[i] = 0;
            }

        switch (hist->c_dims)
        {
        case 1:
            {
                uchar *data0 = img[0];
                int *array = (int *) hist->array;
                int *chdims = hist->chdims[0];

                for( i = 0; i < j; i++ )
                    array[i] = cvRound( hist->array[i] );

                for( y = 0; y < size.height; y++, data0 += step )
                {
                    for( x = 0; x <= size.width - 1; x += 2 )
                    {
                        int v1_r = MIN( data0[x], data0[x + 1] );
                        int v2_r = MAX( data0[x], data0[x + 1] );

//    calculate contrast for the right-left pair 
                        for( t = v1_r; t < v2_r; t++ )
                        {
                            int val0 = chdims[t + 128];

                            array[val0] += MIN( t - v1_r, v2_r - t );
                            n[val0]++;
                        }

                        if( y < size.height - 1 )
                        {
                            int v1_d = MIN( data0[x], data0[x + step] );
                            int v2_d = MAX( data0[x], data0[x + step] );

//    calculate contrast for the top-down pair 
                            for( t = v1_d; t < v2_d; t++ )
                            {
                                int val0 = chdims[t + 128];

                                array[val0] += MIN( t - v1_d, v2_d - t );
                                n[val0]++;
                            }
                        }
                    }
                }

//  convert int to float 
                for( i = 0; i < j; i++ )
                {
                    if( n[i] != 0 )
                        hist->array[i] = (float) array[i] / n[i];
                    else
                        hist->array[i] = 0;
                }
            }
            break;
        default:
            return CV_BADSIZE_ERR;
        }
    }

    cvFree( &n );
    return CV_NO_ERR;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:       icvCalcContrastHistMask8uC1R
//    Purpose:    Calculating the mask histogram of contrast from one-channel images
//    Context:
//    Parameters:
//    Returns:
//    Notes:      if dont_clear parameter is NULL then histogram clearing before
//                calculating (all values sets to NULL)
//F*/
static CvStatus CV_STDCALL
icvCalcContrastHistMask8uC1R( uchar** img, int step, uchar* mask, int mask_step,
                              CvSize size, CvHistogram * hist, int dont_clear )
{
    int i, j, t, x = 0, y = 0;
    int dims;


    if( !hist || !img || !mask )
        return CV_NULLPTR_ERR;

    dims = hist->c_dims;
    if( dims != 1 )
        return CV_BADSIZE_ERR;

    if( hist->type != CV_HIST_ARRAY )
        return CV_BADFLAG_ERR;

    for( i = 0; i < dims; i++ )
        if( !img[i] )
            return CV_NULLPTR_ERR;

    for( i = 0; i < hist->c_dims; i++ )
    {
        if( !hist->thresh[i] )
            return CV_NULLPTR_ERR;
        assert( hist->chdims[i] );
    }

    j = hist->dims[0] * hist->mdims[0];

    int *n = (int *)cvAlloc( (size_t) hist->dims[0] * sizeof( int ));

    if( hist->type == CV_HIST_ARRAY )
    {
        if( !dont_clear )
            for( i = 0; i < j; i++ )
            {
                hist->array[i] = 0;
                n[i] = 0;
            }

        switch (hist->c_dims)
        {
        case 1:
            {
                uchar *data0 = img[0];
                uchar *maskp = mask;
                int *array = (int *) hist->array;
                int *chdims = hist->chdims[0];

                for( i = 0; i < j; i++ )
                    array[i] = cvRound( hist->array[i] );

                for( y = 0; y < size.height; y++, data0 += step, maskp += mask_step )
                {
                    for( x = 0; x <= size.width - 2; x++ )
                    {
                        if( maskp[x] )
                        {
                            if( maskp[x + 1] )
                            {
                                int v1_r = MIN( data0[x], data0[x + 1] );
                                int v2_r = MAX( data0[x], data0[x + 1] );


                                //    calculate contrast for the right-left pair 
                                for( t = v1_r; t < v2_r; t++ )
                                {
                                    int val0 = chdims[t + 128];

                                    array[val0] += MIN( t - v1_r, v2_r - t );
                                    n[val0]++;

                                }
                            }

                            if( y < size.height - 1 )
                            {
                                if( maskp[x + mask_step] )
                                {
                                    int v1_d = MIN( data0[x], data0[x + step] );
                                    int v2_d = MAX( data0[x], data0[x + step] );

                                    //    calculate contrast for the top-down pair 
                                    for( t = v1_d; t < v2_d; t++ )
                                    {
                                        int val0 = chdims[t + 128];

                                        array[val0] += MIN( t - v1_d, v2_d - t );
                                        n[val0]++;

                                    }
                                }
                            }
                        }
                    }
                }

//  convert int to float 
                for( i = 0; i < j; i++ )
                {
                    if( n[i] != 0 )
                        hist->array[i] = (float) array[i] / n[i];
                    else
                        hist->array[i] = 0;
                }
            }
            break;
        default:
            return CV_BADSIZE_ERR;
        }
    }

    cvFree( &n );
    return CV_NO_ERR;
}

/*
CV_IMPL void cvCalcContrastHist( IplImage** img, CvHistogram* hist, int dont_clear )
{
    CV_FUNCNAME( "cvCalcContrastHist" );
    uchar*   data[CV_HIST_MAX_DIM];
    int      step = 0;
    CvSize roi = {0,0};

    __BEGIN__;

    {for( int i = 0; i < hist->c_dims; i++ )
        CV_CALL( CV_CHECK_IMAGE( img[i] ) );}

    {for( int i = 0; i < hist->c_dims; i++ )
        cvGetImageRawData( img[i], &data[i], &step, &roi );}

    if(img[0]->nChannels != 1) 
        CV_ERROR( IPL_BadNumChannels, "bad channels numbers" );

    if(img[0]->depth != IPL_DEPTH_8U) 
        CV_ERROR( IPL_BadDepth, "bad image depth" );

    switch(img[0]->depth)
    {
    case IPL_DEPTH_8U:
        IPPI_CALL( icvCalcContrastHist8uC1R( data, step, roi, hist, dont_clear ) );
        break;
    default:  CV_ERROR( IPL_BadDepth, "bad image depth" );
    }

    __CLEANUP__;
    __END__;
}
*/

CV_IMPL void
cvCalcContrastHist( IplImage ** img, CvHistogram * hist, int dont_clear, IplImage * mask )
{
    CV_FUNCNAME( "cvCalcContrastHist" );
    uchar *data[CV_HIST_MAX_DIM];
    uchar *mask_data = 0;
    int step = 0;
    int mask_step = 0;
    CvSize roi = { 0, 0 };

    __BEGIN__;

    {
        for( int i = 0; i < hist->c_dims; i++ )
            CV_CALL( CV_CHECK_IMAGE( img[i] ));
    }
    if( mask )
    {
        CV_CALL( CV_CHECK_IMAGE( mask ));
        if( mask->depth != IPL_DEPTH_8U )
            CV_ERROR( CV_BadDepth, "bad mask depth" );
        cvGetImageRawData( mask, &mask_data, &mask_step, 0 );
    }


    {
        for( int i = 0; i < hist->c_dims; i++ )
            cvGetImageRawData( img[i], &data[i], &step, &roi );
    }

    if( img[0]->nChannels != 1 )
        CV_ERROR( CV_BadNumChannels, "bad channels numbers" );

    if( img[0]->depth != IPL_DEPTH_8U )
        CV_ERROR( CV_BadDepth, "bad image depth" );


    switch (img[0]->depth)
    {
    case IPL_DEPTH_8U:
        if( !mask )
        {
            IPPI_CALL( icvCalcContrastHist8uC1R( data, step, roi, hist, dont_clear ));
        }
        else
        {
            IPPI_CALL( icvCalcContrastHistMask8uC1R( data, step, mask_data,
                                                     mask_step, roi, hist, dont_clear ));
        }
        break;
    default:
        CV_ERROR( CV_BadDepth, "bad image depth" );
    }

    __CLEANUP__;
    __END__;
}

#endif
