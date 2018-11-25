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

#define ICV_DIST_SHIFT  16
#define ICV_INIT_DIST0  (INT_MAX >> 2)

static CvStatus
icvInitTopBottom( int* temp, int tempstep, CvSize size, int border )
{
    int i, j;
    for( i = 0; i < border; i++ )
    {
        int* ttop = (int*)(temp + i*tempstep);
        int* tbottom = (int*)(temp + (size.height + border*2 - i - 1)*tempstep);
        
        for( j = 0; j < size.width + border*2; j++ )
        {
            ttop[j] = ICV_INIT_DIST0;
            tbottom[j] = ICV_INIT_DIST0;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvDistanceTransform_3x3_C1R( const uchar* src, int srcstep, int* temp,
        int step, float* dist, int dststep, CvSize size, const float* metrics )
{
    const int BORDER = 1;
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], ICV_DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], ICV_DIST_SHIFT );
    const float scale = 1.f/(1 << ICV_DIST_SHIFT);

    srcstep /= sizeof(src[0]);
    step /= sizeof(temp[0]);
    dststep /= sizeof(dist[0]);

    icvInitTopBottom( temp, step, size, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = ICV_INIT_DIST0;
        
        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
                tmp[j] = 0;
            else
            {
                int t0 = tmp[j-step-1] + DIAG_DIST;
                int t = tmp[j-step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        
        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
            d[j] = (float)(t0 * scale);
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvDistanceTransform_5x5_C1R( const uchar* src, int srcstep, int* temp,
        int step, float* dist, int dststep, CvSize size, const float* metrics )
{
    const int BORDER = 2;
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], ICV_DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], ICV_DIST_SHIFT );
    const int LONG_DIST = CV_FLT_TO_FIX( metrics[2], ICV_DIST_SHIFT );
    const float scale = 1.f/(1 << ICV_DIST_SHIFT);

    srcstep /= sizeof(src[0]);
    step /= sizeof(temp[0]);
    dststep /= sizeof(dist[0]);

    icvInitTopBottom( temp, step, size, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = ICV_INIT_DIST0;
        
        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
                tmp[j] = 0;
            else
            {
                int t0 = tmp[j-step*2-1] + LONG_DIST;
                int t = tmp[j-step*2+1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step-2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        
        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step*2+1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step*2-1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step+2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
            d[j] = (float)(t0 * scale);
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvDistanceTransformEx_5x5_C1R( const uchar* src, int srcstep, int* temp,
                int step, float* dist, int dststep, int* labels, int lstep,
                CvSize size, const float* metrics )
{
    const int BORDER = 2;
    
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], ICV_DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], ICV_DIST_SHIFT );
    const int LONG_DIST = CV_FLT_TO_FIX( metrics[2], ICV_DIST_SHIFT );
    const float scale = 1.f/(1 << ICV_DIST_SHIFT);

    srcstep /= sizeof(src[0]);
    step /= sizeof(temp[0]);
    dststep /= sizeof(dist[0]);
    lstep /= sizeof(labels[0]);

    icvInitTopBottom( temp, step, size, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        int* lls = (int*)(labels + i*lstep);

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = ICV_INIT_DIST0;
        
        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
            {
                tmp[j] = 0;
                //assert( lls[j] != 0 );
            }
            else
            {
                int t0 = ICV_INIT_DIST0, t;
                int l0 = 0;

                t = tmp[j-step*2-1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep*2-1];
                }
                t = tmp[j-step*2+1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep*2+1];
                }
                t = tmp[j-step-2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep-2];
                }
                t = tmp[j-step-1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep-1];
                }
                t = tmp[j-step] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep];
                }
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep+1];
                }
                t = tmp[j-step+2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep+2];
                }
                t = tmp[j-1] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-1];
                }

                tmp[j] = t0;
                lls[j] = l0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        int* lls = (int*)(labels + i*lstep);
        
        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            int l0 = lls[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step*2+1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep*2+1];
                }
                t = tmp[j+step*2-1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep*2-1];
                }
                t = tmp[j+step+2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep+2];
                }
                t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep+1];
                }
                t = tmp[j+step] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep];
                }
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep-1];
                }
                t = tmp[j+step-2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep-2];
                }
                t = tmp[j+1] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+1];
                }
                tmp[j] = t0;
                lls[j] = l0;
            }
            d[j] = (float)(t0 * scale);
        }
    }

    return CV_OK;
}


static CvStatus
icvGetDistanceTransformMask( int maskType, float *metrics )
{
    if( !metrics )
        return CV_NULLPTR_ERR;

    switch (maskType)
    {
    case 30:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        break;

    case 31:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        break;

    case 32:
        metrics[0] = 0.955f;
        metrics[1] = 1.3693f;
        break;

    case 50:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        metrics[2] = 2.0f;
        break;

    case 51:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        metrics[2] = 3.0f;
        break;

    case 52:
        metrics[0] = 1.0f;
        metrics[1] = 1.4f;
        metrics[2] = 2.1969f;
        break;
    default:
        return CV_BADRANGE_ERR;
    }

    return CV_OK;
}


static void
icvTrueDistTrans( const CvMat* src, CvMat* dst )
{
    CvMat* buffer = 0;

    CV_FUNCNAME( "cvDistTransform2" );

    __BEGIN__;

    int i, m, n;
    int sstep, dstep;
    const float inf = 1e6f;
    int thread_count = cvGetNumThreads();
    int pass1_sz, pass2_sz;

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( CV_MAT_TYPE(src->type) != CV_8UC1 ||
        CV_MAT_TYPE(dst->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "The input image must have 8uC1 type and the output one must have 32fC1 type" );

    m = src->rows;
    n = src->cols;

    // (see stage 1 below):
    // sqr_tab: 2*m, sat_tab: 3*m + 1, d: m*thread_count,
    pass1_sz = src->rows*(5 + thread_count) + 1;
    // (see stage 2):
    // sqr_tab & inv_tab: n each; f & v: n*thread_count each; z: (n+1)*thread_count
    pass2_sz = src->cols*(2 + thread_count*3) + thread_count;
    CV_CALL( buffer = cvCreateMat( 1, MAX(pass1_sz, pass2_sz), CV_32FC1 ));

    sstep = src->step;
    dstep = dst->step / sizeof(float);

    // stage 1: compute 1d distance transform of each column
    {
    float* sqr_tab = buffer->data.fl;
    int* sat_tab = (int*)(sqr_tab + m*2);
    const int shift = m*2;

    for( i = 0; i < m; i++ )
        sqr_tab[i] = (float)(i*i);
    for( i = m; i < m*2; i++ )
        sqr_tab[i] = inf;
    for( i = 0; i < shift; i++ )
        sat_tab[i] = 0;
    for( ; i <= m*3; i++ )
        sat_tab[i] = i - shift;

#ifdef _OPENMP
    #pragma omp parallel for num_threads(thread_count)
#endif
    for( i = 0; i < n; i++ )
    {
        const uchar* sptr = src->data.ptr + i + (m-1)*sstep;
        float* dptr = dst->data.fl + i;
        int* d = (int*)(sat_tab + m*3+1+m*cvGetThreadNum());
        int j, dist = m-1;

        for( j = m-1; j >= 0; j--, sptr -= sstep )
        {
            dist = (dist + 1) & (sptr[0] == 0 ? 0 : -1);
            d[j] = dist;
        }

        dist = m-1;
        for( j = 0; j < m; j++, dptr += dstep )
        {
            dist = dist + 1 - sat_tab[dist + 1 - d[j] + shift];
            d[j] = dist;
            dptr[0] = sqr_tab[dist];
        }
    }
    }

    // stage 2: compute modified distance transform for each row
    {
    float* inv_tab = buffer->data.fl;
    float* sqr_tab = inv_tab + n;

    inv_tab[0] = sqr_tab[0] = 0.f;
    for( i = 1; i < n; i++ )
    {
        inv_tab[i] = (float)(0.5/i);
        sqr_tab[i] = (float)(i*i);
    }

#ifdef _OPENMP
    #pragma omp parallel for num_threads(thread_count) schedule(dynamic)
#endif
    for( i = 0; i < m; i++ )
    {
        float* d = (float*)(dst->data.ptr + i*dst->step);
        float* f = sqr_tab + n + (n*3+1)*cvGetThreadNum();
        float* z = f + n;
        int* v = (int*)(z + n + 1);
        int p, q, k;

        v[0] = 0;
        z[0] = -inf;
        z[1] = inf;
        f[0] = d[0];

        for( q = 1, k = 0; q < n; q++ )
        {
            float fq = d[q];
            f[q] = fq;

            for(;;k--)
            {
                p = v[k];
                float s = (fq + sqr_tab[q] - d[p] - sqr_tab[p])*inv_tab[q - p];
                if( s > z[k] )
                {
                    k++;
                    v[k] = q;
                    z[k] = s;
                    z[k+1] = inf;
                    break;
                }
            }
        }

        for( q = 0, k = 0; q < n; q++ )
        {
            while( z[k+1] < q )
                k++;
            p = v[k];
            d[q] = sqr_tab[abs(q - p)] + f[p];
        }
    }
    }

    cvPow( dst, dst, 0.5 );

    __END__;

    cvReleaseMat( &buffer );
}


/*********************************** IPP functions *********************************/

icvDistanceTransform_3x3_8u32f_C1R_t icvDistanceTransform_3x3_8u32f_C1R_p = 0;
icvDistanceTransform_5x5_8u32f_C1R_t icvDistanceTransform_5x5_8u32f_C1R_p = 0;
icvDistanceTransform_3x3_8u_C1IR_t icvDistanceTransform_3x3_8u_C1IR_p = 0;
icvDistanceTransform_3x3_8u_C1R_t icvDistanceTransform_3x3_8u_C1R_p = 0;

typedef CvStatus (CV_STDCALL * CvIPPDistTransFunc)( const uchar* src, int srcstep,
                                                    void* dst, int dststep,
                                                    CvSize size, const void* metrics );

typedef CvStatus (CV_STDCALL * CvIPPDistTransFunc2)( uchar* src, int srcstep,
                                                     CvSize size, const int* metrics );

/***********************************************************************************/

typedef CvStatus (CV_STDCALL * CvDistTransFunc)( const uchar* src, int srcstep,
                                                 int* temp, int tempstep,
                                                 float* dst, int dststep,
                                                 CvSize size, const float* metrics );


/****************************************************************************************\
 User-contributed code:

 Non-inplace and Inplace 8u->8u Distance Transform for CityBlock (a.k.a. L1) metric
 (C) 2006 by Jay Stavinzky.
\****************************************************************************************/

//BEGIN ATS ADDITION
/* 8-bit grayscale distance transform function */
static void
icvDistanceATS_L1_8u( const CvMat* src, CvMat* dst )
{
    CV_FUNCNAME( "cvDistanceATS" );

    __BEGIN__;

    int width = src->cols, height = src->rows;

    int a;
    uchar lut[256];
    int x, y;
    
    const uchar *sbase = src->data.ptr;
    uchar *dbase = dst->data.ptr;
    int srcstep = src->step;
    int dststep = dst->step;

    CV_ASSERT( CV_IS_MASK_ARR( src ) && CV_MAT_TYPE( dst->type ) == CV_8UC1 );
    CV_ASSERT( CV_ARE_SIZES_EQ( src, dst ));

    ////////////////////// forward scan ////////////////////////
    for( x = 0; x < 256; x++ )
        lut[x] = CV_CAST_8U(x+1);

    //init first pixel to max (we're going to be skipping it)
    dbase[0] = (uchar)(sbase[0] == 0 ? 0 : 255);

    //first row (scan west only, skip first pixel)
    for( x = 1; x < width; x++ )
        dbase[x] = (uchar)(sbase[x] == 0 ? 0 : lut[dbase[x-1]]);

    for( y = 1; y < height; y++ )
    {
        sbase += srcstep;
        dbase += dststep;

        //for left edge, scan north only
        a = sbase[0] == 0 ? 0 : lut[dbase[-dststep]];
        dbase[0] = (uchar)a;

        for( x = 1; x < width; x++ )
        {
            a = sbase[x] == 0 ? 0 : lut[MIN(a, dbase[x - dststep])];
            dbase[x] = (uchar)a;
        }
    }

    ////////////////////// backward scan ///////////////////////

    a = dbase[width-1];

    // do last row east pixel scan here (skip bottom right pixel)
    for( x = width - 2; x >= 0; x-- )
    {
        a = lut[a];
        dbase[x] = (uchar)(CV_CALC_MIN_8U(a, dbase[x]));
    }

    // right edge is the only error case
    for( y = height - 2; y >= 0; y-- )
    {
        dbase -= dststep;

        // do right edge
        a = lut[dbase[width-1+dststep]];
        dbase[width-1] = (uchar)(MIN(a, dbase[width-1]));

        for( x = width - 2; x >= 0; x-- )
        {
            int b = dbase[x+dststep];
            a = lut[MIN(a, b)];
            dbase[x] = (uchar)(MIN(a, dbase[x]));
        }
    }

    __END__;
}
//END ATS ADDITION


/* Wrapper function for distance transform group */
CV_IMPL void
cvDistTransform( const void* srcarr, void* dstarr,
                 int distType, int maskSize,
                 const float *mask,
                 void* labelsarr )
{
    CvMat* temp = 0;
    CvMat* src_copy = 0;
    CvMemStorage* st = 0;
    
    CV_FUNCNAME( "cvDistTransform" );

    __BEGIN__;

    float _mask[5] = {0};
    int _imask[3];
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat lstub, *labels = (CvMat*)labelsarr;
    CvSize size;
    CvIPPDistTransFunc ipp_func = 0;
    CvIPPDistTransFunc2 ipp_inp_func = 0;

    CV_CALL( src = cvGetMat( src, &srcstub ));
    CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( !CV_IS_MASK_ARR( src ) || (CV_MAT_TYPE( dst->type ) != CV_32FC1 &&
        (CV_MAT_TYPE(dst->type) != CV_8UC1 || distType != CV_DIST_L1 || labels)) )
        CV_ERROR( CV_StsUnsupportedFormat,
        "source image must be 8uC1 and the distance map must be 32fC1 "
        "(or 8uC1 in case of simple L1 distance transform)" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "the source and the destination images must be of the same size" );

    if( maskSize != CV_DIST_MASK_3 && maskSize != CV_DIST_MASK_5 && maskSize != CV_DIST_MASK_PRECISE )
        CV_ERROR( CV_StsBadSize, "Mask size should be 3 or 5 or 0 (presize)" );

    if( distType == CV_DIST_C || distType == CV_DIST_L1 )
        maskSize = !labels ? CV_DIST_MASK_3 : CV_DIST_MASK_5;
    else if( distType == CV_DIST_L2 && labels )
        maskSize = CV_DIST_MASK_5;

    if( maskSize == CV_DIST_MASK_PRECISE )
    {
        CV_CALL( icvTrueDistTrans( src, dst ));
        EXIT;
    }
    
    if( labels )
    {
        CV_CALL( labels = cvGetMat( labels, &lstub ));
        if( CV_MAT_TYPE( labels->type ) != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat, "the output array of labels must be 32sC1" );

        if( !CV_ARE_SIZES_EQ( labels, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "the array of labels has a different size" );

        if( maskSize == CV_DIST_MASK_3 )
            CV_ERROR( CV_StsNotImplemented,
            "3x3 mask can not be used for \"labeled\" distance transform. Use 5x5 mask" );
    }

    if( distType == CV_DIST_C || distType == CV_DIST_L1 || distType == CV_DIST_L2 )
    {
        icvGetDistanceTransformMask( (distType == CV_DIST_C ? 0 :
            distType == CV_DIST_L1 ? 1 : 2) + maskSize*10, _mask );
    }
    else if( distType == CV_DIST_USER )
    {
        if( !mask )
            CV_ERROR( CV_StsNullPtr, "" );

        memcpy( _mask, mask, (maskSize/2 + 1)*sizeof(float));
    }

    if( !labels )
    {
        if( CV_MAT_TYPE(dst->type) == CV_32FC1 )
            ipp_func = (CvIPPDistTransFunc)(maskSize == CV_DIST_MASK_3 ?
                icvDistanceTransform_3x3_8u32f_C1R_p : icvDistanceTransform_5x5_8u32f_C1R_p);
        else if( src->data.ptr != dst->data.ptr )
            ipp_func = (CvIPPDistTransFunc)icvDistanceTransform_3x3_8u_C1R_p;
        else
            ipp_inp_func = icvDistanceTransform_3x3_8u_C1IR_p;
    }

    size = cvGetMatSize(src);

    if( (ipp_func || ipp_inp_func) && src->cols >= 4 && src->rows >= 2 )
    {
        _imask[0] = cvRound(_mask[0]);
        _imask[1] = cvRound(_mask[1]);
        _imask[2] = cvRound(_mask[2]);

        if( ipp_func )
        {
            IPPI_CALL( ipp_func( src->data.ptr, src->step,
                    dst->data.fl, dst->step, size,
                    CV_MAT_TYPE(dst->type) == CV_8UC1 ?
                    (void*)_imask : (void*)_mask ));
        }
        else
        {
            IPPI_CALL( ipp_inp_func( src->data.ptr, src->step, size, _imask ));
        }
    }
    else if( CV_MAT_TYPE(dst->type) == CV_8UC1 )
    {
        CV_CALL( icvDistanceATS_L1_8u( src, dst ));
    }
    else
    {
        int border = maskSize == CV_DIST_MASK_3 ? 1 : 2;
        CV_CALL( temp = cvCreateMat( size.height + border*2, size.width + border*2, CV_32SC1 ));

        if( !labels )
        {
            CvDistTransFunc func = maskSize == CV_DIST_MASK_3 ?
                icvDistanceTransform_3x3_C1R :
                icvDistanceTransform_5x5_C1R;

            func( src->data.ptr, src->step, temp->data.i, temp->step,
                  dst->data.fl, dst->step, size, _mask );
        }
        else
        {
            CvSeq *contours = 0;
            CvPoint top_left = {0,0}, bottom_right = {size.width-1,size.height-1};
            int label;

            CV_CALL( st = cvCreateMemStorage() );
            CV_CALL( src_copy = cvCreateMat( size.height, size.width, src->type ));
            cvCmpS( src, 0, src_copy, CV_CMP_EQ );
            cvFindContours( src_copy, st, &contours, sizeof(CvContour),
                            CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
            cvZero( labels );
            for( label = 1; contours != 0; contours = contours->h_next, label++ )
            {
                CvScalar area_color = cvScalarAll(label);
                cvDrawContours( labels, contours, area_color, area_color, -255, -1, 8 );
            }

            cvCopy( src, src_copy );
            cvRectangle( src_copy, top_left, bottom_right, cvScalarAll(255), 1, 8 );

            icvDistanceTransformEx_5x5_C1R( src_copy->data.ptr, src_copy->step, temp->data.i, temp->step,
                        dst->data.fl, dst->step, labels->data.i, labels->step, size, _mask );
        }
    }

    __END__;

    cvReleaseMat( &temp );
    cvReleaseMat( &src_copy );
    cvReleaseMemStorage( &st );
}

/* End of file. */
