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

static CvStatus CV_STDCALL
icvThresh_8u_C1R( const uchar* src, int src_step, uchar* dst, int dst_step,
                  CvSize roi, uchar thresh, uchar maxval, int type )
{
    int i, j;
    uchar tab[256];

    switch( type )
    {
    case CV_THRESH_BINARY:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = maxval;
        break;
    case CV_THRESH_BINARY_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = maxval;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    case CV_THRESH_TRUNC:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = thresh;
        break;
    case CV_THRESH_TOZERO:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = (uchar)i;
        break;
    case CV_THRESH_TOZERO_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    default:
        return CV_BADFLAG_ERR;
    }

    for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
    {
        for( j = 0; j <= roi.width - 4; j += 4 )
        {
            uchar t0 = tab[src[j]];
            uchar t1 = tab[src[j+1]];

            dst[j] = t0;
            dst[j+1] = t1;

            t0 = tab[src[j+2]];
            t1 = tab[src[j+3]];

            dst[j+2] = t0;
            dst[j+3] = t1;
        }

        for( ; j < roi.width; j++ )
            dst[j] = tab[src[j]];
    }

    return CV_NO_ERR;
}


static CvStatus CV_STDCALL
icvThresh_32f_C1R( const float *src, int src_step, float *dst, int dst_step,
                   CvSize roi, float thresh, float maxval, int type )
{
    int i, j;
    const int* isrc = (const int*)src;
    int* idst = (int*)dst;
    Cv32suf v;
    int iThresh, iMax;

    v.f = thresh; iThresh = CV_TOGGLE_FLT(v.i);
    v.f = maxval; iMax = v.i;

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    switch( type )
    {
    case CV_THRESH_BINARY:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT(temp) <= iThresh) - 1) & iMax;
            }
        }
        break;

    case CV_THRESH_BINARY_INV:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT(temp) > iThresh) - 1) & iMax;
            }
        }
        break;

    case CV_THRESH_TRUNC:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                float temp = src[j];

                if( temp > thresh )
                    temp = thresh;
                dst[j] = temp;
            }
        }
        break;

    case CV_THRESH_TOZERO:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT( temp ) <= iThresh) - 1) & temp;
            }
        }
        break;

    case CV_THRESH_TOZERO_INV:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT( temp ) > iThresh) - 1) & temp;
            }
        }
        break;

    default:
        return CV_BADFLAG_ERR;
    }

    return CV_OK;
}


static double
icvGetThreshVal_Otsu( const CvHistogram* hist )
{
    double max_val = 0;
    
    CV_FUNCNAME( "icvGetThreshVal_Otsu" );

    __BEGIN__;

    int i, count;
    const float* h;
    double sum = 0, mu = 0;
    bool uniform = false;
    double low = 0, high = 0, delta = 0;
    float* nu_thresh = 0;
    double mu1 = 0, q1 = 0;
    double max_sigma = 0;

    if( !CV_IS_HIST(hist) || CV_IS_SPARSE_HIST(hist) || hist->mat.dims != 1 )
        CV_ERROR( CV_StsBadArg,
        "The histogram in Otsu method must be a valid dense 1D histogram" );

    count = hist->mat.dim[0].size;
    h = (float*)cvPtr1D( hist->bins, 0 );

    if( !CV_HIST_HAS_RANGES(hist) || CV_IS_UNIFORM_HIST(hist) )
    {
        if( CV_HIST_HAS_RANGES(hist) )
        {
            low = hist->thresh[0][0];
            high = hist->thresh[0][1];
        }
        else
        {
            low = 0;
            high = count;
        }

        delta = (high-low)/count;
        low += delta*0.5;
        uniform = true;
    }
    else
        nu_thresh = hist->thresh2[0];

    for( i = 0; i < count; i++ )
    {
        sum += h[i];
        if( uniform )
            mu += (i*delta + low)*h[i];
        else
            mu += (nu_thresh[i*2] + nu_thresh[i*2+1])*0.5*h[i];
    }
    
    sum = fabs(sum) > FLT_EPSILON ? 1./sum : 0;
    mu *= sum;

    mu1 = 0;
    q1 = 0;

    for( i = 0; i < count; i++ )
    {
        double p_i, q2, mu2, val_i, sigma;

        p_i = h[i]*sum;
        mu1 *= q1;
        q1 += p_i;
        q2 = 1. - q1;

        if( MIN(q1,q2) < FLT_EPSILON || MAX(q1,q2) > 1. - FLT_EPSILON )
            continue;

        if( uniform )
            val_i = i*delta + low;
        else
            val_i = (nu_thresh[i*2] + nu_thresh[i*2+1])*0.5;

        mu1 = (mu1 + val_i*p_i)/q1;
        mu2 = (mu - q1*mu1)/q2;
        sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
        if( sigma > max_sigma )
        {
            max_sigma = sigma;
            max_val = val_i;
        }
    }

    __END__;

    return max_val;
}


icvAndC_8u_C1R_t icvAndC_8u_C1R_p = 0;
icvCompareC_8u_C1R_cv_t icvCompareC_8u_C1R_cv_p = 0;
icvThreshold_GTVal_8u_C1R_t icvThreshold_GTVal_8u_C1R_p = 0;
icvThreshold_GTVal_32f_C1R_t icvThreshold_GTVal_32f_C1R_p = 0;
icvThreshold_LTVal_8u_C1R_t icvThreshold_LTVal_8u_C1R_p = 0;
icvThreshold_LTVal_32f_C1R_t icvThreshold_LTVal_32f_C1R_p = 0;

CV_IMPL double
cvThreshold( const void* srcarr, void* dstarr, double thresh, double maxval, int type )
{
    CvHistogram* hist = 0;
    
    CV_FUNCNAME( "cvThreshold" );

    __BEGIN__;

    CvSize roi;
    int src_step, dst_step;
    CvMat src_stub, *src = (CvMat*)srcarr;
    CvMat dst_stub, *dst = (CvMat*)dstarr;
    CvMat src0, dst0;
    int coi1 = 0, coi2 = 0;
    int ithresh, imaxval, cn;
    bool use_otsu;

    CV_CALL( src = cvGetMat( src, &src_stub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dst_stub, &coi2 ));

    if( coi1 + coi2 )
        CV_ERROR( CV_BadCOI, "COI is not supported by the function" );

    if( !CV_ARE_CNS_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedFormats, "Both arrays must have equal number of channels" );

    cn = CV_MAT_CN(src->type);
    if( cn > 1 )
    {
        src = cvReshape( src, &src0, 1 );
        dst = cvReshape( dst, &dst0, 1 );
    }

    use_otsu = (type & ~CV_THRESH_MASK) == CV_THRESH_OTSU;
    type &= CV_THRESH_MASK;

    if( use_otsu )
    {
        float _ranges[] = { 0, 256 };
        float* ranges = _ranges;
        int hist_size = 256;
        void* srcarr0 = src;

        if( CV_MAT_TYPE(src->type) != CV_8UC1 )
            CV_ERROR( CV_StsNotImplemented, "Otsu method can only be used with 8uC1 images" );

        CV_CALL( hist = cvCreateHist( 1, &hist_size, CV_HIST_ARRAY, &ranges ));
        cvCalcArrHist( &srcarr0, hist );
        thresh = cvFloor(icvGetThreshVal_Otsu( hist ));
    }

    if( !CV_ARE_DEPTHS_EQ( src, dst ) )
    {
        if( CV_MAT_TYPE(dst->type) != CV_8UC1 )
            CV_ERROR( CV_StsUnsupportedFormat, "In case of different types destination should be 8uC1" );

        if( type != CV_THRESH_BINARY && type != CV_THRESH_BINARY_INV )
            CV_ERROR( CV_StsBadArg,
            "In case of different types only CV_THRESH_BINARY "
            "and CV_THRESH_BINARY_INV thresholding types are supported" );

        if( maxval < 0 )
        {
            CV_CALL( cvSetZero( dst ));
        }
        else
        {
            CV_CALL( cvCmpS( src, thresh, dst, type == CV_THRESH_BINARY ? CV_CMP_GT : CV_CMP_LE ));
            if( maxval < 255 )
                CV_CALL( cvAndS( dst, cvScalarAll( maxval ), dst ));
        }
        EXIT;
    }

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    roi = cvGetMatSize( src );
    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        roi.width *= roi.height;
        roi.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }
    else
    {
        src_step = src->step;
        dst_step = dst->step;
    }

    switch( CV_MAT_DEPTH(src->type) )
    {
    case CV_8U:
        
        ithresh = cvFloor(thresh);
        imaxval = cvRound(maxval);
        if( type == CV_THRESH_TRUNC )
            imaxval = ithresh;
        imaxval = CV_CAST_8U(imaxval);

        if( ithresh < 0 || ithresh >= 255 )
        {
            if( type == CV_THRESH_BINARY || type == CV_THRESH_BINARY_INV ||
                ((type == CV_THRESH_TRUNC || type == CV_THRESH_TOZERO_INV) && ithresh < 0) ||
                (type == CV_THRESH_TOZERO && ithresh >= 255) )
            {
                int v = type == CV_THRESH_BINARY ? (ithresh >= 255 ? 0 : imaxval) :
                        type == CV_THRESH_BINARY_INV ? (ithresh >= 255 ? imaxval : 0) :
                        type == CV_THRESH_TRUNC ? imaxval : 0;

                cvSet( dst, cvScalarAll(v) );
                EXIT;
            }
            else
            {
                cvCopy( src, dst );
                EXIT;
            }
        }

        if( type == CV_THRESH_BINARY || type == CV_THRESH_BINARY_INV )
        {
            if( icvCompareC_8u_C1R_cv_p && icvAndC_8u_C1R_p )
            {
                IPPI_CALL( icvCompareC_8u_C1R_cv_p( src->data.ptr, src_step,
                    (uchar)ithresh, dst->data.ptr, dst_step, roi,
                    type == CV_THRESH_BINARY ? cvCmpGreater : cvCmpLessEq ));

                if( imaxval < 255 )
                    IPPI_CALL( icvAndC_8u_C1R_p( dst->data.ptr, dst_step,
                    (uchar)imaxval, dst->data.ptr, dst_step, roi ));
                EXIT;
            }
        }
        else if( type == CV_THRESH_TRUNC || type == CV_THRESH_TOZERO_INV )
        {
            if( icvThreshold_GTVal_8u_C1R_p )
            {
                IPPI_CALL( icvThreshold_GTVal_8u_C1R_p( src->data.ptr, src_step,
                    dst->data.ptr, dst_step, roi, (uchar)ithresh,
                    (uchar)(type == CV_THRESH_TRUNC ? ithresh : 0) ));
                EXIT;
            }
        }
        else
        {
            assert( type == CV_THRESH_TOZERO );
            if( icvThreshold_LTVal_8u_C1R_p )
            {
                ithresh = cvFloor(thresh+1.);
                ithresh = CV_CAST_8U(ithresh);
                IPPI_CALL( icvThreshold_LTVal_8u_C1R_p( src->data.ptr, src_step,
                    dst->data.ptr, dst_step, roi, (uchar)ithresh, 0 ));
                EXIT;
            }
        }

        icvThresh_8u_C1R( src->data.ptr, src_step,
                          dst->data.ptr, dst_step, roi,
                          (uchar)ithresh, (uchar)imaxval, type );
        break;
    case CV_32F:

        if( type == CV_THRESH_TRUNC || type == CV_THRESH_TOZERO_INV )
        {
            if( icvThreshold_GTVal_32f_C1R_p )
            {
                IPPI_CALL( icvThreshold_GTVal_32f_C1R_p( src->data.fl, src_step,
                    dst->data.fl, dst_step, roi, (float)thresh,
                    type == CV_THRESH_TRUNC ? (float)thresh : 0 ));
                EXIT;
            }
        }
        else if( type == CV_THRESH_TOZERO )
        {
            if( icvThreshold_LTVal_32f_C1R_p )
            {
                IPPI_CALL( icvThreshold_LTVal_32f_C1R_p( src->data.fl, src_step,
                    dst->data.fl, dst_step, roi, (float)(thresh*(1 + FLT_EPSILON)), 0 ));
                EXIT;
            }
        }

        icvThresh_32f_C1R( src->data.fl, src_step, dst->data.fl, dst_step, roi,
                           (float)thresh, (float)maxval, type );
        break;
    default:
        CV_ERROR( CV_BadDepth, cvUnsupportedFormat );
    }

    __END__;

    if( hist )
        cvReleaseHist( &hist );

    return thresh;
}

/* End of file. */
