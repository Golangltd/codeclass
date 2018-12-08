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

/*
 * This file includes the code, contributed by Simon Perreault
 * (the function icvMedianBlur_8u_CnR_O1)
 *
 * Constant-time median filtering -- http://nomis80.org/ctmf.html
 * Copyright (C) 2006 Simon Perreault
 *
 * Contact:
 *  Laboratoire de vision et systemes numeriques
 *  Pavillon Adrien-Pouliot
 *  Universite Laval
 *  Sainte-Foy, Quebec, Canada
 *  G1K 7P4
 *
 *  perreaul@gel.ulaval.ca
 */

// uncomment the line below to force SSE2 mode
//#define CV_SSE2 1

/****************************************************************************************\
                                         Box Filter
\****************************************************************************************/

static void icvSumRow_8u32s( const uchar* src0, int* dst, void* params );
static void icvSumRow_32f64f( const float* src0, double* dst, void* params );
static void icvSumCol_32s8u( const int** src, uchar* dst, int dst_step,
                             int count, void* params );
static void icvSumCol_32s16s( const int** src, short* dst, int dst_step,
                             int count, void* params );
static void icvSumCol_32s32s( const int** src, int* dst, int dst_step,
                             int count, void* params );
static void icvSumCol_64f32f( const double** src, float* dst, int dst_step,
                              int count, void* params );

CvBoxFilter::CvBoxFilter()
{
    min_depth = CV_32S;
    sum = 0;
    sum_count = 0;
    normalized = false;
}


CvBoxFilter::CvBoxFilter( int _max_width, int _src_type, int _dst_type,
                          bool _normalized, CvSize _ksize,
                          CvPoint _anchor, int _border_mode,
                          CvScalar _border_value )
{
    min_depth = CV_32S;
    sum = 0;
    sum_count = 0;
    normalized = false;
    init( _max_width, _src_type, _dst_type, _normalized,
          _ksize, _anchor, _border_mode, _border_value );
}


CvBoxFilter::~CvBoxFilter()
{
    clear();
}


void CvBoxFilter::init( int _max_width, int _src_type, int _dst_type,
                        bool _normalized, CvSize _ksize,
                        CvPoint _anchor, int _border_mode,
                        CvScalar _border_value )
{
    CV_FUNCNAME( "CvBoxFilter::init" );

    __BEGIN__;
    
    sum = 0;
    normalized = _normalized;

    if( (normalized && CV_MAT_TYPE(_src_type) != CV_MAT_TYPE(_dst_type)) ||
        (!normalized && CV_MAT_CN(_src_type) != CV_MAT_CN(_dst_type)))
        CV_ERROR( CV_StsUnmatchedFormats,
        "In case of normalized box filter input and output must have the same type.\n"
        "In case of unnormalized box filter the number of input and output channels must be the same" );

    min_depth = CV_MAT_DEPTH(_src_type) == CV_8U ? CV_32S : CV_64F;

    CvBaseImageFilter::init( _max_width, _src_type, _dst_type, 1, _ksize,
                             _anchor, _border_mode, _border_value );
    
    scale = normalized ? 1./(ksize.width*ksize.height) : 1;

    if( CV_MAT_DEPTH(src_type) == CV_8U )
        x_func = (CvRowFilterFunc)icvSumRow_8u32s;
    else if( CV_MAT_DEPTH(src_type) == CV_32F )
        x_func = (CvRowFilterFunc)icvSumRow_32f64f;
    else
        CV_ERROR( CV_StsUnsupportedFormat, "Unknown/unsupported input image format" );

    if( CV_MAT_DEPTH(dst_type) == CV_8U )
    {
        if( !normalized )
            CV_ERROR( CV_StsBadArg, "Only normalized box filter can be used for 8u->8u transformation" );
        y_func = (CvColumnFilterFunc)icvSumCol_32s8u;
    }
    else if( CV_MAT_DEPTH(dst_type) == CV_16S )
    {
        if( normalized || CV_MAT_DEPTH(src_type) != CV_8U )
            CV_ERROR( CV_StsBadArg, "Only 8u->16s unnormalized box filter is supported in case of 16s output" );
        y_func = (CvColumnFilterFunc)icvSumCol_32s16s;
    }
	else if( CV_MAT_DEPTH(dst_type) == CV_32S )
	{
		if( normalized || CV_MAT_DEPTH(src_type) != CV_8U )
			CV_ERROR( CV_StsBadArg, "Only 8u->32s unnormalized box filter is supported in case of 32s output");

		y_func = (CvColumnFilterFunc)icvSumCol_32s32s;
	}
    else if( CV_MAT_DEPTH(dst_type) == CV_32F )
    {
        if( CV_MAT_DEPTH(src_type) != CV_32F )
            CV_ERROR( CV_StsBadArg, "Only 32f->32f box filter (normalized or not) is supported in case of 32f output" );
        y_func = (CvColumnFilterFunc)icvSumCol_64f32f;
    }
	else{
		CV_ERROR( CV_StsBadArg, "Unknown/unsupported destination image format" );
	}

    __END__;
}


void CvBoxFilter::start_process( CvSlice x_range, int width )
{
    CvBaseImageFilter::start_process( x_range, width );
    int i, psz = CV_ELEM_SIZE(work_type);
    uchar* s;
    buf_end -= buf_step;
    buf_max_count--;
    assert( buf_max_count >= max_ky*2 + 1 );
    s = sum = buf_end + cvAlign((width + ksize.width - 1)*CV_ELEM_SIZE(src_type), ALIGN);
    sum_count = 0;

    width *= psz;
    for( i = 0; i < width; i++ )
        s[i] = (uchar)0;
}


static void
icvSumRow_8u32s( const uchar* src, int* dst, void* params )
{
    const CvBoxFilter* state = (const CvBoxFilter*)params;
    int ksize = state->get_kernel_size().width;
    int width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int i, k;

    width = (width - 1)*cn; ksize *= cn;

    for( k = 0; k < cn; k++, src++, dst++ )
    {
        int s = 0;
        for( i = 0; i < ksize; i += cn )
            s += src[i];
        dst[0] = s;
        for( i = 0; i < width; i += cn )
        {
            s += src[i+ksize] - src[i];
            dst[i+cn] = s;
        }
    }
}


static void
icvSumRow_32f64f( const float* src, double* dst, void* params )
{
    const CvBoxFilter* state = (const CvBoxFilter*)params;
    int ksize = state->get_kernel_size().width;
    int width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int i, k;

    width = (width - 1)*cn; ksize *= cn;

    for( k = 0; k < cn; k++, src++, dst++ )
    {
        double s = 0;
        for( i = 0; i < ksize; i += cn )
            s += src[i];
        dst[0] = s;
        for( i = 0; i < width; i += cn )
        {
            s += (double)src[i+ksize] - src[i];
            dst[i+cn] = s;
        }
    }
}


static void
icvSumCol_32s8u( const int** src, uchar* dst,
                 int dst_step, int count, void* params )
{
#define BLUR_SHIFT 24
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    double scale = state->get_scale();
    int iscale = cvFloor(scale*(1 << BLUR_SHIFT));
    int* sum = (int*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const int* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                int t0 = CV_DESCALE(s0*iscale, BLUR_SHIFT), t1 = CV_DESCALE(s1*iscale, BLUR_SHIFT);
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
                dst[i] = (uchar)t0; dst[i+1] = (uchar)t1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i], t0 = CV_DESCALE(s0*iscale, BLUR_SHIFT);
                sum[i] = s0 - sm[i]; dst[i] = (uchar)t0;
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
#undef BLUR_SHIFT
}


static void
icvSumCol_32s16s( const int** src, short* dst,
                  int dst_step, int count, void* params )
{
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int ktotal = ksize*state->get_kernel_size().width;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int* sum = (int*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    dst_step /= sizeof(dst[0]);
    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const int* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else if( ktotal < 128 )
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                dst[i] = (short)s0; dst[i+1] = (short)s1;
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i];
                dst[i] = (short)s0;
                sum[i] = s0 - sm[i];
            }
            dst += dst_step;
        }
        else
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                dst[i] = CV_CAST_16S(s0); dst[i+1] = CV_CAST_16S(s1);
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i];
                dst[i] = CV_CAST_16S(s0);
                sum[i] = s0 - sm[i];
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
}

static void
icvSumCol_32s32s( const int** src, int * dst,
                  int dst_step, int count, void* params )
{
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int* sum = (int*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    dst_step /= sizeof(dst[0]);
    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const int* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                dst[i] = s0; dst[i+1] = s1;
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i];
                dst[i] = s0;
                sum[i] = s0 - sm[i];
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
}


static void
icvSumCol_64f32f( const double** src, float* dst,
                  int dst_step, int count, void* params )
{
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    double scale = state->get_scale();
    bool normalized = state->is_normalized();
    double* sum = (double*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    dst_step /= sizeof(dst[0]);
    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const double* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                double s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else
        {
            const double* sm = src[-ksize+1];
            if( normalized )
                for( i = 0; i <= width - 2; i += 2 )
                {
                    double s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                    double t0 = s0*scale, t1 = s1*scale;
                    s0 -= sm[i]; s1 -= sm[i+1];
                    dst[i] = (float)t0; dst[i+1] = (float)t1;
                    sum[i] = s0; sum[i+1] = s1;
                }
            else
                for( i = 0; i <= width - 2; i += 2 )
                {
                    double s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                    dst[i] = (float)s0; dst[i+1] = (float)s1;
                    s0 -= sm[i]; s1 -= sm[i+1];
                    sum[i] = s0; sum[i+1] = s1;
                }

            for( ; i < width; i++ )
            {
                double s0 = sum[i] + sp[i], t0 = s0*scale;
                sum[i] = s0 - sm[i]; dst[i] = (float)t0;
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
}


/****************************************************************************************\
                                      Median Filter
\****************************************************************************************/

#define CV_MINMAX_8U(a,b) \
    (t = CV_FAST_CAST_8U((a) - (b)), (b) += t, a -= t)

#if CV_SSE2 && !defined __SSE2__
#define __SSE2__ 1
#include "emmintrin.h"
#endif

#if defined(__VEC__) || defined(__ALTIVEC__)
#include <altivec.h>
#undef bool
#endif

#if defined(__GNUC__)
#define align(x) __attribute__ ((aligned (x)))
#elif CV_SSE2 && (defined(__ICL) || (_MSC_VER >= 1300))
#define align(x) __declspec(align(x))
#else
#define align(x)
#endif

#if _MSC_VER >= 1200
#pragma warning( disable: 4244 )
#endif

/**
 * This structure represents a two-tier histogram. The first tier (known as the
 * "coarse" level) is 4 bit wide and the second tier (known as the "fine" level)
 * is 8 bit wide. Pixels inserted in the fine level also get inserted into the
 * coarse bucket designated by the 4 MSBs of the fine bucket value.
 *
 * The structure is aligned on 16 bits, which is a prerequisite for SIMD
 * instructions. Each bucket is 16 bit wide, which means that extra care must be
 * taken to prevent overflow.
 */
typedef struct align(16)
{
    ushort coarse[16];
    ushort fine[16][16];
} Histogram;

/**
 * HOP is short for Histogram OPeration. This macro makes an operation \a op on
 * histogram \a h for pixel value \a x. It takes care of handling both levels.
 */
#define HOP(h,x,op) \
    h.coarse[x>>4] op; \
    *((ushort*) h.fine + x) op;

#define COP(c,j,x,op) \
    h_coarse[ 16*(n*c+j) + (x>>4) ] op; \
    h_fine[ 16 * (n*(16*c+(x>>4)) + j) + (x & 0xF) ] op;

#if defined __SSE2__ || defined __MMX__ || defined __ALTIVEC__
#define MEDIAN_HAVE_SIMD 1
#else
#define MEDIAN_HAVE_SIMD 0
#endif

/**
 * Adds histograms \a x and \a y and stores the result in \a y. Makes use of
 * SSE2, MMX or Altivec, if available.
 */
#if defined(__SSE2__)
static inline void histogram_add( const ushort x[16], ushort y[16] )
{
    _mm_store_si128( (__m128i*) &y[0], _mm_add_epi16(
        _mm_load_si128((__m128i*) &y[0]), _mm_load_si128((__m128i*) &x[0] )));
    _mm_store_si128( (__m128i*) &y[8], _mm_add_epi16(
        _mm_load_si128((__m128i*) &y[8]), _mm_load_si128((__m128i*) &x[8] )));
}
#elif defined(__MMX__)
static inline void histogram_add( const ushort x[16], ushort y[16] )
{
    *(__m64*) &y[0]  = _mm_add_pi16( *(__m64*) &y[0],  *(__m64*) &x[0]  );
    *(__m64*) &y[4]  = _mm_add_pi16( *(__m64*) &y[4],  *(__m64*) &x[4]  );
    *(__m64*) &y[8]  = _mm_add_pi16( *(__m64*) &y[8],  *(__m64*) &x[8]  );
    *(__m64*) &y[12] = _mm_add_pi16( *(__m64*) &y[12], *(__m64*) &x[12] );
}
#elif defined(__ALTIVEC__)
static inline void histogram_add( const ushort x[16], ushort y[16] )
{
    *(vector ushort*) &y[0] = vec_add( *(vector ushort*) &y[0], *(vector ushort*) &x[0] );
    *(vector ushort*) &y[8] = vec_add( *(vector ushort*) &y[8], *(vector ushort*) &x[8] );
}
#else
static inline void histogram_add( const ushort x[16], ushort y[16] )
{
    int i;
    for( i = 0; i < 16; ++i )
        y[i] = (ushort)(y[i] + x[i]);
}
#endif

/**
 * Subtracts histogram \a x from \a y and stores the result in \a y. Makes use
 * of SSE2, MMX or Altivec, if available.
 */
#if defined(__SSE2__)
static inline void histogram_sub( const ushort x[16], ushort y[16] )
{
    _mm_store_si128( (__m128i*) &y[0], _mm_sub_epi16(
        _mm_load_si128((__m128i*) &y[0]), _mm_load_si128((__m128i*) &x[0] )));
    _mm_store_si128( (__m128i*) &y[8], _mm_sub_epi16(
        _mm_load_si128((__m128i*) &y[8]), _mm_load_si128((__m128i*) &x[8] )));
}
#elif defined(__MMX__)
static inline void histogram_sub( const ushort x[16], ushort y[16] )
{
    *(__m64*) &y[0]  = _mm_sub_pi16( *(__m64*) &y[0],  *(__m64*) &x[0]  );
    *(__m64*) &y[4]  = _mm_sub_pi16( *(__m64*) &y[4],  *(__m64*) &x[4]  );
    *(__m64*) &y[8]  = _mm_sub_pi16( *(__m64*) &y[8],  *(__m64*) &x[8]  );
    *(__m64*) &y[12] = _mm_sub_pi16( *(__m64*) &y[12], *(__m64*) &x[12] );
}
#elif defined(__ALTIVEC__)
static inline void histogram_sub( const ushort x[16], ushort y[16] )
{
    *(vector ushort*) &y[0] = vec_sub( *(vector ushort*) &y[0], *(vector ushort*) &x[0] );
    *(vector ushort*) &y[8] = vec_sub( *(vector ushort*) &y[8], *(vector ushort*) &x[8] );
}
#else
static inline void histogram_sub( const ushort x[16], ushort y[16] )
{
    int i;
    for( i = 0; i < 16; ++i )
        y[i] = (ushort)(y[i] - x[i]);
}
#endif

static inline void histogram_muladd( int a, const ushort x[16],
        ushort y[16] )
{
    int i;
    for ( i = 0; i < 16; ++i )
        y[i] = (ushort)(y[i] + a * x[i]);
}

static CvStatus CV_STDCALL
icvMedianBlur_8u_CnR_O1( uchar* src, int src_step, uchar* dst, int dst_step,
                         CvSize size, int kernel_size, int cn, int pad_left, int pad_right )
{
    int r = (kernel_size-1)/2;
    const int m = size.height, n = size.width;
    int i, j, k, c;
    const unsigned char *p, *q;
    Histogram H[4];
    ushort *h_coarse, *h_fine, luc[4][16];

    if( size.height < r || size.width < r )
        return CV_BADSIZE_ERR;

    assert( src );
    assert( dst );
    assert( r >= 0 );
    assert( size.width >= 2*r+1 );
    assert( size.height >= 2*r+1 );
    assert( src_step != 0 );
    assert( dst_step != 0 );

    h_coarse = (ushort*) cvAlloc(  1 * 16 * n * cn * sizeof(ushort) );
    h_fine   = (ushort*) cvAlloc( 16 * 16 * n * cn * sizeof(ushort) );
    memset( h_coarse, 0,  1 * 16 * n * cn * sizeof(ushort) );
    memset( h_fine,   0, 16 * 16 * n * cn * sizeof(ushort) );

    /* First row initialization */
    for ( j = 0; j < n; ++j ) {
        for ( c = 0; c < cn; ++c ) {
            COP( c, j, src[cn*j+c], += r+1 );
        }
    }
    for ( i = 0; i < r; ++i ) {
        for ( j = 0; j < n; ++j ) {
            for ( c = 0; c < cn; ++c ) {
                COP( c, j, src[src_step*i+cn*j+c], ++ );
            }
        }
    }

    for ( i = 0; i < m; ++i ) {

        /* Update column histograms for entire row. */
        p = src + src_step * MAX( 0, i-r-1 );
        q = p + cn * n;
        for ( j = 0; p != q; ++j ) {
            for ( c = 0; c < cn; ++c, ++p ) {
                COP( c, j, *p, -- );
            }
        }

        p = src + src_step * MIN( m-1, i+r );
        q = p + cn * n;
        for ( j = 0; p != q; ++j ) {
            for ( c = 0; c < cn; ++c, ++p ) {
                COP( c, j, *p, ++ );
            }
        }

        /* First column initialization */
        memset( H, 0, cn*sizeof(H[0]) );
        memset( luc, 0, cn*sizeof(luc[0]) );
        if ( pad_left ) {
            for ( c = 0; c < cn; ++c ) {
                histogram_muladd( r, &h_coarse[16*n*c], H[c].coarse );
            }
        }
        for ( j = 0; j < (pad_left ? r : 2*r); ++j ) {
            for ( c = 0; c < cn; ++c ) {
                histogram_add( &h_coarse[16*(n*c+j)], H[c].coarse );
            }
        }
        for ( c = 0; c < cn; ++c ) {
            for ( k = 0; k < 16; ++k ) {
                histogram_muladd( 2*r+1, &h_fine[16*n*(16*c+k)], &H[c].fine[k][0] );
            }
        }

        for ( j = pad_left ? 0 : r; j < (pad_right ? n : n-r); ++j ) {
            for ( c = 0; c < cn; ++c ) {
                int t = 2*r*r + 2*r, b, sum = 0;
                ushort* segment;

                histogram_add( &h_coarse[16*(n*c + MIN(j+r,n-1))], H[c].coarse );

                /* Find median at coarse level */
                for ( k = 0; k < 16 ; ++k ) {
                    sum += H[c].coarse[k];
                    if ( sum > t ) {
                        sum -= H[c].coarse[k];
                        break;
                    }
                }
                assert( k < 16 );

                /* Update corresponding histogram segment */
                if ( luc[c][k] <= j-r ) {
                    memset( &H[c].fine[k], 0, 16 * sizeof(ushort) );
                    for ( luc[c][k] = j-r; luc[c][k] < MIN(j+r+1,n); ++luc[c][k] ) {
                        histogram_add( &h_fine[16*(n*(16*c+k)+luc[c][k])], H[c].fine[k] );
                    }
                    if ( luc[c][k] < j+r+1 ) {
                        histogram_muladd( j+r+1 - n, &h_fine[16*(n*(16*c+k)+(n-1))], &H[c].fine[k][0] );
                        luc[c][k] = (ushort)(j+r+1);
                    }
                }
                else {
                    for ( ; luc[c][k] < j+r+1; ++luc[c][k] ) {
                        histogram_sub( &h_fine[16*(n*(16*c+k)+MAX(luc[c][k]-2*r-1,0))], H[c].fine[k] );
                        histogram_add( &h_fine[16*(n*(16*c+k)+MIN(luc[c][k],n-1))], H[c].fine[k] );
                    }
                }

                histogram_sub( &h_coarse[16*(n*c+MAX(j-r,0))], H[c].coarse );

                /* Find median in segment */
                segment = H[c].fine[k];
                for ( b = 0; b < 16 ; ++b ) {
                    sum += segment[b];
                    if ( sum > t ) {
                        dst[dst_step*i+cn*j+c] = (uchar)(16*k + b);
                        break;
                    }
                }
                assert( b < 16 );
            }
        }
    }

#if defined(__MMX__)
    _mm_empty();
#endif

    cvFree(&h_coarse);
    cvFree(&h_fine);

#undef HOP
#undef COP
    return CV_OK;
}


#if _MSC_VER >= 1200
#pragma warning( default: 4244 )
#endif


static CvStatus CV_STDCALL
icvMedianBlur_8u_CnR_Om( uchar* src, int src_step, uchar* dst, int dst_step,
                         CvSize size, int m, int cn )
{
    #define N  16
    int     zone0[4][N];
    int     zone1[4][N*N];
    int     x, y;
    int     n2 = m*m/2;
    int     nx = (m + 1)/2 - 1;
    uchar*  src_max = src + size.height*src_step;
    uchar*  src_right = src + size.width*cn;

    #define UPDATE_ACC01( pix, cn, op ) \
    {                                   \
        int p = (pix);                  \
        zone1[cn][p] op;                \
        zone0[cn][p >> 4] op;           \
    }

    if( size.height < nx || size.width < nx )
        return CV_BADSIZE_ERR;

    if( m == 3 )
    {
        size.width *= cn;

        for( y = 0; y < size.height; y++, dst += dst_step )
        {
            const uchar* src0 = src + src_step*(y-1);
            const uchar* src1 = src0 + src_step;
            const uchar* src2 = src1 + src_step;
            if( y == 0 )
                src0 = src1;
            else if( y == size.height - 1 )
                src2 = src1;

            for( x = 0; x < 2*cn; x++ )
            {
                int x0 = x < cn ? x : size.width - 3*cn + x;
                int x2 = x < cn ? x + cn : size.width - 2*cn + x;
                int x1 = x < cn ? x0 : x2, t;

                int p0 = src0[x0], p1 = src0[x1], p2 = src0[x2];
                int p3 = src1[x0], p4 = src1[x1], p5 = src1[x2];
                int p6 = src2[x0], p7 = src2[x1], p8 = src2[x2];

                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p1);
                CV_MINMAX_8U(p3, p4); CV_MINMAX_8U(p6, p7);
                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p3);
                CV_MINMAX_8U(p5, p8); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p3, p6); CV_MINMAX_8U(p1, p4);
                CV_MINMAX_8U(p2, p5); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p4, p2); CV_MINMAX_8U(p6, p4);
                CV_MINMAX_8U(p4, p2);
                dst[x1] = (uchar)p4;
            }

            for( x = cn; x < size.width - cn; x++ )
            {
                int p0 = src0[x-cn], p1 = src0[x], p2 = src0[x+cn];
                int p3 = src1[x-cn], p4 = src1[x], p5 = src1[x+cn];
                int p6 = src2[x-cn], p7 = src2[x], p8 = src2[x+cn];
                int t;

                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p1);
                CV_MINMAX_8U(p3, p4); CV_MINMAX_8U(p6, p7);
                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p3);
                CV_MINMAX_8U(p5, p8); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p3, p6); CV_MINMAX_8U(p1, p4);
                CV_MINMAX_8U(p2, p5); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p4, p2); CV_MINMAX_8U(p6, p4);
                CV_MINMAX_8U(p4, p2);

                dst[x] = (uchar)p4;
            }
        }

        return CV_OK;
    }

    for( x = 0; x < size.width; x++, dst += cn )
    {
        uchar* dst_cur = dst;
        uchar* src_top = src;
        uchar* src_bottom = src;
        int    k, c;
        int    x0 = -1;
        int    src_step1 = src_step, dst_step1 = dst_step;

        if( x % 2 != 0 )
        {
            src_bottom = src_top += src_step*(size.height-1);
            dst_cur += dst_step*(size.height-1);
            src_step1 = -src_step1;
            dst_step1 = -dst_step1;
        }

        if( x <= m/2 )
            nx++;

        if( nx < m )
            x0 = x < m/2 ? 0 : (nx-1)*cn;

        // init accumulator
        memset( zone0, 0, sizeof(zone0[0])*cn );
        memset( zone1, 0, sizeof(zone1[0])*cn );

        for( y = 0; y <= m/2; y++ )
        {
            for( c = 0; c < cn; c++ )
            {
                if( y > 0 )
                {
                    if( x0 >= 0 )
                        UPDATE_ACC01( src_bottom[x0+c], c, += (m - nx) );
                    for( k = 0; k < nx*cn; k += cn )
                        UPDATE_ACC01( src_bottom[k+c], c, ++ );
                }
                else
                {
                    if( x0 >= 0 )
                        UPDATE_ACC01( src_bottom[x0+c], c, += (m - nx)*(m/2+1) );
                    for( k = 0; k < nx*cn; k += cn )
                        UPDATE_ACC01( src_bottom[k+c], c, += m/2+1 );
                }
            }

            if( (src_step1 > 0 && y < size.height-1) ||
                (src_step1 < 0 && size.height-y-1 > 0) )
                src_bottom += src_step1;
        }

        for( y = 0; y < size.height; y++, dst_cur += dst_step1 )
        {
            // find median
            for( c = 0; c < cn; c++ )
            {
                int s = 0;
                for( k = 0; ; k++ )
                {
                    int t = s + zone0[c][k];
                    if( t > n2 ) break;
                    s = t;
                }

                for( k *= N; ;k++ )
                {
                    s += zone1[c][k];
                    if( s > n2 ) break;
                }

                dst_cur[c] = (uchar)k;
            }

            if( y+1 == size.height )
                break;

            if( cn == 1 )
            {
                for( k = 0; k < nx; k++ )
                {
                    int p = src_top[k];
                    int q = src_bottom[k];
                    zone1[0][p]--;
                    zone0[0][p>>4]--;
                    zone1[0][q]++;
                    zone0[0][q>>4]++;
                }
            }
            else if( cn == 3 )
            {
                for( k = 0; k < nx*3; k += 3 )
                {
                    UPDATE_ACC01( src_top[k], 0, -- );
                    UPDATE_ACC01( src_top[k+1], 1, -- );
                    UPDATE_ACC01( src_top[k+2], 2, -- );

                    UPDATE_ACC01( src_bottom[k], 0, ++ );
                    UPDATE_ACC01( src_bottom[k+1], 1, ++ );
                    UPDATE_ACC01( src_bottom[k+2], 2, ++ );
                }
            }
            else
            {
                assert( cn == 4 );
                for( k = 0; k < nx*4; k += 4 )
                {
                    UPDATE_ACC01( src_top[k], 0, -- );
                    UPDATE_ACC01( src_top[k+1], 1, -- );
                    UPDATE_ACC01( src_top[k+2], 2, -- );
                    UPDATE_ACC01( src_top[k+3], 3, -- );

                    UPDATE_ACC01( src_bottom[k], 0, ++ );
                    UPDATE_ACC01( src_bottom[k+1], 1, ++ );
                    UPDATE_ACC01( src_bottom[k+2], 2, ++ );
                    UPDATE_ACC01( src_bottom[k+3], 3, ++ );
                }
            }

            if( x0 >= 0 )
            {
                for( c = 0; c < cn; c++ )
                {
                    UPDATE_ACC01( src_top[x0+c], c, -= (m - nx) );
                    UPDATE_ACC01( src_bottom[x0+c], c, += (m - nx) );
                }
            }

            if( (src_step1 > 0 && src_bottom + src_step1 < src_max) ||
                (src_step1 < 0 && src_bottom + src_step1 >= src) )
                src_bottom += src_step1;

            if( y >= m/2 )
                src_top += src_step1;
        }

        if( x >= m/2 )
            src += cn;
        if( src + nx*cn > src_right ) nx--;
    }
#undef N
#undef UPDATE_ACC
    return CV_OK;
}


/****************************************************************************************\
                                   Bilateral Filtering
\****************************************************************************************/

static void
icvBilateralFiltering_8u( const CvMat* src, CvMat* dst, int d,
                          double sigma_color, double sigma_space )
{
    CvMat* temp = 0;
    float* color_weight = 0;
    float* space_weight = 0;
    int* space_ofs = 0;

    CV_FUNCNAME( "icvBilateralFiltering_8u" );

    __BEGIN__;

    double gauss_color_coeff = -0.5/(sigma_color*sigma_color);
    double gauss_space_coeff = -0.5/(sigma_space*sigma_space);
    int cn = CV_MAT_CN(src->type);
    int i, j, k, maxk, radius;
    CvSize size = cvGetMatSize(src);

    if( (CV_MAT_TYPE(src->type) != CV_8UC1 &&
        CV_MAT_TYPE(src->type) != CV_8UC3) ||
        !CV_ARE_TYPES_EQ(src, dst) )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Both source and destination must be 8-bit, single-channel or 3-channel images" );

    if( sigma_color <= 0 )
        sigma_color = 1;
    if( sigma_space <= 0 )
        sigma_space = 1;

    if( d == 0 )
        radius = cvRound(sigma_space*1.5);
    else
        radius = d/2;
    radius = MAX(radius, 1);
    d = radius*2 + 1;

    CV_CALL( temp = cvCreateMat( src->rows + radius*2,
        src->cols + radius*2, src->type ));
    CV_CALL( cvCopyMakeBorder( src, temp, cvPoint(radius,radius), IPL_BORDER_REPLICATE ));
    CV_CALL( color_weight = (float*)cvAlloc(cn*256*sizeof(color_weight[0])));
    CV_CALL( space_weight = (float*)cvAlloc(d*d*sizeof(space_weight[0])));
    CV_CALL( space_ofs = (int*)cvAlloc(d*d*sizeof(space_ofs[0])));

    // initialize color-related bilateral filter coefficients
    for( i = 0; i < 256*cn; i++ )
        color_weight[i] = (float)exp(i*i*gauss_color_coeff);

    // initialize space-related bilateral filter coefficients
    for( i = -radius, maxk = 0; i <= radius; i++ )
        for( j = -radius; j <= radius; j++ )
        {
            double r = sqrt((double)i*i + (double)j*j);
            if( r > radius )
                continue;
            space_weight[maxk] = (float)exp(r*r*gauss_space_coeff);
            space_ofs[maxk++] = i*temp->step + j*cn;
        }

    for( i = 0; i < size.height; i++ )
    {
        const uchar* sptr = temp->data.ptr + (i+radius)*temp->step + radius*cn;
        uchar* dptr = dst->data.ptr + i*dst->step;

        if( cn == 1 )
        {
            for( j = 0; j < size.width; j++ )
            {
                float sum = 0, wsum = 0;
                int val0 = sptr[j];
                for( k = 0; k < maxk; k++ )
                {
                    int val = sptr[j + space_ofs[k]];
                    float w = space_weight[k]*color_weight[abs(val - val0)];
                    sum += val*w;
                    wsum += w;
                }
                // overflow is not possible here => there is no need to use CV_CAST_8U
                dptr[j] = (uchar)cvRound(sum/wsum);
            }
        }
        else
        {
            assert( cn == 3 );
            for( j = 0; j < size.width*3; j += 3 )
            {
                float sum_b = 0, sum_g = 0, sum_r = 0, wsum = 0;
                int b0 = sptr[j], g0 = sptr[j+1], r0 = sptr[j+2];
                for( k = 0; k < maxk; k++ )
                {
                    const uchar* sptr_k = sptr + j + space_ofs[k];
                    int b = sptr_k[0], g = sptr_k[1], r = sptr_k[2];
                    float w = space_weight[k]*color_weight[abs(b - b0) +
                        abs(g - g0) + abs(r - r0)];
                    sum_b += b*w; sum_g += g*w; sum_r += r*w;
                    wsum += w;
                }
                wsum = 1.f/wsum;
                b0 = cvRound(sum_b*wsum);
                g0 = cvRound(sum_g*wsum);
                r0 = cvRound(sum_r*wsum);
                dptr[j] = (uchar)b0; dptr[j+1] = (uchar)g0; dptr[j+2] = (uchar)r0;
            }
        }
    }

    __END__;

    cvReleaseMat( &temp );
    cvFree( &color_weight );
    cvFree( &space_weight );
    cvFree( &space_ofs );
}


static void icvBilateralFiltering_32f( const CvMat* src, CvMat* dst, int d,
                                       double sigma_color, double sigma_space )
{
  	CvMat* temp = 0;
    float* space_weight = 0;
    int* space_ofs = 0;
    float *expLUT = 0;
 
    CV_FUNCNAME( "icvBilateralFiltering_32f" );
    
    __BEGIN__;

    double gauss_color_coeff = -0.5/(sigma_color*sigma_color);
    double gauss_space_coeff = -0.5/(sigma_space*sigma_space);
    int cn = CV_MAT_CN(src->type);
    int i, j, k, maxk, radius;
    double minValSrc=-1, maxValSrc=1;
    const int kExpNumBinsPerChannel = 1 << 12;
    int kExpNumBins = 0;
    float lastExpVal = 1.f;
    int temp_step;
    float len, scale_index;
    CvMat src_reshaped;

    CvSize size = cvGetMatSize(src);

    if( (CV_MAT_TYPE(src->type) != CV_32FC1 &&
        CV_MAT_TYPE(src->type) != CV_32FC3) ||
        !CV_ARE_TYPES_EQ(src, dst) )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Both source and destination must be 32-bit float, single-channel or 3-channel images" );

    if( sigma_color <= 0 )
        sigma_color = 1;
    if( sigma_space <= 0 )
        sigma_space = 1;

    if( d == 0 )
        radius = cvRound(sigma_space*1.5);
    else
        radius = d/2;
    radius = MAX(radius, 1);
    d = radius*2 + 1;
    // compute the min/max range for the input image (even if multichannel)
    
    CV_CALL( cvReshape( src, &src_reshaped, 1 ) );   
    CV_CALL( cvMinMaxLoc(&src_reshaped, &minValSrc, &maxValSrc) ); 
    
    // temporary copy of the image with borders for easy processing
    CV_CALL( temp = cvCreateMat( src->rows + radius*2,
        src->cols + radius*2, src->type ));
    temp_step = temp->step/sizeof(float);
    CV_CALL( cvCopyMakeBorder( src, temp, cvPoint(radius,radius), IPL_BORDER_REPLICATE ));
    // allocate lookup tables
    CV_CALL( space_weight = (float*)cvAlloc(d*d*sizeof(space_weight[0])));
    CV_CALL( space_ofs = (int*)cvAlloc(d*d*sizeof(space_ofs[0])));
   
    // assign a length which is slightly more than needed
    len = (float)(maxValSrc - minValSrc) * cn;
    kExpNumBins = kExpNumBinsPerChannel * cn;
    CV_CALL( expLUT = (float*)cvAlloc((kExpNumBins+2) * sizeof(expLUT[0])));
    scale_index = kExpNumBins/len;
    
    // initialize the exp LUT
    for( i = 0; i < kExpNumBins+2; i++ )
    {
        if( lastExpVal > 0.f )
        {
            double val =  i / scale_index;
            expLUT[i] = (float)exp(val * val * gauss_color_coeff);
            lastExpVal = expLUT[i];
        }
        else
            expLUT[i] = 0.f;
    }
    
    // initialize space-related bilateral filter coefficients
    for( i = -radius, maxk = 0; i <= radius; i++ )
        for( j = -radius; j <= radius; j++ )
        {
            double r = sqrt((double)i*i + (double)j*j);
            if( r > radius )
                continue;
            space_weight[maxk] = (float)exp(r*r*gauss_space_coeff);
            space_ofs[maxk++] = i*temp_step + j*cn;
        }

    for( i = 0; i < size.height; i++ )
    {
	    const float* sptr = temp->data.fl + (i+radius)*temp_step + radius*cn;
        float* dptr = (float*)(dst->data.ptr + i*dst->step);

        if( cn == 1 )
        {
            for( j = 0; j < size.width; j++ )
            {
                float sum = 0, wsum = 0;
                float val0 = sptr[j];
                for( k = 0; k < maxk; k++ )
                {
                    float val = sptr[j + space_ofs[k]];
					float alpha = (float)(fabs(val - val0)*scale_index);
                    int idx = cvFloor(alpha);
                    alpha -= idx;
                    float w = space_weight[k]*(expLUT[idx] + alpha*(expLUT[idx+1] - expLUT[idx]));
	                sum += val*w;
                    wsum += w;
                }
                dptr[j] = (float)(sum/wsum);
            }
        }
        else
        {
            assert( cn == 3 );
            for( j = 0; j < size.width*3; j += 3 )
            {
                float sum_b = 0, sum_g = 0, sum_r = 0, wsum = 0;
                float b0 = sptr[j], g0 = sptr[j+1], r0 = sptr[j+2];
                for( k = 0; k < maxk; k++ )
                {
                    const float* sptr_k = sptr + j + space_ofs[k];
                    float b = sptr_k[0], g = sptr_k[1], r = sptr_k[2];
					float alpha = (float)((fabs(b - b0) + fabs(g - g0) + fabs(r - r0))*scale_index);
                    int idx = cvFloor(alpha);
                    alpha -= idx;
                    float w = space_weight[k]*(expLUT[idx] + alpha*(expLUT[idx+1] - expLUT[idx]));
                    sum_b += b*w; sum_g += g*w; sum_r += r*w;
                    wsum += w;
                }
                wsum = 1.f/wsum;
                b0 = sum_b*wsum;
                g0 = sum_g*wsum;
                r0 = sum_r*wsum;
                dptr[j] = b0; dptr[j+1] = g0; dptr[j+2] = r0;
            }
        }
    }
    
    __END__;

    cvReleaseMat( &temp );
    cvFree( &space_weight );
    cvFree( &space_ofs );
    cvFree( &expLUT );
}

//////////////////////////////// IPP smoothing functions /////////////////////////////////

icvFilterMedian_8u_C1R_t icvFilterMedian_8u_C1R_p = 0;
icvFilterMedian_8u_C3R_t icvFilterMedian_8u_C3R_p = 0;
icvFilterMedian_8u_C4R_t icvFilterMedian_8u_C4R_p = 0;

icvFilterBox_8u_C1R_t icvFilterBox_8u_C1R_p = 0;
icvFilterBox_8u_C3R_t icvFilterBox_8u_C3R_p = 0;
icvFilterBox_8u_C4R_t icvFilterBox_8u_C4R_p = 0;
icvFilterBox_32f_C1R_t icvFilterBox_32f_C1R_p = 0;
icvFilterBox_32f_C3R_t icvFilterBox_32f_C3R_p = 0;
icvFilterBox_32f_C4R_t icvFilterBox_32f_C4R_p = 0;

typedef CvStatus (CV_STDCALL * CvSmoothFixedIPPFunc)
( const void* src, int srcstep, void* dst, int dststep,
  CvSize size, CvSize ksize, CvPoint anchor );

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvSmooth( const void* srcarr, void* dstarr, int smooth_type,
          int param1, int param2, double param3, double param4 )
{
    CvBoxFilter box_filter;
    CvSepFilter gaussian_filter;

    CvMat* temp = 0;

    CV_FUNCNAME( "cvSmooth" );

    __BEGIN__;

    int coi1 = 0, coi2 = 0;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_type, dst_type, depth, cn;
    double sigma1 = 0, sigma2 = 0;
    bool have_ipp = icvFilterMedian_8u_C1R_p != 0;

    CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    src_type = CV_MAT_TYPE( src->type );
    dst_type = CV_MAT_TYPE( dst->type );
    depth = CV_MAT_DEPTH(src_type);
    cn = CV_MAT_CN(src_type);
    size = cvGetMatSize(src);

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( smooth_type != CV_BLUR_NO_SCALE && !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats,
        "The specified smoothing algorithm requires input and ouput arrays be of the same type" );

    if( smooth_type == CV_BLUR || smooth_type == CV_BLUR_NO_SCALE ||
        smooth_type == CV_GAUSSIAN || smooth_type == CV_MEDIAN )
    {
        // automatic detection of kernel size from sigma
        if( smooth_type == CV_GAUSSIAN )
        {
            sigma1 = param3;
            sigma2 = param4 ? param4 : param3;

            if( param1 == 0 && sigma1 > 0 )
                param1 = cvRound(sigma1*(depth == CV_8U ? 3 : 4)*2 + 1)|1;
            if( param2 == 0 && sigma2 > 0 )
                param2 = cvRound(sigma2*(depth == CV_8U ? 3 : 4)*2 + 1)|1;
        }

        if( param2 == 0 )
            param2 = size.height == 1 ? 1 : param1;
        if( param1 < 1 || (param1 & 1) == 0 || param2 < 1 || (param2 & 1) == 0 )
            CV_ERROR( CV_StsOutOfRange,
                "Both mask width and height must be >=1 and odd" );

        if( param1 == 1 && param2 == 1 )
        {
            cvConvert( src, dst );
            EXIT;
        }
    }

    if( have_ipp && (smooth_type == CV_BLUR || (smooth_type == CV_MEDIAN && param1 <= 15)) &&
        size.width >= param1 && size.height >= param2 && param1 > 1 && param2 > 1 )
    {
        CvSmoothFixedIPPFunc ipp_median_box_func = 0;

        if( smooth_type == CV_BLUR )
        {
            ipp_median_box_func =
                src_type == CV_8UC1 ? icvFilterBox_8u_C1R_p :
                src_type == CV_8UC3 ? icvFilterBox_8u_C3R_p :
                src_type == CV_8UC4 ? icvFilterBox_8u_C4R_p :
                src_type == CV_32FC1 ? icvFilterBox_32f_C1R_p :
                src_type == CV_32FC3 ? icvFilterBox_32f_C3R_p :
                src_type == CV_32FC4 ? icvFilterBox_32f_C4R_p : 0;
        }
        else if( smooth_type == CV_MEDIAN )
        {
            ipp_median_box_func =
                src_type == CV_8UC1 ? icvFilterMedian_8u_C1R_p :
                src_type == CV_8UC3 ? icvFilterMedian_8u_C3R_p :
                src_type == CV_8UC4 ? icvFilterMedian_8u_C4R_p : 0;
        }

        if( ipp_median_box_func )
        {
            CvSize el_size = { param1, param2 };
            CvPoint el_anchor = { param1/2, param2/2 };
            int stripe_size = 1 << 14; // the optimal value may depend on CPU cache,
                                       // overhead of the current IPP code etc.
            const uchar* shifted_ptr;
            int y, dy = 0;
            int temp_step, dst_step = dst->step;

            CV_CALL( temp = icvIPPFilterInit( src, stripe_size, el_size ));

            shifted_ptr = temp->data.ptr +
                el_anchor.y*temp->step + el_anchor.x*CV_ELEM_SIZE(src_type);
            temp_step = temp->step ? temp->step : CV_STUB_STEP;

            for( y = 0; y < src->rows; y += dy )
            {
                dy = icvIPPFilterNextStripe( src, temp, y, el_size, el_anchor );
                IPPI_CALL( ipp_median_box_func( shifted_ptr, temp_step,
                    dst->data.ptr + y*dst_step, dst_step, cvSize(src->cols, dy),
                    el_size, el_anchor ));
            }
            EXIT;
        }
    }

    if( smooth_type == CV_BLUR || smooth_type == CV_BLUR_NO_SCALE )
    {
        CV_CALL( box_filter.init( src->cols, src_type, dst_type,
            smooth_type == CV_BLUR, cvSize(param1, param2) ));
        CV_CALL( box_filter.process( src, dst ));
    }
    else if( smooth_type == CV_MEDIAN )
    {
        int img_size_mp = size.width*size.height;
        img_size_mp = (img_size_mp + (1<<19)) >> 20;

        if( depth != CV_8U || (cn != 1 && cn != 3 && cn != 4) )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Median filter only supports 8uC1, 8uC3 and 8uC4 images" );

        if( size.width < param1*2 || size.height < param1*2 ||
            param1 <= 3 + (img_size_mp < 1 ? 12 : img_size_mp < 4 ? 6 : 2)*(MEDIAN_HAVE_SIMD ? 1 : 3))
        {
            // Special case optimized for 3x3
            IPPI_CALL( icvMedianBlur_8u_CnR_Om( src->data.ptr, src->step,
                dst->data.ptr, dst->step, size, param1, cn ));
        }
        else
        {
            const int r = (param1 - 1) / 2;
            const int CACHE_SIZE = (int) ( 0.95 * 256 * 1024 / cn );  // assume a 256 kB cache size
            const int STRIPES = (int) cvCeil( (double) (size.width - 2*r) /
                    (CACHE_SIZE / sizeof(Histogram) - 2*r) );
            const int STRIPE_SIZE = (int) cvCeil(
                    (double) ( size.width + STRIPES*2*r - 2*r ) / STRIPES );

            for( int i = 0; i < size.width; i += STRIPE_SIZE - 2*r )
            {
                int stripe = STRIPE_SIZE;
                // Make sure that the filter kernel fits into one stripe.
                if( i + STRIPE_SIZE - 2*r >= size.width ||
                    size.width - (i + STRIPE_SIZE - 2*r) < 2*r+1 )
                    stripe = size.width - i;

                IPPI_CALL( icvMedianBlur_8u_CnR_O1( src->data.ptr + cn*i, src->step,
                    dst->data.ptr + cn*i, dst->step, cvSize(stripe, size.height),
                    param1, cn, i == 0, stripe == size.width - i ));

                if( stripe == size.width - i )
                    break;
            }
        }
    }
    else if( smooth_type == CV_GAUSSIAN )
    {
        CvSize ksize = { param1, param2 };
        float* kx = (float*)cvStackAlloc( ksize.width*sizeof(kx[0]) );
        float* ky = (float*)cvStackAlloc( ksize.height*sizeof(ky[0]) );
        CvMat KX = cvMat( 1, ksize.width, CV_32F, kx );
        CvMat KY = cvMat( 1, ksize.height, CV_32F, ky );
        
        CvSepFilter::init_gaussian_kernel( &KX, sigma1 );
        if( ksize.width != ksize.height || fabs(sigma1 - sigma2) > FLT_EPSILON )
            CvSepFilter::init_gaussian_kernel( &KY, sigma2 );
        else
            KY.data.fl = kx;
        
        if( have_ipp && size.width >= param1*3 &&
            size.height >= param2 && param1 > 1 && param2 > 1 )
        {
            int done;
            CV_CALL( done = icvIPPSepFilter( src, dst, &KX, &KY,
                        cvPoint(ksize.width/2,ksize.height/2)));
            if( done )
                EXIT;
        }

        CV_CALL( gaussian_filter.init( src->cols, src_type, dst_type, &KX, &KY ));
        CV_CALL( gaussian_filter.process( src, dst ));
    }
    else if( smooth_type == CV_BILATERAL )
    {
        if( param2 != 0 && (param2 != param1 || param1 % 2 == 0) )
            CV_ERROR( CV_StsBadSize, "Bilateral filter only supports square windows of odd size" );
        
        switch( src_type )
        {
        case CV_32FC1:
        case CV_32FC3:
            CV_CALL( icvBilateralFiltering_32f( src, dst, param1, param3, param4 ));
            break;
        case CV_8UC1:
        case CV_8UC3:
            CV_CALL( icvBilateralFiltering_8u( src, dst, param1, param3, param4 ));
            break;
        default:
            CV_ERROR( CV_StsUnsupportedFormat,
                "Unknown/unsupported format: bilateral filter only supports 8uC1, 8uC3, 32fC1 and 32fC3 formats" );
        }
    }

    __END__;

    cvReleaseMat( &temp );
}

/* End of file. */
