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

#include "_cxcore.h"

/****************************************************************************************\
*                                         cvGEMM                                         *
\****************************************************************************************/

icvBLAS_GEMM_32f_t icvBLAS_GEMM_32f_p = 0;
icvBLAS_GEMM_64f_t icvBLAS_GEMM_64f_p = 0;
icvBLAS_GEMM_32fc_t icvBLAS_GEMM_32fc_p = 0;
icvBLAS_GEMM_64fc_t icvBLAS_GEMM_64fc_p = 0;

static void
icvGEMM_CopyBlock( const uchar* src, int src_step,
                   uchar* dst, int dst_step,
                   CvSize size, int pix_size )
{
    int j;
    size.width = size.width * (pix_size / sizeof(int));

    for( ; size.height--; src += src_step, dst += dst_step )
    {
        for( j = 0; j <= size.width - 4; j += 4 )
        {
            int t0 = ((const int*)src)[j];
            int t1 = ((const int*)src)[j+1];
            ((int*)dst)[j] = t0;
            ((int*)dst)[j+1] = t1;
            t0 = ((const int*)src)[j+2];
            t1 = ((const int*)src)[j+3];
            ((int*)dst)[j+2] = t0;
            ((int*)dst)[j+3] = t1;
        }

        for( ; j < size.width; j++ )
            ((int*)dst)[j] = ((const int*)src)[j];
    }
}


static void
icvGEMM_TransposeBlock( const uchar* src, int src_step,
                        uchar* dst, int dst_step,
                        CvSize size, int pix_size )
{
    int i, j;
    for( i = 0; i < size.width; i++, dst += dst_step, src += pix_size )
    {
        const uchar* _src = src;
        switch( pix_size )
        {
        case sizeof(int):
            for( j = 0; j < size.height; j++, _src += src_step )
                ((int*)dst)[j] = ((int*)_src)[0];
            break;
        case sizeof(int)*2:
            for( j = 0; j < size.height*2; j += 2, _src += src_step )
            {
                int t0 = ((int*)_src)[0];
                int t1 = ((int*)_src)[1];
                ((int*)dst)[j] = t0;
                ((int*)dst)[j+1] = t1;
            }
            break;
        case sizeof(int)*4:
            for( j = 0; j < size.height*4; j += 4, _src += src_step )
            {
                int t0 = ((int*)_src)[0];
                int t1 = ((int*)_src)[1];
                ((int*)dst)[j] = t0;
                ((int*)dst)[j+1] = t1;
                t0 = ((int*)_src)[2];
                t1 = ((int*)_src)[3];
                ((int*)dst)[j+2] = t0;
                ((int*)dst)[j+3] = t1;
            }
            break;
        default:
            assert(0);
            return;
        }
    }
}

#define ICV_DEF_GEMM_SINGLE_MUL( flavor, arrtype, worktype )                \
static CvStatus CV_STDCALL                                                  \
icvGEMMSingleMul_##flavor( const arrtype* a_data, size_t a_step,            \
                         const arrtype* b_data, size_t b_step,              \
                         const arrtype* c_data, size_t c_step,              \
                         arrtype* d_data, size_t d_step,                    \
                         CvSize a_size, CvSize d_size,                      \
                         double alpha, double beta, int flags )             \
{                                                                           \
    int i, j, k, n = a_size.width, m = d_size.width, drows = d_size.height; \
    const arrtype *_a_data = a_data, *_b_data = b_data, *_c_data = c_data;  \
    arrtype* a_buf = 0;                                                     \
    size_t a_step0, a_step1, c_step0, c_step1, t_step;                      \
                                                                            \
    a_step /= sizeof(a_data[0]);                                            \
    b_step /= sizeof(b_data[0]);                                            \
    c_step /= sizeof(c_data[0]);                                            \
    d_step /= sizeof(d_data[0]);                                            \
    a_step0 = a_step;                                                       \
    a_step1 = 1;                                                            \
                                                                            \
    if( !c_data )                                                           \
        c_step0 = c_step1 = 0;                                              \
    else if( !(flags & CV_GEMM_C_T) )                                       \
        c_step0 = c_step, c_step1 = 1;                                      \
    else                                                                    \
        c_step0 = 1, c_step1 = c_step;                                      \
                                                                            \
    if( flags & CV_GEMM_A_T )                                               \
    {                                                                       \
        CV_SWAP( a_step0, a_step1, t_step );                                \
        n = a_size.height;                                                  \
        if( a_step > 1 && n > 1 )                                           \
            a_buf = (arrtype*)cvStackAlloc(n*sizeof(a_data[0]));            \
    }                                                                       \
                                                                            \
    if( n == 1 ) /* external product */                                     \
    {                                                                       \
        arrtype* b_buf = 0;                                                 \
                                                                            \
        if( a_step > 1 )                                                    \
        {                                                                   \
            a_buf = (arrtype*)cvStackAlloc(drows*sizeof(a_data[0]));        \
            for( k = 0; k < drows; k++ )                                    \
                a_buf[k] = a_data[a_step*k];                                \
            a_data = a_buf;                                                 \
        }                                                                   \
                                                                            \
        if( b_step > 1 )                                                    \
        {                                                                   \
            b_buf = (arrtype*)cvStackAlloc(d_size.width*sizeof(b_buf[0]) ); \
            for( j = 0; j < d_size.width; j++ )                             \
                b_buf[j] = b_data[j*b_step];                                \
            b_data = b_buf;                                                 \
        }                                                                   \
                                                                            \
        for( i = 0; i < drows; i++, _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            worktype al = worktype(a_data[i])*alpha;                        \
            c_data = _c_data;                                               \
            for( j = 0; j <= d_size.width - 2; j += 2, c_data += 2*c_step1 )\
            {                                                               \
                worktype s0 = al*b_data[j];                                 \
                worktype s1 = al*b_data[j+1];                               \
                if( !c_data )                                               \
                {                                                           \
                    d_data[j] = arrtype(s0);                                \
                    d_data[j+1] = arrtype(s1);                              \
                }                                                           \
                else                                                        \
                {                                                           \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
                    d_data[j+1] = arrtype(s1 + c_data[c_step1]*beta);       \
                }                                                           \
            }                                                               \
                                                                            \
            for( ; j < d_size.width; j++, c_data += c_step1 )               \
            {                                                               \
                worktype s0 = al*b_data[j];                                 \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else if( flags & CV_GEMM_B_T ) /* A * Bt */                             \
    {                                                                       \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                    _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            a_data = _a_data;                                               \
            b_data = _b_data;                                               \
            c_data = _c_data;                                               \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = a_data[a_step1*k];                           \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j < d_size.width; j++, b_data += b_step,            \
                                               c_data += c_step1 )          \
            {                                                               \
                worktype s0(0), s1(0), s2(0), s3(0);                        \
                                                                            \
                for( k = 0; k <= n - 4; k += 4 )                            \
                {                                                           \
                    s0 += worktype(a_data[k])*b_data[k];                    \
                    s1 += worktype(a_data[k+1])*b_data[k+1];                \
                    s2 += worktype(a_data[k+2])*b_data[k+2];                \
                    s3 += worktype(a_data[k+3])*b_data[k+3];                \
                }                                                           \
                                                                            \
                for( ; k < n; k++ )                                         \
                    s0 += worktype(a_data[k])*b_data[k];                    \
                s0 = (s0+s1+s2+s3)*alpha;                                   \
                                                                            \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else if( d_size.width*sizeof(d_data[0]) <= 1600 )                       \
    {                                                                       \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                    _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            a_data = _a_data, c_data = _c_data;                             \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = a_data[a_step1*k];                           \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j <= m - 4; j += 4, c_data += 4*c_step1 )           \
            {                                                               \
                const arrtype* b = _b_data + j;                             \
                worktype s0(0), s1(0), s2(0), s3(0);                        \
                                                                            \
                for( k = 0; k < n; k++, b += b_step )                       \
                {                                                           \
                    worktype a(a_data[k]);                                  \
                    s0 += a * b[0]; s1 += a * b[1];                         \
                    s2 += a * b[2]; s3 += a * b[3];                         \
                }                                                           \
                                                                            \
                if( !c_data )                                               \
                {                                                           \
                    d_data[j] = arrtype(s0*alpha);                          \
                    d_data[j+1] = arrtype(s1*alpha);                        \
                    d_data[j+2] = arrtype(s2*alpha);                        \
                    d_data[j+3] = arrtype(s3*alpha);                        \
                }                                                           \
                else                                                        \
                {                                                           \
                    s0 = s0*alpha; s1 = s1*alpha;                           \
                    s2 = s2*alpha; s3 = s3*alpha;                           \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
                    d_data[j+1] = arrtype(s1 + c_data[c_step1]*beta);       \
                    d_data[j+2] = arrtype(s2 + c_data[c_step1*2]*beta);     \
                    d_data[j+3] = arrtype(s3 + c_data[c_step1*3]*beta);     \
                }                                                           \
            }                                                               \
                                                                            \
            for( ; j < m; j++, c_data += c_step1 )                          \
            {                                                               \
                const arrtype* b = _b_data + j;                             \
                worktype s0(0);                                             \
                                                                            \
                for( k = 0; k < n; k++, b += b_step )                       \
                    s0 += worktype(a_data[k]) * b[0];                       \
                                                                            \
                s0 = s0*alpha;                                              \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        worktype* d_buf = (worktype*)cvStackAlloc(m*sizeof(d_buf[0]));      \
                                                                            \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                            _c_data += c_step0,             \
                                            d_data += d_step )              \
        {                                                                   \
            a_data = _a_data;                                               \
            b_data = _b_data;                                               \
            c_data = _c_data;                                               \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = _a_data[a_step1*k];                          \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j < m; j++ )                                        \
                d_buf[j] = worktype(0);                                     \
                                                                            \
            for( k = 0; k < n; k++, b_data += b_step )                      \
            {                                                               \
                worktype al(a_data[k]);                                     \
                                                                            \
                for( j = 0; j <= m - 4; j += 4 )                            \
                {                                                           \
                    worktype t0 = d_buf[j] + b_data[j]*al;                  \
                    worktype t1 = d_buf[j+1] + b_data[j+1]*al;              \
                    d_buf[j] = t0;                                          \
                    d_buf[j+1] = t1;                                        \
                    t0 = d_buf[j+2] + b_data[j+2]*al;                       \
                    t1 = d_buf[j+3] + b_data[j+3]*al;                       \
                    d_buf[j+2] = t0;                                        \
                    d_buf[j+3] = t1;                                        \
                }                                                           \
                                                                            \
                for( ; j < m; j++ )                                         \
                    d_buf[j] += b_data[j]*al;                               \
            }                                                               \
                                                                            \
            if( !c_data )                                                   \
                for( j = 0; j < m; j++ )                                    \
                    d_data[j] = arrtype(d_buf[j]*alpha);                    \
            else                                                            \
                for( j = 0; j < m; j++, c_data += c_step1 )                 \
                {                                                           \
                    worktype t = d_buf[j]*alpha;                            \
                    d_data[j] = arrtype(t + c_data[0]*beta);                \
                }                                                           \
        }                                                                   \
    }                                                                       \
    return CV_OK;                                                           \
}


#define ICV_DEF_GEMM_BLOCK_MUL( flavor, arrtype, worktype )         \
static CvStatus CV_STDCALL                                          \
icvGEMMBlockMul_##flavor( const arrtype* a_data, size_t a_step,     \
                        const arrtype* b_data, size_t b_step,       \
                        worktype* d_data, size_t d_step,            \
                        CvSize a_size, CvSize d_size, int flags )   \
{                                                                   \
    int i, j, k, n = a_size.width, m = d_size.width;                \
    const arrtype *_a_data = a_data, *_b_data = b_data;             \
    arrtype* a_buf = 0;                                             \
    size_t a_step0, a_step1, t_step;                                \
    int do_acc = flags & 16;                                        \
                                                                    \
    a_step /= sizeof(a_data[0]);                                    \
    b_step /= sizeof(b_data[0]);                                    \
    d_step /= sizeof(d_data[0]);                                    \
                                                                    \
    a_step0 = a_step;                                               \
    a_step1 = 1;                                                    \
                                                                    \
    if( flags & CV_GEMM_A_T )                                       \
    {                                                               \
        CV_SWAP( a_step0, a_step1, t_step );                        \
        n = a_size.height;                                          \
        a_buf = (arrtype*)cvStackAlloc(n*sizeof(a_data[0]));        \
    }                                                               \
                                                                    \
    if( flags & CV_GEMM_B_T )                                       \
    {                                                               \
        /* second operand is transposed */                          \
        for( i = 0; i < d_size.height; i++, _a_data += a_step0,     \
                                            d_data += d_step )      \
        {                                                           \
            a_data = _a_data; b_data = _b_data;                     \
                                                                    \
            if( a_buf )                                             \
            {                                                       \
                for( k = 0; k < n; k++ )                            \
                    a_buf[k] = a_data[a_step1*k];                   \
                a_data = a_buf;                                     \
            }                                                       \
                                                                    \
            for( j = 0; j < d_size.width; j++, b_data += b_step )   \
            {                                                       \
                worktype s0 = do_acc ? d_data[j]:worktype(0), s1(0);\
                for( k = 0; k <= n - 2; k += 2 )                    \
                {                                                   \
                    s0 += worktype(a_data[k])*b_data[k];            \
                    s1 += worktype(a_data[k+1])*b_data[k+1];        \
                }                                                   \
                                                                    \
                for( ; k < n; k++ )                                 \
                    s0 += worktype(a_data[k])*b_data[k];            \
                                                                    \
                d_data[j] = s0 + s1;                                \
            }                                                       \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        for( i = 0; i < d_size.height; i++, _a_data += a_step0,     \
                                            d_data += d_step )      \
        {                                                           \
            a_data = _a_data, b_data = _b_data;                     \
                                                                    \
            if( a_buf )                                             \
            {                                                       \
                for( k = 0; k < n; k++ )                            \
                    a_buf[k] = a_data[a_step1*k];                   \
                a_data = a_buf;                                     \
            }                                                       \
                                                                    \
            for( j = 0; j <= m - 4; j += 4 )                        \
            {                                                       \
                worktype s0, s1, s2, s3;                            \
                const arrtype* b = b_data + j;                      \
                                                                    \
                if( do_acc )                                        \
                {                                                   \
                    s0 = d_data[j]; s1 = d_data[j+1];               \
                    s2 = d_data[j+2]; s3 = d_data[j+3];             \
                }                                                   \
                else                                                \
                    s0 = s1 = s2 = s3 = worktype(0);                \
                                                                    \
                for( k = 0; k < n; k++, b += b_step )               \
                {                                                   \
                    worktype a(a_data[k]);                          \
                    s0 += a * b[0]; s1 += a * b[1];                 \
                    s2 += a * b[2]; s3 += a * b[3];                 \
                }                                                   \
                                                                    \
                d_data[j] = s0; d_data[j+1] = s1;                   \
                d_data[j+2] = s2; d_data[j+3] = s3;                 \
            }                                                       \
                                                                    \
            for( ; j < m; j++ )                                     \
            {                                                       \
                const arrtype* b = b_data + j;                      \
                worktype s0 = do_acc ? d_data[j] : worktype(0);     \
                                                                    \
                for( k = 0; k < n; k++, b += b_step )               \
                    s0 += worktype(a_data[k]) * b[0];               \
                                                                    \
                d_data[j] = s0;                                     \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}


#define ICV_DEF_GEMM_STORE( flavor, arrtype, worktype )             \
static CvStatus CV_STDCALL                                          \
icvGEMMStore_##flavor( const arrtype* c_data, size_t c_step,        \
                       const worktype* d_buf, size_t d_buf_step,    \
                       arrtype* d_data, size_t d_step, CvSize d_size,\
                       double alpha, double beta, int flags )       \
{                                                                   \
    const arrtype* _c_data = c_data;                                \
    int j;                                                          \
    size_t c_step0, c_step1;                                        \
                                                                    \
    c_step /= sizeof(c_data[0]);                                    \
    d_buf_step /= sizeof(d_buf[0]);                                 \
    d_step /= sizeof(d_data[0]);                                    \
                                                                    \
    if( !c_data )                                                   \
        c_step0 = c_step1 = 0;                                      \
    else if( !(flags & CV_GEMM_C_T) )                               \
        c_step0 = c_step, c_step1 = 1;                              \
    else                                                            \
        c_step0 = 1, c_step1 = c_step;                              \
                                                                    \
    for( ; d_size.height--; _c_data += c_step0,                     \
                            d_buf += d_buf_step,                    \
                            d_data += d_step )                      \
    {                                                               \
        if( _c_data )                                               \
        {                                                           \
            c_data = _c_data;                                       \
            for( j = 0; j <= d_size.width - 4; j += 4, c_data += 4*c_step1 )\
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                worktype t1 = alpha*d_buf[j+1];                     \
                t0 += beta*worktype(c_data[0]);                     \
                t1 += beta*worktype(c_data[c_step1]);               \
                d_data[j] = arrtype(t0);                            \
                d_data[j+1] = arrtype(t1);                          \
                t0 = alpha*d_buf[j+2];                              \
                t1 = alpha*d_buf[j+3];                              \
                t0 += beta*worktype(c_data[c_step1*2]);             \
                t1 += beta*worktype(c_data[c_step1*3]);             \
                d_data[j+2] = arrtype(t0);                          \
                d_data[j+3] = arrtype(t1);                          \
            }                                                       \
            for( ; j < d_size.width; j++, c_data += c_step1 )       \
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                d_data[j] = arrtype(t0 + beta*c_data[0]);           \
            }                                                       \
        }                                                           \
        else                                                        \
        {                                                           \
            for( j = 0; j <= d_size.width - 4; j += 4 )             \
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                worktype t1 = alpha*d_buf[j+1];                     \
                d_data[j] = arrtype(t0);                            \
                d_data[j+1] = arrtype(t1);                          \
                t0 = alpha*d_buf[j+2];                              \
                t1 = alpha*d_buf[j+3];                              \
                d_data[j+2] = arrtype(t0);                          \
                d_data[j+3] = arrtype(t1);                          \
            }                                                       \
            for( ; j < d_size.width; j++ )                          \
                d_data[j] = arrtype(alpha*d_buf[j]);                \
        }                                                           \
    }                                                               \
    return CV_OK;                                                   \
}


ICV_DEF_GEMM_SINGLE_MUL( 32f_C1R, float, double)
ICV_DEF_GEMM_BLOCK_MUL( 32f_C1R, float, double)
ICV_DEF_GEMM_STORE( 32f_C1R, float, double)

ICV_DEF_GEMM_SINGLE_MUL( 64f_C1R, double, double)
ICV_DEF_GEMM_BLOCK_MUL( 64f_C1R, double, double)
ICV_DEF_GEMM_STORE( 64f_C1R, double, double)

ICV_DEF_GEMM_SINGLE_MUL( 32f_C2R, CvComplex32f, CvComplex64f)
ICV_DEF_GEMM_BLOCK_MUL( 32f_C2R, CvComplex32f, CvComplex64f)
ICV_DEF_GEMM_STORE( 32f_C2R, CvComplex32f, CvComplex64f)

ICV_DEF_GEMM_SINGLE_MUL( 64f_C2R, CvComplex64f, CvComplex64f)
ICV_DEF_GEMM_BLOCK_MUL( 64f_C2R, CvComplex64f, CvComplex64f)
ICV_DEF_GEMM_STORE( 64f_C2R, CvComplex64f, CvComplex64f)

typedef CvStatus (CV_STDCALL *CvGEMMSingleMulFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, const void* src3, size_t step3,
                   void* dst, size_t dststep, CvSize srcsize, CvSize dstsize,
                   double alpha, double beta, int flags );

typedef CvStatus (CV_STDCALL *CvGEMMBlockMulFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, void* dst, size_t dststep,
                   CvSize srcsize, CvSize dstsize, int flags );

typedef CvStatus (CV_STDCALL *CvGEMMStoreFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, void* dst, size_t dststep,
                   CvSize dstsize, double alpha, double beta, int flags );


static void icvInitGEMMTable( CvBigFuncTable* single_mul_tab,
                              CvBigFuncTable* block_mul_tab,
                              CvBigFuncTable* store_tab )
{
    single_mul_tab->fn_2d[CV_32FC1] = (void*)icvGEMMSingleMul_32f_C1R;
    single_mul_tab->fn_2d[CV_64FC1] = (void*)icvGEMMSingleMul_64f_C1R;
    single_mul_tab->fn_2d[CV_32FC2] = (void*)icvGEMMSingleMul_32f_C2R;
    single_mul_tab->fn_2d[CV_64FC2] = (void*)icvGEMMSingleMul_64f_C2R;
    block_mul_tab->fn_2d[CV_32FC1] = (void*)icvGEMMBlockMul_32f_C1R;
    block_mul_tab->fn_2d[CV_64FC1] = (void*)icvGEMMBlockMul_64f_C1R;
    block_mul_tab->fn_2d[CV_32FC2] = (void*)icvGEMMBlockMul_32f_C2R;
    block_mul_tab->fn_2d[CV_64FC2] = (void*)icvGEMMBlockMul_64f_C2R;
    store_tab->fn_2d[CV_32FC1] = (void*)icvGEMMStore_32f_C1R;
    store_tab->fn_2d[CV_64FC1] = (void*)icvGEMMStore_64f_C1R;
    store_tab->fn_2d[CV_32FC2] = (void*)icvGEMMStore_32f_C2R;
    store_tab->fn_2d[CV_64FC2] = (void*)icvGEMMStore_64f_C2R;
}


CV_IMPL void
cvGEMM( const CvArr* Aarr, const CvArr* Barr, double alpha,
        const CvArr* Carr, double beta, CvArr* Darr, int flags )
{
    const int block_lin_size = 128;
    const int block_size = block_lin_size * block_lin_size;

    static CvBigFuncTable single_mul_tab, block_mul_tab, store_tab;
    static int inittab = 0;
    static double zero[] = {0,0,0,0};
    static float zerof[] = {0,0,0,0};
    
    uchar* buffer = 0;
    int local_alloc = 0;
    uchar* block_buffer = 0;

    CV_FUNCNAME( "cvGEMM" );

    __BEGIN__;

    CvMat *A = (CvMat*)Aarr;
    CvMat *B = (CvMat*)Barr;
    CvMat *C = (CvMat*)Carr;
    CvMat *D = (CvMat*)Darr;
    int len = 0;
    
    CvMat stub, stub1, stub2, stub3;
    CvSize a_size, d_size;
    int type;

    if( !CV_IS_MAT( A ))
    {
        int coi = 0;
        CV_CALL( A = cvGetMat( A, &stub1, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( B ))
    {
        int coi = 0;
        CV_CALL( B = cvGetMat( B, &stub2, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( D ))
    {
        int coi = 0;
        CV_CALL( D = cvGetMat( D, &stub, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( beta == 0 )
        C = 0;

    if( C )
    {
        if( !CV_IS_MAT( C ))
        {
            int coi = 0;
            CV_CALL( C = cvGetMat( C, &stub3, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }

        if( !CV_ARE_TYPES_EQ( C, D ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( ((flags&CV_GEMM_C_T) == 0 && (C->cols != D->cols || C->rows != D->rows)) ||
            ((flags&CV_GEMM_C_T) != 0 && (C->rows != D->cols || C->cols != D->rows)))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        if( (flags & CV_GEMM_C_T) != 0 && C->data.ptr == D->data.ptr )
        {
            cvTranspose( C, D );
            C = D;
            flags &= ~CV_GEMM_C_T;
        }
    }
    else
    {
        C = &stub3;
        C->data.ptr = 0;
        C->step = 0;
        C->type = CV_MAT_CONT_FLAG;
    }

    type = CV_MAT_TYPE(A->type);
    if( !CV_ARE_TYPES_EQ( A, B ) || !CV_ARE_TYPES_EQ( A, D ) )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    a_size.width = A->cols;
    a_size.height = A->rows;
    d_size.width = D->cols;
    d_size.height = D->rows;

    switch( flags & (CV_GEMM_A_T|CV_GEMM_B_T) )
    {
    case 0:
        len = B->rows;
        if( a_size.width != len ||
            B->cols != d_size.width ||
            a_size.height != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 1:
        len = B->rows;
        if( a_size.height != len ||
            B->cols != d_size.width ||
            a_size.width != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 2:
        len = B->cols;
        if( a_size.width != len ||
            B->rows != d_size.width ||
            a_size.height != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 3:
        len = B->cols;
        if( a_size.height != len ||
            B->rows != d_size.width ||
            a_size.width != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    }

    if( flags == 0 && 2 <= len && len <= 4 && (len == d_size.width || len == d_size.height) )
    {
        int i;
        if( type == CV_64F )
        {
            double* d = D->data.db;
            const double *a = A->data.db, *b = B->data.db, *c = C->data.db;
            size_t d_step = D->step/sizeof(d[0]),
                   a_step = A->step/sizeof(a[0]),
                   b_step = B->step/sizeof(b[0]),
                   c_step = C->step/sizeof(c[0]);

            if( !c )
                c = zero;

            switch( len )
            {
            case 2:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step];
                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                    }
                }
                else
                    break;
                EXIT;
            case 3:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1];
                        double t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                        d[2] = t2*alpha + c[2]*beta;
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] + a[a_step+2]*b[b_step*2];
                        double t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] + a[a_step*2+2]*b[b_step*2];

                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                        d[d_step*2] = t2*alpha + c[c_step*2]*beta;
                    }
                }
                else
                    break;
                EXIT;
            case 4:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1] + a[3]*b[b_step*3+1];
                        double t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2] + a[3]*b[b_step*3+2];
                        double t3 = a[0]*b[3] + a[1]*b[b_step+3] + a[2]*b[b_step*2+3] + a[3]*b[b_step*3+3];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                        d[2] = t2*alpha + c[2]*beta;
                        d[3] = t3*alpha + c[3]*beta;
                    }
                }
                else if( d_size.width <= 16 && a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] +
                                    a[a_step+2]*b[b_step*2] + a[a_step+3]*b[b_step*3];
                        double t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] +
                                    a[a_step*2+2]*b[b_step*2] + a[a_step*2+3]*b[b_step*3];
                        double t3 = a[a_step*3]*b[0] + a[a_step*3+1]*b[b_step] +
                                    a[a_step*3+2]*b[b_step*2] + a[a_step*3+3]*b[b_step*3];
                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                        d[d_step*2] = t2*alpha + c[c_step*2]*beta;
                        d[d_step*3] = t3*alpha + c[c_step*3]*beta;
                    }
                }
                else
                    break;
                EXIT;
            }
        }

        if( type == CV_32F )
        {
            float* d = D->data.fl;
            const float *a = A->data.fl, *b = B->data.fl, *c = C->data.fl;
            size_t d_step = D->step/sizeof(d[0]),
                   a_step = A->step/sizeof(a[0]),
                   b_step = B->step/sizeof(b[0]),
                   c_step = C->step/sizeof(c[0]);

            if( !c )
                c = zerof;

            switch( len )
            {
            case 2:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                    }
                }
                else
                    break;
                EXIT;
            case 3:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1];
                        float t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                        d[2] = (float)(t2*alpha + c[2]*beta);
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] + a[a_step+2]*b[b_step*2];
                        float t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] + a[a_step*2+2]*b[b_step*2];

                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                        d[d_step*2] = (float)(t2*alpha + c[c_step*2]*beta);
                    }
                }
                else
                    break;
                EXIT;
            case 4:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1] + a[3]*b[b_step*3+1];
                        float t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2] + a[3]*b[b_step*3+2];
                        float t3 = a[0]*b[3] + a[1]*b[b_step+3] + a[2]*b[b_step*2+3] + a[3]*b[b_step*3+3];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                        d[2] = (float)(t2*alpha + c[2]*beta);
                        d[3] = (float)(t3*alpha + c[3]*beta);
                    }
                }
                else if( len <= 16 && a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] +
                                   a[a_step+2]*b[b_step*2] + a[a_step+3]*b[b_step*3];
                        float t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] +
                                   a[a_step*2+2]*b[b_step*2] + a[a_step*2+3]*b[b_step*3];
                        float t3 = a[a_step*3]*b[0] + a[a_step*3+1]*b[b_step] +
                                   a[a_step*3+2]*b[b_step*2] + a[a_step*3+3]*b[b_step*3];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                        d[d_step*2] = (float)(t2*alpha + c[c_step*2]*beta);
                        d[d_step*3] = (float)(t3*alpha + c[c_step*3]*beta);
                    }
                }
                else
                    break;
                EXIT;
            }
        }
    }
 
    {
        int b_step = B->step;
        CvGEMMSingleMulFunc single_mul_func;
        CvMat tmat, *D0 = D;
        icvBLAS_GEMM_32f_t blas_func = 0;

        if( !inittab )
        {
            icvInitGEMMTable( &single_mul_tab, &block_mul_tab, &store_tab );
            inittab = 1;
        }

        single_mul_func = (CvGEMMSingleMulFunc)single_mul_tab.fn_2d[type];
        if( !single_mul_func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( D->data.ptr == A->data.ptr || D->data.ptr == B->data.ptr )
        {
            int buf_size = d_size.width*d_size.height*CV_ELEM_SIZE(type);
            if( d_size.width <= CV_MAX_LOCAL_MAT_SIZE )
            {
                buffer = (uchar*)cvStackAlloc( buf_size );
                local_alloc = 1;
            }
            else
                CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));

            tmat = cvMat( d_size.height, d_size.width, type, buffer );
            D = &tmat;
        }

        if( (d_size.width == 1 || len == 1) && !(flags & CV_GEMM_B_T) && CV_IS_MAT_CONT(B->type) )
        {
            b_step = d_size.width == 1 ? 0 : CV_ELEM_SIZE(type);
            flags |= CV_GEMM_B_T;
        }

        if( (d_size.width | d_size.height | len) >= 16 && icvBLAS_GEMM_32f_p != 0 )
        {
            blas_func = type == CV_32FC1 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_32f_p :
                        type == CV_64FC1 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_64f_p :
                        type == CV_32FC2 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_32fc_p :
                        type == CV_64FC2 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_64fc_p : 0;
        }

        if( blas_func )
        {
            const char* transa = flags & CV_GEMM_A_T ? "t" : "n";
            const char* transb = flags & CV_GEMM_B_T ? "t" : "n";
            int lda, ldb, ldd;
            
            if( C->data.ptr )
            {
                if( C->data.ptr != D->data.ptr )
                {
                    if( !(flags & CV_GEMM_C_T) )
                        cvCopy( C, D );
                    else
                        cvTranspose( C, D );
                }
            }

            if( CV_MAT_DEPTH(type) == CV_32F )
            {
                CvComplex32f _alpha, _beta;
                
                lda = A->step/sizeof(float);
                ldb = b_step/sizeof(float);
                ldd = D->step/sizeof(float);
                _alpha.re = (float)alpha;
                _alpha.im = 0;
                _beta.re = C->data.ptr ? (float)beta : 0;
                _beta.im = 0;
                if( CV_MAT_CN(type) == 2 )
                    lda /= 2, ldb /= 2, ldd /= 2;

                blas_func( transb, transa, &d_size.width, &d_size.height, &len,
                       &_alpha, B->data.ptr, &ldb, A->data.ptr, &lda,
                       &_beta, D->data.ptr, &ldd );
            }
            else
            {
                CvComplex64f _alpha, _beta;
                
                lda = A->step/sizeof(double);
                ldb = b_step/sizeof(double);
                ldd = D->step/sizeof(double);
                _alpha.re = alpha;
                _alpha.im = 0;
                _beta.re = C->data.ptr ? beta : 0;
                _beta.im = 0;
                if( CV_MAT_CN(type) == 2 )
                    lda /= 2, ldb /= 2, ldd /= 2;

                blas_func( transb, transa, &d_size.width, &d_size.height, &len,
                       &_alpha, B->data.ptr, &ldb, A->data.ptr, &lda,
                       &_beta, D->data.ptr, &ldd );
            }
        }
        else if( ((d_size.height <= block_lin_size/2 || d_size.width <= block_lin_size/2) &&
            len <= 10000) || len <= 10 ||
            (d_size.width <= block_lin_size && d_size.height <= block_lin_size && len <= block_lin_size) )
        {
            single_mul_func( A->data.ptr, A->step, B->data.ptr, b_step,
                             C->data.ptr, C->step, D->data.ptr, D->step,
                             a_size, d_size, alpha, beta, flags );
        }
        else
        {
            int is_a_t = flags & CV_GEMM_A_T;
            int is_b_t = flags & CV_GEMM_B_T;
            int elem_size = CV_ELEM_SIZE(type);
            int dk0_1, dk0_2;
            int a_buf_size = 0, b_buf_size, d_buf_size;
            uchar* a_buf = 0;
            uchar* b_buf = 0;
            uchar* d_buf = 0;
            int i, j, k, di = 0, dj = 0, dk = 0;
            int dm0, dn0, dk0;
            int a_step0, a_step1, b_step0, b_step1, c_step0, c_step1;
            int work_elem_size = elem_size << (CV_MAT_DEPTH(type) == CV_32F ? 1 : 0);
            CvGEMMBlockMulFunc block_mul_func = (CvGEMMBlockMulFunc)block_mul_tab.fn_2d[type];
            CvGEMMStoreFunc store_func = (CvGEMMStoreFunc)store_tab.fn_2d[type];

            assert( block_mul_func && store_func );

            if( !is_a_t )
                a_step0 = A->step, a_step1 = elem_size;
            else
                a_step0 = elem_size, a_step1 = A->step;

            if( !is_b_t )
                b_step0 = b_step, b_step1 = elem_size;
            else
                b_step0 = elem_size, b_step1 = b_step;

            if( !C->data.ptr )
            {
                c_step0 = c_step1 = 0;
                flags &= ~CV_GEMM_C_T;
            }
            else if( !(flags & CV_GEMM_C_T) )
                c_step0 = C->step, c_step1 = elem_size;
            else
                c_step0 = elem_size, c_step1 = C->step;

            dm0 = MIN( block_lin_size, d_size.height );
            dn0 = MIN( block_lin_size, d_size.width );
            dk0_1 = block_size / dm0;
            dk0_2 = block_size / dn0;
            dk0 = MAX( dk0_1, dk0_2 );
            dk0 = MIN( dk0, len );
            if( dk0*dm0 > block_size )
                dm0 = block_size / dk0;
            if( dk0*dn0 > block_size )
                dn0 = block_size / dk0;

            dk0_1 = (dn0+dn0/8+2) & -2;
            b_buf_size = (dk0+dk0/8+1)*dk0_1*elem_size;
            d_buf_size = (dk0+dk0/8+1)*dk0_1*work_elem_size;
        
            if( is_a_t )
            {
                a_buf_size = (dm0+dm0/8+1)*((dk0+dk0/8+2)&-2)*elem_size;
                flags &= ~CV_GEMM_A_T;
            }

            CV_CALL( block_buffer = (uchar*)cvAlloc(a_buf_size + b_buf_size + d_buf_size));
            d_buf = block_buffer;
            b_buf = d_buf + d_buf_size;

            if( is_a_t )
                a_buf = b_buf + b_buf_size;

            for( i = 0; i < d_size.height; i += di )
            {
                di = dm0;
                if( i + di >= d_size.height || 8*(i + di) + di > 8*d_size.height )
                    di = d_size.height - i;

                for( j = 0; j < d_size.width; j += dj )
                {
                    uchar* _d = D->data.ptr + i*D->step + j*elem_size;
                    const uchar* _c = C->data.ptr + i*c_step0 + j*c_step1;
                    int _d_step = D->step;
                    dj = dn0;

                    if( j + dj >= d_size.width || 8*(j + dj) + dj > 8*d_size.width )
                        dj = d_size.width - j;

                    flags &= 15;
                    if( dk0 < len )
                    {
                        _d = d_buf;
                        _d_step = dj*work_elem_size;
                    }

                    for( k = 0; k < len; k += dk )
                    {
                        const uchar* _a = A->data.ptr + i*a_step0 + k*a_step1;
                        int _a_step = A->step;
                        const uchar* _b = B->data.ptr + k*b_step0 + j*b_step1;
                        int _b_step = b_step;
                        CvSize a_bl_size;

                        dk = dk0;
                        if( k + dk >= len || 8*(k + dk) + dk > 8*len )
                            dk = len - k;

                        if( !is_a_t )
                            a_bl_size.width = dk, a_bl_size.height = di;
                        else
                            a_bl_size.width = di, a_bl_size.height = dk;

                        if( a_buf && is_a_t )
                        {
                            int t;
                            _a_step = dk*elem_size;
                            icvGEMM_TransposeBlock( _a, A->step, a_buf, _a_step, a_bl_size, elem_size );
                            CV_SWAP( a_bl_size.width, a_bl_size.height, t );
                            _a = a_buf;
                        }

                        if( dj < d_size.width )
                        {
                            CvSize b_size;
                            if( !is_b_t )
                                b_size.width = dj, b_size.height = dk;
                            else
                                b_size.width = dk, b_size.height = dj;

                            _b_step = b_size.width*elem_size;
                            icvGEMM_CopyBlock( _b, b_step, b_buf, _b_step, b_size, elem_size );
                            _b = b_buf;
                        }

                        if( dk0 < len )
                            block_mul_func( _a, _a_step, _b, _b_step, _d, _d_step,
                                            a_bl_size, cvSize(dj,di), flags );
                        else
                            single_mul_func( _a, _a_step, _b, _b_step, _c, C->step, _d, _d_step,
                                             a_bl_size, cvSize(dj,di), alpha, beta, flags );
                        flags |= 16;
                    }

                    if( dk0 < len )
                        store_func( _c, C->step, _d, _d_step, D->data.ptr + i*D->step + j*elem_size,
                                    D->step, cvSize(dj,di), alpha, beta, flags );
                }
            }
        }

        if( D0 != D )
            CV_CALL( cvCopy( D, D0 ));
    }

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
    if( block_buffer )
        cvFree( &block_buffer );
}


/****************************************************************************************\
*                                        cvTransform                                     *
\****************************************************************************************/

#define  ICV_DEF_TRANSFORM_CASE_C1( arrtype, temptype, _ld_,        \
                                   _cast_macro1_, _cast_macro2_ )   \
{                                                                   \
    for( i = 0; i < size.width; i++, dst += dst_cn )                \
    {                                                               \
        const double* _mat = mat;                                   \
        double v0 = _ld_(src[i]);                                   \
        for( k = 0; k < dst_cn; k++, _mat += 2 )                    \
        {                                                           \
            temptype t0 = _cast_macro1_(_mat[0]*v0 + _mat[1]);      \
            dst[k] = _cast_macro2_(t0);                             \
        }                                                           \
    }                                                               \
    src += size.width;                                              \
}


#define  ICV_DEF_DIAG_TRANSFORM_CASE_C1( arrtype, temptype, _ld_,   \
                                  _cast_macro1_, _cast_macro2_ )    \
    for( i = 0; i < size.width; i++ )                               \
    {                                                               \
        double ft0;                                                 \
        temptype t0;                                                \
        ft0 = mat[0]*_ld_(src[i]) + mat[1];                         \
        t0 = _cast_macro1_(ft0);                                    \
        dst[i] = _cast_macro2_(t0);                                 \
    }


#define  ICV_DEF_TRANSFORM_CASE_C2( arrtype, temptype, _ld_,        \
                                  _cast_macro1_, _cast_macro2_ )    \
if( dst_cn == 2 )                                                   \
{                                                                   \
    for( i = 0; i < size.width*2; i += 2 )                          \
    {                                                               \
        double ft0, ft1;                                            \
        temptype t0, t1;                                            \
        ft0 = mat[0]*_ld_(src[i]) + mat[1]*_ld_(src[i+1]) + mat[2]; \
        ft1 = mat[3]*_ld_(src[i]) + mat[4]*_ld_(src[i+1]) + mat[5]; \
        t0 = _cast_macro1_(ft0);                                    \
        t1 = _cast_macro1_(ft1);                                    \
        dst[i] = _cast_macro2_(t0);                                 \
        dst[i+1] = _cast_macro2_(t1);                               \
    }                                                               \
    src += size.width*2; dst += size.width*2;                       \
}                                                                   \
else                                                                \
    for( i = 0; i < size.width; i++, src += 2, dst += dst_cn )      \
    {                                                               \
        const double* _mat = mat;                                   \
        double v0 = _ld_(src[0]), v1 = src[1];                      \
        for( k = 0; k < dst_cn; k++, _mat += 3 )                    \
        {                                                           \
            temptype t0 =                                           \
                _cast_macro1_(_mat[0]*v0 + _mat[1]*v1 + _mat[2]);   \
            dst[k] = _cast_macro2_(t0);                             \
        }                                                           \
    }


#define  ICV_DEF_DIAG_TRANSFORM_CASE_C2( arrtype, temptype, _ld_,   \
                                  _cast_macro1_, _cast_macro2_ )    \
    for( i = 0; i < size.width*2; i += 2 )                          \
    {                                                               \
        double ft0, ft1;                                            \
        temptype t0, t1;                                            \
        ft0 = mat[0]*_ld_(src[i]) + mat[2];                         \
        ft1 = mat[4]*_ld_(src[i+1]) + mat[5];                       \
        t0 = _cast_macro1_(ft0);                                    \
        t1 = _cast_macro1_(ft1);                                    \
        dst[i] = _cast_macro2_(t0);                                 \
        dst[i+1] = _cast_macro2_(t1);                               \
    }


#define  ICV_DEF_TRANSFORM_CASE_C3( arrtype, temptype, _ld_,        \
                                  _cast_macro1_, _cast_macro2_ )    \
if( dst_cn == 3 )                                                   \
{                                                                   \
    for( i = 0; i < size.width*3; i += 3 )                          \
    {                                                               \
        double ft0, ft1, ft2;                                       \
        temptype t0, t1, t2;                                        \
        ft0 = mat[0]*_ld_(src[i]) + mat[1]*_ld_(src[i+1]) +         \
              mat[2]*_ld_(src[i+2]) + mat[3];                       \
        ft1 = mat[4]*_ld_(src[i]) + mat[5]*_ld_(src[i+1]) +         \
              mat[6]*_ld_(src[i+2]) + mat[7];                       \
        ft2 = mat[8]*_ld_(src[i]) + mat[9]*_ld_(src[i+1]) +         \
              mat[10]*_ld_(src[i+2]) + mat[11];                     \
        t0 = _cast_macro1_(ft0);                                    \
        t1 = _cast_macro1_(ft1);                                    \
        t2 = _cast_macro1_(ft2);                                    \
        dst[i] = _cast_macro2_(t0);                                 \
        dst[i+1] = _cast_macro2_(t1);                               \
        dst[i+2] = _cast_macro2_(t2);                               \
    }                                                               \
    src += size.width*3; dst += size.width*3;                       \
}                                                                   \
else if( dst_cn == 1 )                                              \
{                                                                   \
    for( i = 0; i < size.width; i++, src += 3 )                     \
    {                                                               \
        temptype t0 = _cast_macro1_(mat[0]*_ld_(src[0]) +           \
            mat[1]*_ld_(src[1]) + mat[2]*_ld_(src[2]) + mat[3]);    \
        dst[i] = _cast_macro2_(t0);                                 \
    }                                                               \
    dst += size.width;                                              \
}                                                                   \
else                                                                \
    for( i = 0; i < size.width; i++, src += 3, dst += dst_cn )      \
    {                                                               \
        const double* _mat = mat;                                   \
        double v0=_ld_(src[0]), v1=_ld_(src[1]), v2=_ld_(src[2]);   \
        for( k = 0; k < dst_cn; k++, _mat += 4 )                    \
        {                                                           \
            temptype t0 = _cast_macro1_(_mat[0]*v0 +                \
                    _mat[1]*v1 + _mat[2]*v2 + _mat[3]);             \
            dst[k] = _cast_macro2_(t0);                             \
        }                                                           \
    }


#define  ICV_DEF_DIAG_TRANSFORM_CASE_C3( arrtype, temptype, _ld_,   \
                                  _cast_macro1_, _cast_macro2_ )    \
    for( i = 0; i < size.width*3; i += 3 )                          \
    {                                                               \
        double ft0, ft1, ft2;                                       \
        temptype t0, t1, t2;                                        \
        ft0 = mat[0]*_ld_(src[i]) + mat[3];                         \
        ft1 = mat[5]*_ld_(src[i+1]) + mat[7];                       \
        ft2 = mat[10]*_ld_(src[i+2]) + mat[11];                     \
        t0 = _cast_macro1_(ft0);                                    \
        t1 = _cast_macro1_(ft1);                                    \
        t2 = _cast_macro1_(ft2);                                    \
        dst[i] = _cast_macro2_(t0);                                 \
        dst[i+1] = _cast_macro2_(t1);                               \
        dst[i+2] = _cast_macro2_(t2);                               \
    }


#define  ICV_DEF_TRANSFORM_CASE_C4( arrtype, temptype, _ld_,        \
                                  _cast_macro1_, _cast_macro2_ )    \
for( i = 0; i < size.width; i++, src += 4, dst += dst_cn )          \
{                                                                   \
    const double* _mat = mat;                                       \
    double v0 = _ld_(src[0]), v1 = _ld_(src[1]),                    \
           v2 = _ld_(src[2]), v3 = _ld_(src[3]);                    \
    for( k = 0; k < dst_cn; k++, _mat += 5 )                        \
    {                                                               \
        temptype t0 =_cast_macro1_(_mat[0]*v0+_mat[1]*v1+           \
                                   _mat[2]*v2+_mat[3]*v3+_mat[4]);  \
        dst[k] = _cast_macro2_(t0);                                 \
    }                                                               \
}


#define  ICV_DEF_DIAG_TRANSFORM_CASE_C4( arrtype, temptype, _ld_,   \
                                  _cast_macro1_, _cast_macro2_ )    \
    for( i = 0; i < size.width*4; i += 4 )                          \
    {                                                               \
        double ft0, ft1;                                            \
        temptype t0, t1;                                            \
        ft0 = mat[0]*_ld_(src[i]) + mat[4];                         \
        ft1 = mat[6]*_ld_(src[i+1]) + mat[9];                       \
        t0 = _cast_macro1_(ft0);                                    \
        t1 = _cast_macro1_(ft1);                                    \
        dst[i] = _cast_macro2_(t0);                                 \
        dst[i+1] = _cast_macro2_(t1);                               \
        ft0 = mat[12]*_ld_(src[i+2]) + mat[14];                     \
        ft1 = mat[18]*_ld_(src[i+3]) + mat[19];                     \
        t0 = _cast_macro1_(ft0);                                    \
        t1 = _cast_macro1_(ft1);                                    \
        dst[i+2] = _cast_macro2_(t0);                               \
        dst[i+3] = _cast_macro2_(t1);                               \
    }



#define  ICV_DEF_TRANSFORM_FUNC( flavor, arrtype, temptype, _ld_,   \
                                 _cast_macro1_, _cast_macro2_, cn  )\
static CvStatus CV_STDCALL                                          \
icvTransform_##flavor( const arrtype* src, int srcstep,             \
                       arrtype* dst, int dststep, CvSize size,      \
                       const double* mat, int dst_cn )              \
{                                                                   \
    srcstep = srcstep/sizeof(src[0]) - size.width*cn;               \
    dststep = dststep/sizeof(dst[0]) - size.width*dst_cn;           \
    for( ; size.height--; src += srcstep, dst += dststep )          \
    {                                                               \
        int i, k;                                                   \
        ICV_DEF_TRANSFORM_CASE_C##cn( arrtype, temptype, _ld_,      \
                                     _cast_macro1_, _cast_macro2_ ) \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}


#define  ICV_DEF_DIAG_TRANSFORM_FUNC( flavor, arrtype, temptype, _ld_, \
                                 _cast_macro1_, _cast_macro2_, cn  )\
static CvStatus CV_STDCALL                                          \
icvDiagTransform_##flavor( const arrtype* src, int srcstep,         \
                       arrtype* dst, int dststep, CvSize size,      \
                       const double* mat )                          \
{                                                                   \
    srcstep /= sizeof(src[0]);                                      \
    dststep /= sizeof(dst[0]);                                      \
    for( ; size.height--; src += srcstep, dst += dststep )          \
    {                                                               \
        int i;                                                      \
        ICV_DEF_DIAG_TRANSFORM_CASE_C##cn( arrtype, temptype, _ld_, \
                                     _cast_macro1_, _cast_macro2_ ) \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}


ICV_DEF_TRANSFORM_FUNC( 8u_C1R, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U, 1 )
ICV_DEF_TRANSFORM_FUNC( 8u_C2R, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U, 2 )
ICV_DEF_TRANSFORM_FUNC( 8u_C3R, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U, 3 )
ICV_DEF_TRANSFORM_FUNC( 8u_C4R, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U, 4 )

ICV_DEF_TRANSFORM_FUNC( 16u_C1R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 1 )
ICV_DEF_TRANSFORM_FUNC( 16u_C2R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 2 )
ICV_DEF_TRANSFORM_FUNC( 16u_C3R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 3 )
ICV_DEF_TRANSFORM_FUNC( 16u_C4R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 4 )

ICV_DEF_TRANSFORM_FUNC( 16s_C1R, short, int, CV_NOP, cvRound, CV_CAST_16S, 1 )
ICV_DEF_TRANSFORM_FUNC( 16s_C2R, short, int, CV_NOP, cvRound, CV_CAST_16S, 2 )
ICV_DEF_TRANSFORM_FUNC( 16s_C3R, short, int, CV_NOP, cvRound, CV_CAST_16S, 3 )
ICV_DEF_TRANSFORM_FUNC( 16s_C4R, short, int, CV_NOP, cvRound, CV_CAST_16S, 4 )

ICV_DEF_TRANSFORM_FUNC( 32s_C1R, int, int, CV_NOP, cvRound, CV_NOP, 1 )
ICV_DEF_TRANSFORM_FUNC( 32s_C2R, int, int, CV_NOP, cvRound, CV_NOP, 2 )
ICV_DEF_TRANSFORM_FUNC( 32s_C3R, int, int, CV_NOP, cvRound, CV_NOP, 3 )
ICV_DEF_TRANSFORM_FUNC( 32s_C4R, int, int, CV_NOP, cvRound, CV_NOP, 4 )

ICV_DEF_TRANSFORM_FUNC( 32f_C1R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 1 )
ICV_DEF_TRANSFORM_FUNC( 32f_C2R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 2 )
ICV_DEF_TRANSFORM_FUNC( 32f_C3R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 3 )
ICV_DEF_TRANSFORM_FUNC( 32f_C4R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 4 )

ICV_DEF_TRANSFORM_FUNC( 64f_C1R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 1 )
ICV_DEF_TRANSFORM_FUNC( 64f_C2R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 2 )
ICV_DEF_TRANSFORM_FUNC( 64f_C3R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 3 )
ICV_DEF_TRANSFORM_FUNC( 64f_C4R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 4 )

ICV_DEF_DIAG_TRANSFORM_FUNC( 16u_C1R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 1 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 16u_C2R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 2 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 16u_C3R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 3 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 16u_C4R, ushort, int, CV_NOP, cvRound, CV_CAST_16U, 4 )

ICV_DEF_DIAG_TRANSFORM_FUNC( 16s_C1R, short, int, CV_NOP, cvRound, CV_CAST_16S, 1 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 16s_C2R, short, int, CV_NOP, cvRound, CV_CAST_16S, 2 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 16s_C3R, short, int, CV_NOP, cvRound, CV_CAST_16S, 3 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 16s_C4R, short, int, CV_NOP, cvRound, CV_CAST_16S, 4 )

ICV_DEF_DIAG_TRANSFORM_FUNC( 32s_C1R, int, int, CV_NOP, cvRound, CV_NOP, 1 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 32s_C2R, int, int, CV_NOP, cvRound, CV_NOP, 2 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 32s_C3R, int, int, CV_NOP, cvRound, CV_NOP, 3 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 32s_C4R, int, int, CV_NOP, cvRound, CV_NOP, 4 )

ICV_DEF_DIAG_TRANSFORM_FUNC( 32f_C1R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 1 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 32f_C2R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 2 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 32f_C3R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 3 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 32f_C4R, float, double, CV_NOP, CV_NOP, CV_CAST_32F, 4 )

ICV_DEF_DIAG_TRANSFORM_FUNC( 64f_C1R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 1 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 64f_C2R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 2 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 64f_C3R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 3 )
ICV_DEF_DIAG_TRANSFORM_FUNC( 64f_C4R, double, double, CV_NOP, CV_NOP, CV_CAST_64F, 4 )

#define icvTransform_8s_C1R 0
#define icvTransform_8s_C2R 0
#define icvTransform_8s_C3R 0
#define icvTransform_8s_C4R 0

#define icvDiagTransform_8s_C1R 0
#define icvDiagTransform_8s_C2R 0
#define icvDiagTransform_8s_C3R 0
#define icvDiagTransform_8s_C4R 0

#define icvDiagTransform_8u_C1R 0
#define icvDiagTransform_8u_C2R 0
#define icvDiagTransform_8u_C3R 0
#define icvDiagTransform_8u_C4R 0

CV_DEF_INIT_BIG_FUNC_TAB_2D( Transform, R )
CV_DEF_INIT_BIG_FUNC_TAB_2D( DiagTransform, R )

typedef CvStatus (CV_STDCALL * CvTransformFunc)(
                       const void* src, int srcstep,
                       void* dst, int dststep, CvSize size,
                       const void* mat, int dst_cn );

typedef CvStatus (CV_STDCALL * CvDiagTransformFunc)(
                       const void* src, int srcstep,
                       void* dst, int dststep, CvSize size,
                       const void* mat );

typedef CvStatus (CV_STDCALL * CvDiagTransformFunc)(
                       const void* src, int srcstep,
                       void* dst, int dststep, CvSize size,
                       const void* mat );

///////////////////// IPP transform functions //////////////////

icvColorTwist_8u_C3R_t icvColorTwist_8u_C3R_p = 0;
icvColorTwist_16u_C3R_t icvColorTwist_16u_C3R_p = 0;
icvColorTwist_16s_C3R_t icvColorTwist_16s_C3R_p = 0;
icvColorTwist_32f_C3R_t icvColorTwist_32f_C3R_p = 0;
icvColorTwist_32f_C4R_t icvColorTwist_32f_C4R_p = 0;

icvColorToGray_8u_C3C1R_t icvColorToGray_8u_C3C1R_p = 0;
icvColorToGray_16u_C3C1R_t icvColorToGray_16u_C3C1R_p = 0;
icvColorToGray_16s_C3C1R_t icvColorToGray_16s_C3C1R_p = 0;
icvColorToGray_32f_C3C1R_t icvColorToGray_32f_C3C1R_p = 0;

icvColorToGray_8u_AC4C1R_t icvColorToGray_8u_AC4C1R_p = 0;
icvColorToGray_16u_AC4C1R_t icvColorToGray_16u_AC4C1R_p = 0;
icvColorToGray_16s_AC4C1R_t icvColorToGray_16s_AC4C1R_p = 0;
icvColorToGray_32f_AC4C1R_t icvColorToGray_32f_AC4C1R_p = 0;

typedef CvStatus (CV_STDCALL * CvColorTwistIPPFunc)( const void* src, int srcstep,
                        void* dst, int dststep, CvSize size, const float* coeffs );

////////////////////////////////////////////////////////////////

CV_IMPL void
cvTransform( const CvArr* srcarr, CvArr* dstarr,
             const CvMat* transmat, const CvMat* shiftvec )
{
    static CvBigFuncTable transform_tab, diag_transform_tab;
    static int inittab = 0;
    CvMat* lut = 0;
    
    CV_FUNCNAME( "cvTransform" );

    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat rotstub, *rot = (CvMat*)transmat;
    CvMat shiftstub, *shift = (CvMat*)shiftvec;
    CvSeq *src_seq = 0, *dst_seq = 0;
    CvSeq hdr; // need only one copy of stub header & seqblock (either for src or dst)
    CvSeqBlock block_hdr;
    int i, j, type, cn, dst_cn;
    int coi = 0, coi2 = 0;
    double* buffer = (double*)cvStackAlloc( CV_CN_MAX*(CV_CN_MAX+1)*sizeof(buffer[0]) );

    if( !inittab )
    {
        icvInitTransformRTable( &transform_tab );
        icvInitDiagTransformRTable( &diag_transform_tab );
        inittab = 1;
    }

    if( CV_IS_SEQ( src ))
    {
        src_seq = (CvSeq*)src;
        if( CV_ELEM_SIZE(src_seq->flags) != src_seq->elem_size )
            CV_ERROR( CV_StsUnsupportedFormat, "Unsupported type of sequence elements" );
    }
    else
        CV_CALL( src = cvGetMat( src, &srcstub, &coi ));

    if( CV_IS_SEQ( dst ))
    {
        dst_seq = (CvSeq*)dst;
        if( CV_ELEM_SIZE(dst_seq->flags) != dst_seq->elem_size )
            CV_ERROR( CV_StsUnsupportedFormat, "Unsupported type of sequence elements" );
    }
    else
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

    if( coi != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_DEPTHS_EQ(src, dst) )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( src_seq || dst_seq )
    {
        if( !src_seq )
        {
            if( CV_IS_MAT_CONT(src->type) || (src->rows != 1 && src->cols != 1) )
                CV_ERROR( CV_StsBadSize, "if eigher the source or destination is a sequence, "
                "the other array must be also a sequence of continous 1d vector" );
            src_seq = cvMakeSeqHeaderForArray( CV_MAT_TYPE(src->type), sizeof(hdr),
                                       CV_ELEM_SIZE(src->type), src->data.ptr,
                                       src->rows + src->cols + 1, &hdr, &block_hdr );
        }

        if( !dst_seq )
        {
            if( CV_IS_MAT_CONT(dst->type) || (dst->rows != 1 && dst->cols != 1) )
                CV_ERROR( CV_StsBadSize, "if eigher the source or destination is a sequence, "
                "the other array must be also a sequence of continous 1d vector" );
            if( dst->rows + dst->cols - 1 != src_seq->total )
                CV_ERROR( CV_StsUnmatchedFormats,
                "source sequence and destination vector have different sizes" );
            dst_seq = cvMakeSeqHeaderForArray( CV_MAT_TYPE(dst->type), sizeof(hdr),
                                           CV_ELEM_SIZE(dst->type), dst->data.ptr,
                                           dst->rows + dst->cols + 1, &hdr, &block_hdr );
        }
        else if( dst_seq->total != src_seq->total )
        {
            if( dst_seq->total > src_seq->total )
                cvSeqPopMulti( dst_seq, 0, dst_seq->total - src_seq->total );
            else
                cvSeqPushMulti( dst_seq, 0, src_seq->total - dst_seq->total );
        }
    }
    else if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    type = CV_MAT_TYPE( src->type );
    cn = CV_MAT_CN( type );
    dst_cn = CV_MAT_CN( dst->type );

    if( cn > 4 || dst_cn > 4 )
        CV_ERROR( CV_StsOutOfRange, "Both input and output array must have at most 4 channels" );

    if( !CV_IS_MAT( rot ))
        CV_CALL( rot = cvGetMat( rot, &rotstub, &coi ));

    if( rot->rows != dst_cn )
        CV_ERROR( CV_StsBadSize,
        "The height of transmat matrix must be equal to number of channels" );

    if( rot->cols == cn + 1 || rot->cols == cn )
    {
        if( CV_MAT_TYPE( rot->type ) == CV_64FC1 )
        {
            for( i = 0; i < dst_cn; i++ )
            {
                buffer[i*(cn+1) + cn] = 0;
                for( j = 0; j < rot->cols; j++ )
                    buffer[i*(cn+1) + j] = ((double*)(rot->data.ptr + rot->step*i))[j];
            }
        }
        else if( CV_MAT_TYPE( rot->type ) == CV_32FC1 )
        {
            for( i = 0; i < dst_cn; i++ )
            {
                buffer[i*(cn+1) + cn] = 0;
                for( j = 0; j < rot->cols; j++ )
                    buffer[i*(cn+1) + j] = ((float*)(rot->data.ptr + rot->step*i))[j];
            }
        }
        else
            CV_ERROR( CV_StsUnsupportedFormat, "Rotation matrix must be 32fC1 or 64fC1" );
    }
    else
        CV_ERROR( CV_StsUnmatchedSizes, "If the source array has <cn> channels, "
           "the transformation matrix must have <cn> x <cn>+1 or <cn> x <cn> size" );

    if( shift )
    {
        if( !CV_IS_MAT( shift ))
            CV_CALL( shift = cvGetMat( shift, &shiftstub, &coi ));

        if( CV_MAT_CN( shift->type ) * shift->cols * shift->rows == dst_cn &&
            (shift->rows == 1 || shift->cols == 1) )
        {
            if( CV_MAT_DEPTH( shift->type ) == CV_64F )
            {
                int step = shift->step ? shift->step/sizeof(double) : 1;
                for( i = 0; i < dst_cn; i++ )
                    buffer[i*(cn+1) + cn] += shift->data.db[i*step];
            }
            else if( CV_MAT_DEPTH( shift->type ) == CV_32F )
            {
                int step = shift->step ? shift->step/sizeof(float) : 1;
                for( i = 0; i < dst_cn; i++ )
                    buffer[i*(cn+1) + cn] += shift->data.fl[i*step];
            }
            else
                CV_ERROR( CV_StsUnsupportedFormat, "Shift vector must be 32f or 64f" );
        }
        else
        {
            CV_ERROR( CV_StsUnmatchedSizes,
                "Shift (if present) must be 1 dimensional vector with the number "
                "of elements equal to number of channels in the processed array" );
        }
    }

    if( coi != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    {
        CvTransformFunc func = (CvTransformFunc)(transform_tab.fn_2d[type]);
        CvDiagTransformFunc diag_func = 0;
        CvLUT_TransformFunc lut_func = 0;
        int diag_transform = 0;
        CvColorTwistIPPFunc ipp_func = 0;
        CvSize size;
        float* ipp_coeffs = (float*)cvStackAlloc( 16*sizeof(ipp_coeffs[0]) );

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( cn == dst_cn )
            ipp_func = type == CV_8UC3 ? icvColorTwist_8u_C3R_p :
                       type == CV_16UC3 ? icvColorTwist_16u_C3R_p :
                       type == CV_16SC3 ? icvColorTwist_16s_C3R_p :
                       type == CV_32FC3 ? icvColorTwist_32f_C3R_p :
                       type == CV_32FC4 && fabs(buffer[4]) < DBL_EPSILON &&
                       fabs(buffer[9]) < DBL_EPSILON && fabs(buffer[14]) < DBL_EPSILON &&
                       fabs(buffer[19]) < DBL_EPSILON ? icvColorTwist_32f_C4R_p : 0;
        else if( dst_cn == 1 && (cn == 3 || cn == 4) &&
                 buffer[0] >= 0 && buffer[1] >= 0 && buffer[2] >= 0 &&
                 buffer[0] + buffer[1] + buffer[2] <= 1.01 &&
                 fabs(buffer[3]) < DBL_EPSILON && (cn == 3 || fabs(buffer[4]) < DBL_EPSILON) )
        {
            if( cn == 3 )
                ipp_func = type == CV_8UC3 ? icvColorToGray_8u_C3C1R_p :
                           type == CV_16UC3 ? icvColorToGray_16u_C3C1R_p :
                           type == CV_16SC3 ? icvColorToGray_16s_C3C1R_p :
                           type == CV_32FC3 ? icvColorToGray_32f_C3C1R_p : 0;
            else
                ipp_func = type == CV_8UC4 ? icvColorToGray_8u_AC4C1R_p :
                           type == CV_16UC4 ? icvColorToGray_16u_AC4C1R_p :
                           type == CV_16SC4 ? icvColorToGray_16s_AC4C1R_p :
                           type == CV_32FC4 ? icvColorToGray_32f_AC4C1R_p : 0;
        }

        if( dst_cn == cn )
        {
            diag_transform = 1;
            for( i = 0; i < dst_cn; i++ )
                for( j = 0; j < cn; j++ )
                {
                    if( i != j && fabs(buffer[i*(cn+1) + j]) > DBL_EPSILON )
                    {
                        diag_transform = 0;
                        break;
                    }
                }
            
            if( diag_transform )
            {
                if( CV_MAT_DEPTH(type) == CV_8U )
                {
                    CV_CALL( lut = cvCreateMat( 1, 256, type ));
                    for( i = 0; i < cn; i++ )
                    {
                        double a = buffer[i*(cn+1) + i], b = buffer[i*(cn+1) + cn];
                        uchar* ltab = lut->data.ptr;
                        for( j = 0; j < 256; j++ )
                        {
                            int t = cvRound(a*j + b);
                            ltab[j*cn + i] = CV_CAST_8U(t);
                        }
                    }
                    lut_func = cn == 1 ? (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C1R :
                               cn == 2 ? (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C2R :
                               cn == 3 ? (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C3R :
                               (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C4R;
                }
                else
                    diag_func = (CvDiagTransformFunc)(diag_transform_tab.fn_2d[type]);
            }
        }

        if( ipp_func )
        {
            const double* ptr = buffer;

            // fill cn x 4 ipp_coeffs array
            for( i = 0; i < cn*4; i += 4, ptr += cn+1 )
            {
                float t0 = (float)ptr[0];
                float t1 = (float)ptr[1];
                ipp_coeffs[i] = t0;
                ipp_coeffs[i+1] = t1;
                t0 = (float)ptr[2];
                t1 = (float)ptr[3];
                ipp_coeffs[i+2] = t0;
                ipp_coeffs[i+3] = t1;
            }
        }

        if( !src_seq )
        {
            int srcstep = src->step;
            int dststep = dst->step;
            size = cvGetMatSize( src );
            
            if( CV_IS_MAT_CONT( src->type & dst->type ))
            {
                size.width *= size.height;
                size.height = 1;
                srcstep = dststep = CV_STUB_STEP;
            }
            
            if( lut_func )
                lut_func( src->data.ptr, src->step, dst->data.ptr,
                          dst->step, size, lut->data.ptr );
            else if( ipp_func )
            {
                IPPI_CALL( ipp_func( src->data.ptr, srcstep, dst->data.ptr,
                                     dststep, size, ipp_coeffs ));
            }
            else if( diag_transform )
                diag_func( src->data.ptr, src->step, dst->data.ptr,
                           dst->step, size, buffer );
            else
                func( src->data.ptr, src->step, dst->data.ptr,
                      dst->step, size, buffer, dst_cn );
        }
        else
        {
            CvSeqBlock* src_block = src_seq->first;
            CvSeqBlock* dst_block = dst_seq->first;
            int src_idx = 0, dst_idx = 0;
            int src_elem_size = CV_ELEM_SIZE(src_seq->flags);
            int dst_elem_size = CV_ELEM_SIZE(dst_seq->flags);

            for( i = src_seq->total; i > 0; )
            {
                int src_len = src_block->count - src_idx;
                int dst_len = dst_block->count - dst_idx;
                const void* srcptr = src_block->data + src_idx*src_elem_size;
                void* dstptr = dst_block->data + dst_idx*dst_elem_size;
                src_len = MIN(src_len, dst_len);

                if( lut_func )
                    lut_func( srcptr, CV_STUB_STEP, dstptr, CV_STUB_STEP,
                              cvSize( src_len, 1 ), lut->data.ptr );
                else if( ipp_func )
                {
                    IPPI_CALL( ipp_func( srcptr, CV_STUB_STEP, dstptr, CV_STUB_STEP,
                                         cvSize( src_len, 1 ), ipp_coeffs ));
                }
                else if( diag_transform )
                    diag_func( srcptr, CV_STUB_STEP, dstptr, CV_STUB_STEP,
                               cvSize( src_len, 1 ), buffer );
                else
                    func( srcptr, CV_STUB_STEP, dstptr, CV_STUB_STEP,
                          cvSize( src_len, 1 ), buffer, dst_cn );

                if( (src_idx += src_len) == src_block->count )
                    src_block = src_block->next, src_idx = 0;
                if( (dst_idx += src_len) == dst_block->count )
                    dst_block = dst_block->next, dst_idx = 0;
                i -= src_len;
            }
        }
    }

    __END__;

    cvReleaseMat( &lut );
}


/****************************************************************************************\
*                                        cvPerspectiveTransform                          *
\****************************************************************************************/

#define ICV_PERSPECTIVE_TRANSFORM_FUNC_2( flavor, arrtype )                             \
static CvStatus CV_STDCALL                                                              \
icvPerspectiveTransform_##flavor##_C2R( const arrtype* src, int srcstep,                \
                                        arrtype* dst, int dststep,                      \
                                        CvSize size, const double* mat )                \
{                                                                                       \
    int i;                                                                              \
    size.width *= 2;                                                                    \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);                               \
                                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )                              \
    {                                                                                   \
        for( i = 0; i < size.width; i += 2 )                                            \
        {                                                                               \
            arrtype x = src[i], y = src[i + 1];                                         \
            double w = x*mat[6] + y*mat[7] + mat[8];                                    \
                                                                                        \
            if( fabs(w) > FLT_EPSILON )                                                 \
            {                                                                           \
                w = 1./w;                                                               \
                dst[i] = (arrtype)((x*mat[0] + y*mat[1] + mat[2]) * w);                 \
                dst[i+1] = (arrtype)((x*mat[3] + y*mat[4] + mat[5]) * w);               \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                dst[i] = (arrtype)0;                                                    \
                dst[i+1] = (arrtype)0;                                                  \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}


#define ICV_PERSPECTIVE_TRANSFORM_FUNC_3( flavor, arrtype )                             \
static CvStatus CV_STDCALL                                                              \
icvPerspectiveTransform_##flavor##_C3R( const arrtype* src, int srcstep,                \
                                             arrtype* dst, int dststep,                 \
                                             CvSize size, const double* mat )           \
{                                                                                       \
    int i;                                                                              \
    size.width *= 3;                                                                    \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);                               \
                                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )                              \
    {                                                                                   \
        for( i = 0; i < size.width; i += 3 )                                            \
        {                                                                               \
            arrtype x = src[i], y = src[i + 1], z = src[i + 2];                         \
            double w = x*mat[12] + y*mat[13] + z*mat[14] + mat[15];                     \
                                                                                        \
            if( fabs(w) > FLT_EPSILON )                                                 \
            {                                                                           \
                w = 1./w;                                                               \
                dst[i] = (arrtype)((x*mat[0] + y*mat[1] + z*mat[2] + mat[3]) * w);      \
                dst[i+1] = (arrtype)((x*mat[4] + y*mat[5] + z*mat[6] + mat[7]) * w);    \
                dst[i+2] = (arrtype)((x*mat[8] + y*mat[9] + z*mat[10] + mat[11]) * w);  \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                dst[i] = (arrtype)0;                                                    \
                dst[i+1] = (arrtype)0;                                                  \
                dst[i+2] = (arrtype)0;                                                  \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}

ICV_PERSPECTIVE_TRANSFORM_FUNC_2( 32f, float )
ICV_PERSPECTIVE_TRANSFORM_FUNC_2( 64f, double )
ICV_PERSPECTIVE_TRANSFORM_FUNC_3( 32f, float )
ICV_PERSPECTIVE_TRANSFORM_FUNC_3( 64f, double )

static void icvInitPerspectiveTransformTable( CvFuncTable* tab2, CvFuncTable* tab3 )\
{                                                                                   \
    tab2->fn_2d[CV_32F] = (void*)icvPerspectiveTransform_32f_C2R;                   \
    tab2->fn_2d[CV_64F] = (void*)icvPerspectiveTransform_64f_C2R;                   \
    tab3->fn_2d[CV_32F] = (void*)icvPerspectiveTransform_32f_C3R;                   \
    tab3->fn_2d[CV_64F] = (void*)icvPerspectiveTransform_64f_C3R;                   \
}


CV_IMPL void
cvPerspectiveTransform( const CvArr* srcarr, CvArr* dstarr, const CvMat* mat )
{
    static CvFuncTable tab[2];
    static int inittab = 0;
    double buffer[16];

    CV_FUNCNAME( "cvPerspectiveProject" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    int i, j, type, cn;
    CvFunc2D_2A1P func = 0;
    CvSize size;

    if( !inittab )
    {
        icvInitPerspectiveTransformTable( &tab[0], &tab[1] );
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( dst ))
    {
        int coi = 0;
        CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    type = CV_MAT_TYPE( src->type );
    cn = CV_MAT_CN( type );

    if( cn != 2 && cn != 3 )
        CV_ERROR( CV_BadNumChannels, cvUnsupportedFormat );

    if( !CV_IS_MAT( mat ))
        CV_ERROR( CV_StsBadArg, "Invalid transformation matrix" );

    if( mat->rows != cn + 1 && mat->cols != mat->rows )
        CV_ERROR( CV_StsBadSize,
        "The size of transform matrix must be equal to number of channels" );

    if( CV_MAT_TYPE( mat->type ) == CV_64FC1 )
    {
        for( i = 0; i <= cn; i++ )
        {
            for( j = 0; j <= cn; j++ )
                buffer[i*(cn+1) + j] = ((double*)(mat->data.ptr + mat->step*i))[j];
        }
    }
    else if( CV_MAT_TYPE( mat->type ) == CV_32FC1 )
    {
        for( i = 0; i <= cn; i++ )
        {
            for( j = 0; j <= cn; j++ )
                buffer[i*(cn+1) + j] = ((float*)(mat->data.ptr + mat->step*i))[j];
        }
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "Rotation matrix must be 32fC1 or 64fC1" );
    }

    func = (CvFunc2D_2A1P)tab[cn == 3].fn_2d[CV_MAT_DEPTH(type)];

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    size = cvGetMatSize( src );

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        size.height = 1;
    }

    IPPI_CALL( func( src->data.ptr, src->step, dst->data.ptr, dst->step, size, buffer));

    CV_CHECK_NANS( dst );

    __END__;
}


/****************************************************************************************\
*                                       cvScaleAdd                                       *
\****************************************************************************************/

#define  ICV_DEF_MULADDC_CASE_C1( arrtype, temptype, src1, src2, dst, len )     \
{                                                                               \
    int i;                                                                      \
                                                                                \
    for( i = 0; i <= (len) - 4; i += 4 )                                        \
    {                                                                           \
        temptype t0 = (src1)[i]*s0 + (src2)[i];                                 \
        temptype t1 = (src1)[i+1]*s0 + (src2)[i+1];                             \
                                                                                \
        (dst)[i] = (arrtype)t0;                                                 \
        (dst)[i+1] = (arrtype)t1;                                               \
                                                                                \
        t0 = (src1)[i+2]*s0 + (src2)[i+2];                                      \
        t1 = (src1)[i+3]*s0 + (src2)[i+3];                                      \
                                                                                \
        (dst)[i+2] = (arrtype)t0;                                               \
        (dst)[i+3] = (arrtype)t1;                                               \
    }                                                                           \
                                                                                \
    for( ; i < (len); i++ )                                                     \
    {                                                                           \
        temptype t0 = (src1)[i]*s0 + (src2)[i];                                 \
        (dst)[i] = (arrtype)t0;                                                 \
    }                                                                           \
}


#define  ICV_DEF_MULADDC_CASE_C2( arrtype, temptype, src1, src2, dst, len )     \
{                                                                               \
    int i;                                                                      \
                                                                                \
    for( i = 0; i <= (len) - 4; i += 4 )                                        \
    {                                                                           \
        temptype t0 = (src1)[i]*s0 - (src1)[i+1]*s1 + (src2)[i];                \
        temptype t1 = (src1)[i]*s1 + (src1)[i+1]*s0 + (src2)[i+1];              \
                                                                                \
        (dst)[i] = (arrtype)t0;                                                 \
        (dst)[i+1] = (arrtype)t1;                                               \
                                                                                \
        t0 = (src1)[i+2]*s0 - (src1)[i+3]*s1 + (src2)[i+2];                     \
        t1 = (src1)[i+2]*s1 + (src1)[i+3]*s0 + (src2)[i+3];                     \
                                                                                \
        (dst)[i+2] = (arrtype)t0;                                               \
        (dst)[i+3] = (arrtype)t1;                                               \
    }                                                                           \
                                                                                \
    for( ; i < (len); i += 2 )                                                  \
    {                                                                           \
        temptype t0 = (src1)[i]*s0 - (src1)[i+1]*s1 + (src2)[i];                \
        temptype t1 = (src1)[i]*s1 + (src1)[i+1]*s0 + (src2)[i+1];              \
                                                                                \
        (dst)[i] = (arrtype)t0;                                                 \
        (dst)[i+1] = (arrtype)t1;                                               \
    }                                                                           \
}


#define  ICV_DEF_MULADDS_FUNC( flavor, arrtype, scalartype, entry, cn )     \
static CvStatus CV_STDCALL                                                  \
icvMulAddC_##flavor( const arrtype* src1, int srcstep1,                     \
                      const arrtype* src2, int srcstep2,                    \
                      arrtype* dst, int dststep, CvSize size,               \
                      const scalartype* scalar )                            \
{                                                                           \
    entry(scalartype);                                                      \
    size.width *= (cn);                                                     \
    srcstep1 /= sizeof(src1[0]); srcstep2 /= sizeof(src2[0]);               \
    dststep /= sizeof(dst[0]);                                              \
                                                                            \
    for( ; size.height--; src1+=srcstep1, src2+=srcstep2, dst+=dststep )    \
    {                                                                       \
        ICV_DEF_MULADDC_CASE_C##cn( arrtype, scalartype, src1, src2,        \
                                    dst, size.width )                       \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


ICV_DEF_MULADDS_FUNC( 32f_C1R, float, double, CV_UN_ENTRY_C1, 1 )
ICV_DEF_MULADDS_FUNC( 32f_C2R, float, double, CV_UN_ENTRY_C2, 2 )
ICV_DEF_MULADDS_FUNC( 64f_C1R, double, double, CV_UN_ENTRY_C1, 1 )
ICV_DEF_MULADDS_FUNC( 64f_C2R, double, double, CV_UN_ENTRY_C2, 2 )


static void
icvInitMulAddCTable( CvBigFuncTable* tab )
{
    tab->fn_2d[CV_32FC1] = (void*)icvMulAddC_32f_C1R;
    tab->fn_2d[CV_32FC2] = (void*)icvMulAddC_32f_C2R;
    tab->fn_2d[CV_64FC1] = (void*)icvMulAddC_64f_C1R;
    tab->fn_2d[CV_64FC2] = (void*)icvMulAddC_64f_C2R;
}


CV_IMPL void
cvScaleAdd( const CvArr* srcarr1, CvScalar scale,
            const CvArr* srcarr2, CvArr* dstarr )
{
    static CvBigFuncTable muladds_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvScaleAdd" );

    __BEGIN__;

    CvMat stub1, *src1 = (CvMat*)srcarr1;
    CvMat stub2, *src2 = (CvMat*)srcarr2;
    CvMat stub, *dst = (CvMat*)dstarr;
    CvSize size;
    int type;

    if( !CV_IS_MAT( src1 ) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        int coi1 = 0, coi2 = 0, coi3 = 0;
        CV_CALL( src1 = cvGetMat( src1, &stub1, &coi1 ));
        CV_CALL( src2 = cvGetMat( src2, &stub2, &coi2 ));
        CV_CALL( dst = cvGetMat( dst, &stub, &coi3 ));

        if( coi1 + coi2 + coi3 != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_ARE_TYPES_EQ( src1, dst ) || !CV_ARE_TYPES_EQ( src2, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( src1, dst ) || !CV_ARE_SIZES_EQ( src2, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    type = CV_MAT_TYPE( src1->type );
    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
    {
        size.width *= size.height;

        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
        {
            if( type == CV_32FC1 )
            {
                float* mA = src1->data.fl;
                float* mB = src2->data.fl;
                float* mC = dst->data.fl;

                do
                {
                    mC[size.width - 1] = (float)(mA[size.width - 1]*scale.val[0] +
                                         mB[size.width - 1]);
                }
                while( --size.width );

                EXIT;
            }

            if( type == CV_64FC1 )
            {
                double* mA = src1->data.db;
                double* mB = src2->data.db;
                double* mC = dst->data.db;

                do
                {
                    mC[size.width - 1] = mA[size.width - 1]*scale.val[0] +
                                         mB[size.width - 1];
                }
                while( --size.width );

                EXIT;
            }
        }

        size.height = 1;
    }

    if( !inittab )
    {
        icvInitMulAddCTable( &muladds_tab );
        inittab = 1;
    }

    if( CV_MAT_CN(type) > 2 )
        CV_ERROR( CV_StsOutOfRange, "The function only supports 1- and 2-channel arrays" );

    {
        CvFunc2D_3A1P func = (CvFunc2D_3A1P)(muladds_tab.fn_2d[type]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src1->data.ptr, src1->step, src2->data.ptr, src2->step,
                         dst->data.ptr, dst->step, size, scale.val ));
    }

    CV_CHECK_NANS( dst );

    __END__;
}


/****************************************************************************************\
*                                    cvCalcCovarMatrix                                   *
\****************************************************************************************/

#define ICV_DOT_PRODUCT_CASE( flavor, srctype, avgtype, load_macro )                    \
static CvStatus CV_STDCALL                                                              \
icvDotProductShifted_##flavor##_C1R( const srctype* vec1, int vecstep1,                 \
                                     const srctype* vec2, int vecstep2,                 \
                                     const avgtype* avg, int avgstep,                   \
                                     CvSize size, double* _result )                     \
{                                                                                       \
    double result = 0;                                                                  \
    vecstep1 /= sizeof(vec1[0]); vecstep2 /= sizeof(vec2[0]); avgstep /= sizeof(avg[0]);\
                                                                                        \
    for( ; size.height--; vec1 += vecstep1, vec2 += vecstep2, avg += avgstep )          \
    {                                                                                   \
        int x;                                                                          \
        for( x = 0; x <= size.width - 4; x += 4 )                                       \
            result += (load_macro(vec1[x]) - avg[x])*(load_macro(vec2[x]) - avg[x]) +   \
                (load_macro(vec1[x+1]) - avg[x+1])*(load_macro(vec2[x+1]) - avg[x+1]) + \
                (load_macro(vec1[x+2]) - avg[x+2])*(load_macro(vec2[x+2]) - avg[x+2]) + \
                (load_macro(vec1[x+3]) - avg[x+3])*(load_macro(vec2[x+3]) - avg[x+3]);  \
        for( ; x < size.width; x++ )                                                    \
            result += (load_macro(vec1[x]) - avg[x])*(load_macro(vec2[x]) - avg[x]);    \
    }                                                                                   \
                                                                                        \
    *_result = result;                                                                  \
    return CV_OK;                                                                       \
}


ICV_DOT_PRODUCT_CASE( 8u32f, uchar, float, CV_8TO32F )
ICV_DOT_PRODUCT_CASE( 8u64f, uchar, double, CV_8TO32F )
ICV_DOT_PRODUCT_CASE( 16u32f, ushort, float, CV_NOP )
ICV_DOT_PRODUCT_CASE( 16u64f, ushort, double, CV_NOP )
ICV_DOT_PRODUCT_CASE( 16s32f, short, float, CV_NOP )
ICV_DOT_PRODUCT_CASE( 16s64f, short, double, CV_NOP )
ICV_DOT_PRODUCT_CASE( 32f, float, float, CV_NOP )
ICV_DOT_PRODUCT_CASE( 32f64f, float, double, CV_NOP )
ICV_DOT_PRODUCT_CASE( 64f, double, double, CV_NOP )

static void  icvInitDotProductShiftedTable( CvFuncTable* tabfl, CvFuncTable* tabdb )
{
    tabfl->fn_2d[CV_8U] = (void*)icvDotProductShifted_8u32f_C1R;
    tabfl->fn_2d[CV_8S] = 0;
    tabfl->fn_2d[CV_16U] = (void*)icvDotProductShifted_16u32f_C1R;
    tabfl->fn_2d[CV_16S] = (void*)icvDotProductShifted_16s32f_C1R;
    tabfl->fn_2d[CV_32S] = 0;
    tabfl->fn_2d[CV_32F] = (void*)icvDotProductShifted_32f_C1R;
    tabfl->fn_2d[CV_64F] = 0;

    tabdb->fn_2d[CV_8U] = (void*)icvDotProductShifted_8u64f_C1R;
    tabdb->fn_2d[CV_8S] = 0;
    tabdb->fn_2d[CV_16U] = (void*)icvDotProductShifted_16u64f_C1R;
    tabdb->fn_2d[CV_16S] = (void*)icvDotProductShifted_16s64f_C1R;
    tabdb->fn_2d[CV_32S] = 0;
    tabdb->fn_2d[CV_32F] = (void*)icvDotProductShifted_32f64f_C1R;
    tabdb->fn_2d[CV_64F] = (void*)icvDotProductShifted_64f_C1R;
}

#define ICV_EXT_PRODUCT_CASE( flavor, srctype, avgtype, load_macro )                    \
static CvStatus CV_STDCALL                                                              \
icvExtProductShifted_##flavor##_C1R( const srctype* vec, int vecstep,                   \
                                     const avgtype* avg, int avgstep,                   \
                                     avgtype* dst, int dststep,                         \
                                     CvSize size, avgtype* tempbuf )                    \
{                                                                                       \
    int x, y, dstsize = size.width * size.height;                                       \
                                                                                        \
    vecstep /= sizeof(vec[0]); avgstep /= sizeof(avg[0]);                               \
    for( y = 0; y < size.height; y++, vec += vecstep, avg += avgstep )                  \
        for( x = 0; x < size.width; x++ )                                               \
            *tempbuf++ = load_macro(vec[x]) - avg[x];                                   \
    tempbuf -= dstsize;                                                                 \
                                                                                        \
    dststep /= sizeof(dst[0]);                                                          \
    for( y = 0; y < dstsize; y++, dst += dststep )                                      \
    {                                                                                   \
        double ty = tempbuf[y];                                                         \
        for( x = 0; x <= y - 3; x += 4 )                                                \
        {                                                                               \
            double t0 = dst[x] + ty*tempbuf[x];                                         \
            double t1 = dst[x+1] + ty*tempbuf[x+1];                                     \
            dst[x] = (avgtype)t0;                                                       \
            dst[x+1] = (avgtype)t1;                                                     \
            t0 = dst[x+2] + ty*tempbuf[x+2];                                            \
            t1 = dst[x+3] + ty*tempbuf[x+3];                                            \
            dst[x+2] = (avgtype)t0;                                                     \
            dst[x+3] = (avgtype)t1;                                                     \
        }                                                                               \
        for( ; x <= y; x++ )                                                            \
            dst[x] = (avgtype)(dst[x] + ty*tempbuf[x]);                                 \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}

ICV_EXT_PRODUCT_CASE( 8u32f, uchar, float, CV_8TO32F )
ICV_EXT_PRODUCT_CASE( 8u64f, uchar, double, CV_8TO32F )
ICV_EXT_PRODUCT_CASE( 16u32f, ushort, float, CV_NOP )
ICV_EXT_PRODUCT_CASE( 16u64f, ushort, double, CV_NOP )
ICV_EXT_PRODUCT_CASE( 16s32f, short, float, CV_NOP )
ICV_EXT_PRODUCT_CASE( 16s64f, short, double, CV_NOP )
ICV_EXT_PRODUCT_CASE( 32f, float, float, CV_NOP )
ICV_EXT_PRODUCT_CASE( 32f64f, float, double, CV_NOP )
ICV_EXT_PRODUCT_CASE( 64f, double, double, CV_NOP )


static void  icvInitExtProductShiftedTable( CvFuncTable* tabfl, CvFuncTable* tabdb )
{
    tabfl->fn_2d[CV_8U] = (void*)icvExtProductShifted_8u32f_C1R;
    tabfl->fn_2d[CV_8S] = 0;
    tabfl->fn_2d[CV_16U] = (void*)icvExtProductShifted_16u32f_C1R;
    tabfl->fn_2d[CV_16S] = (void*)icvExtProductShifted_16s32f_C1R;
    tabfl->fn_2d[CV_32S] = 0;
    tabfl->fn_2d[CV_32F] = (void*)icvExtProductShifted_32f_C1R;
    tabfl->fn_2d[CV_64F] = 0;

    tabdb->fn_2d[CV_8U] = (void*)icvExtProductShifted_8u64f_C1R;
    tabdb->fn_2d[CV_8S] = 0;
    tabdb->fn_2d[CV_16U] = (void*)icvExtProductShifted_16u64f_C1R;
    tabdb->fn_2d[CV_16S] = (void*)icvExtProductShifted_16s64f_C1R;
    tabdb->fn_2d[CV_32S] = 0;
    tabdb->fn_2d[CV_32F] = (void*)icvExtProductShifted_32f64f_C1R;
    tabdb->fn_2d[CV_64F] = (void*)icvExtProductShifted_64f_C1R;
}


typedef struct vec_data
{
    void* ptr;
    int step;
}
vec_data;

CV_IMPL void
cvCalcCovarMatrix( const CvArr** vecarr, int count,
                   CvArr* covarr, CvArr* avgarr, int flags )
{
    static CvFuncTable dot_tab[2];
    static CvFuncTable ext_tab[2];
    static int inittab = 0;
    vec_data* vecdata = 0;
    CvMat *tempvec = 0;
    
    CV_FUNCNAME( "cvCalcCovarMatrix" );

    __BEGIN__;

    CvMat covstub, *cov = (CvMat*)covarr;
    CvMat avgstub, *avg = (CvMat*)avgarr;
    CvMat vecstub0, *vecmat = 0;
    CvSize srcsize, contsize;
    int srctype = 0, dsttype = 0;
    int i, j;
    int cont_flag, vec_delta = 0, vec_step = 0;
    int is_covar_normal = (flags & CV_COVAR_NORMAL) != 0;
    double scale;

    if( !inittab )
    {
        icvInitDotProductShiftedTable( dot_tab + 0, dot_tab + 1 );
        icvInitExtProductShiftedTable( ext_tab + 0, ext_tab + 1 );
        inittab = 1;
    }

    if( !vecarr )
        CV_ERROR( CV_StsNullPtr, "NULL vec pointer" );

    CV_CALL( cov = cvGetMat( cov, &covstub ));
    CV_CALL( avg = cvGetMat( avg, &avgstub ));

    if( !CV_ARE_TYPES_EQ( cov, avg ))
        CV_ERROR( CV_StsUnmatchedFormats,
        "Covariation matrix and average vector should have the same types" );

    dsttype = CV_MAT_TYPE( cov->type );
    if( dsttype != CV_32FC1 && dsttype != CV_64FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Covariation matrix must be 32fC1 or 64fC1" );

    if( cov->rows != cov->cols )
        CV_ERROR( CV_StsBadSize, "Covariation matrix must be square" );

    srcsize = cvGetMatSize( avg );
    contsize.width = srcsize.width * srcsize.height;
    contsize.height = 1;
    cont_flag = avg->type;

    if( flags & (CV_COVAR_ROWS|CV_COVAR_COLS) )
    {
        CV_CALL( vecmat = cvGetMat( vecarr[0], &vecstub0 ));
        srctype = CV_MAT_TYPE(vecmat->type);
        if( flags & CV_COVAR_COLS )
        {
            count = vecmat->cols;
            if( avg->cols != 1 || avg->rows != vecmat->rows )
                CV_ERROR( CV_StsUnmatchedSizes,
                "The number of input vectors does not match to avg vector size" );
            cont_flag = 0;
            vec_delta = CV_ELEM_SIZE(vecmat->type);
            vec_step = vecmat->step;
        }
        else
        {
            count = vecmat->rows;
            if( avg->rows != 1 || avg->cols != vecmat->cols )
                CV_ERROR( CV_StsUnmatchedSizes,
                "The number of input vectors does not match to avg vector size" );
            vec_delta = vecmat->step;
            vec_step = CV_STUB_STEP;
        }
        
        if( !(flags & CV_COVAR_USE_AVG) )
            CV_CALL( cvReduce( vecmat, avg, -1, CV_REDUCE_AVG ));

        scale = !(flags & CV_COVAR_SCALE) ? 1. : 1./count;

        cvMulTransposed( vecmat, cov, ((flags & CV_COVAR_ROWS)!=0) ^ ((flags & CV_COVAR_NORMAL)==0), avg, scale );
        EXIT;
    }

    scale = !(flags & CV_COVAR_SCALE) ? 1. : 1./count;

    if( is_covar_normal )
    {
        if( count <= 0 )
            CV_ERROR( CV_StsBadSize,
            "The number of vectors is zero or negative" );
        if( cov->rows != contsize.width )
            CV_ERROR( CV_StsUnmatchedSizes,
            "The size of input vectors does not match with the size of covariation matrix" );

        CV_CALL( tempvec = cvCreateMat( avg->rows, avg->cols, dsttype ));
    }
    else if( count != cov->rows )
        CV_ERROR( CV_StsUnmatchedSizes,
        "The vector count and covariance matrix size do not match" );

    if( !(flags & (CV_COVAR_ROWS|CV_COVAR_COLS)) )
    {
        if( !(flags & CV_COVAR_USE_AVG) )
            cvZero( avg );

        CV_CALL( vecdata = (vec_data*)cvAlloc( count*sizeof(vecdata[0])));
    
        for( i = 0; i < count; i++ )
        {
            CvMat vecstub, *vec = (CvMat*)vecarr[i];
            CvMat* temp;

            if( !CV_IS_MAT(vec) )
                CV_CALL( vec = cvGetMat( vec, &vecstub ));

            if( !CV_ARE_SIZES_EQ( vec, avg ))
                CV_ERROR( CV_StsUnmatchedSizes,
                "All input vectors and average vector must have the same size" );

            vecdata[i].ptr = vec->data.ptr;
            vecdata[i].step = vec->step;
            cont_flag &= vec->type;
            temp = vec;
            if( i == 0 )
            {
                srctype = CV_MAT_TYPE( vec->type );
                if( CV_MAT_CN( srctype ) != 1 )
                    CV_ERROR( CV_BadNumChannels, "All vectors must have a single channel" );
                if( srctype != dsttype && !tempvec && !(flags & CV_COVAR_USE_AVG))
                    CV_CALL( tempvec = cvCreateMat( vec->rows, vec->cols, dsttype ));
            }
            else if( CV_MAT_TYPE(vec->type) != srctype )
                CV_ERROR( CV_StsUnmatchedFormats,
                "All input vectors must have the same type" );

            if( !(flags & CV_COVAR_USE_AVG) )
            {
                if( tempvec )
                {
                    temp = tempvec;
                    cvConvert( vec, temp );
                }
                cvAdd( temp, avg, avg );
            }
        }

        if( !(flags & CV_COVAR_USE_AVG) )
            cvScale( avg, avg, 1./count );
    }

    cont_flag = CV_IS_MAT_CONT( cont_flag );
    if( cont_flag )
        srcsize = contsize;

    if( !is_covar_normal )
    {
        CvFunc2D_3A1P dot_func =
            (CvFunc2D_3A1P)dot_tab[dsttype == CV_64FC1].fn_2d[CV_MAT_DEPTH(srctype)];
        
        if( !dot_func )
            CV_ERROR( CV_StsUnsupportedFormat,
            "The format of input vectors is not supported" );

        for( i = 0; i < count; i++ )
        {
            int a, b, delta;
            if( !(i & 1) )
                a = 0, b = i+1, delta = 1;
            else
                a = i, b = -1, delta = -1;

            for( j = a; j != b; j += delta )
            {
                double result = 0;
                void *v_i, *v_j;
                int step_i, step_j;

                if( !vecmat )
                {
                    v_i = vecdata[i].ptr;
                    v_j = vecdata[j].ptr;
                    step_i = vecdata[i].step;
                    step_j = vecdata[j].step;
                }
                else
                {
                    v_i = vecmat->data.ptr + vec_delta*i;
                    v_j = vecmat->data.ptr + vec_delta*j;
                    step_i = step_j = vec_step;
                }

                dot_func( v_i, step_i, v_j, step_j, avg->data.ptr, avg->step, srcsize, &result );

                if( dsttype == CV_64FC1 )
                {
                    ((double*)(cov->data.ptr + i*cov->step))[j] =
                    ((double*)(cov->data.ptr + j*cov->step))[i] = result*scale;
                }
                else
                {
                    ((float*)(cov->data.ptr + i*cov->step))[j] =
                    ((float*)(cov->data.ptr + j*cov->step))[i] = (float)(result*scale);
                }
            }
        }
    }
    else
    {
        uchar* cov_ptr = cov->data.ptr;
        int cov_step = cov->step;
        int cov_size = cov->rows;
        CvFunc2D_3A1P ext_func =
            (CvFunc2D_3A1P)ext_tab[dsttype == CV_64FC1].fn_2d[CV_MAT_DEPTH(srctype)];
        if( !ext_func )
            CV_ERROR( CV_StsUnsupportedFormat,
            "The format of input vectors is not supported" );
        
        cvZero( cov );
        
        for( i = 0; i < count; i++ )
        {
            void* v;
            int vstep;
            if( !vecmat )
            {
                v = vecdata[i].ptr;
                vstep = vecdata[i].step;
            }
            else
            {
                v = vecmat->data.ptr + vec_delta*i;
                vstep = vec_step;
            }

            ext_func( v, vstep, avg->data.ptr, avg->step,
                      cov_ptr, cov_step, srcsize, tempvec->data.ptr );
        }

        if( dsttype == CV_64FC1 )
            for( i = 0; i < cov_size; i++ )
                for( j = 0; j <= i; j++ )
                {
                    double* cov1 = ((double*)(cov_ptr + i*cov_step)) + j;
                    double* cov2 = ((double*)(cov_ptr + j*cov_step)) + i;

                    if( flags & CV_COVAR_SCALE )
                        *cov1 = *cov2 = *cov1*scale;
                    else
                        *cov2 = *cov1;
                }
        else
            for( i = 0; i < cov_size; i++ )
                for( j = 0; j <= i; j++ )
                {
                    float* cov1 = ((float*)(cov_ptr + i*cov_step)) + j;
                    float* cov2 = ((float*)(cov_ptr + j*cov_step)) + i;

                    if( flags & CV_COVAR_SCALE )
                        *cov1 = *cov2 = (float)(*cov1*scale);
                    else
                        *cov2 = *cov1;
                }
    }

    __END__;

    cvFree( &vecdata );
    cvReleaseMat( &tempvec );
}

/****************************************************************************************\
*                                        cvMahalanobis                                   *
\****************************************************************************************/

#define ICV_MAHALANOBIS( flavor, arrtype )                                              \
static CvStatus CV_STDCALL                                                              \
icvMahalanobis_##flavor##_C1R( const arrtype* mat, int matstep,                         \
                               const arrtype* vec, int len, double* _result )           \
{                                                                                       \
    int i, j;                                                                           \
    double result = 0;                                                                  \
                                                                                        \
    matstep /= sizeof(mat[0]);                                                          \
    for( i = 0; i < len; i++, mat += matstep )                                          \
    {                                                                                   \
        double row_sum = 0;                                                             \
        for( j = 0; j <= len - 4; j += 4 )                                              \
            row_sum += vec[j]*mat[j] + vec[j+1]*mat[j+1] +                              \
                       vec[j+2]*mat[j+2] + vec[j+3]*mat[j+3];                           \
        for( ; j < len; j++ )                                                           \
            row_sum += vec[j]*mat[j];                                                   \
        result += row_sum * vec[i];                                                     \
    }                                                                                   \
    *_result = result;                                                                  \
                                                                                        \
    return CV_OK;                                                                       \
}

ICV_MAHALANOBIS( 32f, float )
ICV_MAHALANOBIS( 64f, double )

static void  icvInitMahalanobisTable( CvFuncTable* tab )
{
    tab->fn_2d[CV_32F] = (void*)icvMahalanobis_32f_C1R;
    tab->fn_2d[CV_64F] = (void*)icvMahalanobis_64f_C1R;
}

typedef CvStatus (CV_STDCALL * CvMahalanobisFunc)( const void* mat, int matstep,
                                                   const void* vec, int len, double* _result );

CV_IMPL double
cvMahalanobis( const CvArr* srcAarr, const CvArr* srcBarr, CvArr* matarr )
{
    static CvFuncTable mahal_tab;
    static int inittab = 0;
    uchar* buffer = 0;
    int local_alloc = 0;
    double dist = 0;

    CV_FUNCNAME( "cvMahalanobis" );

    __BEGIN__;

    int buf_size, elem_size, len;
    CvMat stubA, *srcA = (CvMat*)srcAarr;
    CvMat stubB, *srcB = (CvMat*)srcBarr;
    CvMat stub, *mat = (CvMat*)matarr;
    CvMat temp;
    CvMahalanobisFunc func;

    if( !inittab )
    {
        icvInitMahalanobisTable( &mahal_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(srcA) )
        CV_CALL( srcA = cvGetMat( srcA, &stubA ));

    if( !CV_IS_MAT(srcB) )
        CV_CALL( srcB = cvGetMat( srcB, &stubB ));

    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub ));

    if( srcA->rows != 1 && srcA->cols != 1 )
        CV_ERROR( CV_StsBadSize, "Input matrices must be 1-d vectors" );

    len = srcA->rows + srcA->cols - 1;

    if( !CV_ARE_SIZES_EQ(srcA,srcB) )
        CV_ERROR( CV_StsUnmatchedSizes, "Input vectors have different sizes" );
    
    if( mat->rows != len || mat->cols != len )
        CV_ERROR( CV_StsUnmatchedSizes, "Input vectors and covariation matrix have different sizes" );

    func = (CvMahalanobisFunc)mahal_tab.fn_2d[CV_MAT_DEPTH(srcA->type)];

    if( CV_MAT_CN(srcA->type) > 1 || !func )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Only single-channel floating-point vectors are supported" );

    if( !CV_ARE_TYPES_EQ(srcA,srcB) || !CV_ARE_TYPES_EQ(srcA,mat) )
        CV_ERROR( CV_StsUnmatchedSizes, "Input vectors have different sizes" );

    elem_size = CV_ELEM_SIZE(srcA->type);
    buf_size = len*elem_size;
    
    if( buf_size <= CV_MAX_LOCAL_SIZE )
    {
        buffer = (uchar*)cvStackAlloc( buf_size );
        local_alloc = 1;
    }
    else
    {
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
    }

    temp = cvMat( srcA->rows, srcA->cols, srcA->type, buffer );
    CV_CALL( cvSub( srcA, srcB, &temp ));

    IPPI_CALL( func( mat->data.ptr, mat->step, temp.data.ptr, len, &dist ));
    dist = sqrt(dist);

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    return  dist;
}


/****************************************************************************************\
*                                        cvMulTransposed                                 *
\****************************************************************************************/

#define ICV_DEF_MULTRANS_R_FUNC( flavor, srctype, dsttype, load_macro )         \
static CvStatus CV_STDCALL                                                      \
icvMulTransposedR_##flavor( const srctype* src, int srcstep,                    \
                       dsttype* dst, int dststep,                               \
                       const dsttype* delta, int deltastep,                     \
                       CvSize size, int delta_cols, double scale )              \
{                                                                               \
    int i, j, k;                                                                \
    dsttype* tdst = dst;                                                        \
    dsttype* col_buf = 0;                                                       \
    dsttype* delta_buf = 0;                                                     \
    int local_alloc = 0;                                                        \
    int buf_size = size.height*sizeof(dsttype);                                 \
                                                                                \
    if( delta && delta_cols < size.width )                                      \
    {                                                                           \
        assert( delta_cols == 1 );                                              \
        buf_size += 4*buf_size;                                                 \
    }                                                                           \
                                                                                \
    if( buf_size <= CV_MAX_LOCAL_SIZE )                                         \
    {                                                                           \
        col_buf = (dsttype*)cvStackAlloc( buf_size );                           \
        local_alloc = 1;                                                        \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        col_buf = (dsttype*)cvAlloc( buf_size );                                \
        if( !col_buf )                                                          \
            return CV_OUTOFMEM_ERR;                                             \
    }                                                                           \
                                                                                \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);                       \
    deltastep /= sizeof(delta[0]);                                              \
                                                                                \
    if( delta && delta_cols < size.width )                                      \
    {                                                                           \
        delta_buf = col_buf + size.height;                                      \
        for( i = 0; i < size.height; i++ )                                      \
            delta_buf[i*4] = delta_buf[i*4+1] =                                 \
                delta_buf[i*4+2] = delta_buf[i*4+3] = delta[i*deltastep];       \
        delta = delta_buf;                                                      \
        deltastep = deltastep ? 4 : 0;                                          \
    }                                                                           \
                                                                                \
    if( !delta )                                                                \
        for( i = 0; i < size.width; i++, tdst += dststep )                      \
        {                                                                       \
            for( k = 0; k < size.height; k++ )                                  \
                col_buf[k] = src[k*srcstep+i];                                  \
                                                                                \
            for( j = i; j <= size.width - 4; j += 4 )                           \
            {                                                                   \
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;                          \
                const srctype *tsrc = src + j;                                  \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc += srcstep )             \
                {                                                               \
                    double a = col_buf[k];                                      \
                    s0 += a * load_macro(tsrc[0]);                              \
                    s1 += a * load_macro(tsrc[1]);                              \
                    s2 += a * load_macro(tsrc[2]);                              \
                    s3 += a * load_macro(tsrc[3]);                              \
                }                                                               \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
                tdst[j+1] = (dsttype)(s1*scale);                                \
                tdst[j+2] = (dsttype)(s2*scale);                                \
                tdst[j+3] = (dsttype)(s3*scale);                                \
            }                                                                   \
                                                                                \
            for( ; j < size.width; j++ )                                        \
            {                                                                   \
                double s0 = 0;                                                  \
                const srctype *tsrc = src + j;                                  \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc += srcstep )             \
                    s0 += col_buf[k] * tsrc[0];                                 \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
            }                                                                   \
        }                                                                       \
    else                                                                        \
        for( i = 0; i < size.width; i++, tdst += dststep )                      \
        {                                                                       \
            if( !delta_buf )                                                    \
                for( k = 0; k < size.height; k++ )                              \
                    col_buf[k] = load_macro(src[k*srcstep+i]) - delta[k*deltastep+i]; \
            else                                                                \
                for( k = 0; k < size.height; k++ )                              \
                    col_buf[k] = load_macro(src[k*srcstep+i]) - delta_buf[k*deltastep]; \
                                                                                \
            for( j = i; j <= size.width - 4; j += 4 )                           \
            {                                                                   \
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;                          \
                const srctype *tsrc = src + j;                                  \
                const dsttype *d = delta_buf ? delta_buf : delta + j;           \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc+=srcstep, d+=deltastep ) \
                {                                                               \
                    double a = col_buf[k];                                      \
                    s0 += a * (load_macro(tsrc[0]) - d[0]);                     \
                    s1 += a * (load_macro(tsrc[1]) - d[1]);                     \
                    s2 += a * (load_macro(tsrc[2]) - d[2]);                     \
                    s3 += a * (load_macro(tsrc[3]) - d[3]);                     \
                }                                                               \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
                tdst[j+1] = (dsttype)(s1*scale);                                \
                tdst[j+2] = (dsttype)(s2*scale);                                \
                tdst[j+3] = (dsttype)(s3*scale);                                \
            }                                                                   \
                                                                                \
            for( ; j < size.width; j++ )                                        \
            {                                                                   \
                double s0 = 0;                                                  \
                const srctype *tsrc = src + j;                                  \
                const dsttype *d = delta_buf ? delta_buf : delta + j;           \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc+=srcstep, d+=deltastep ) \
                    s0 += col_buf[k] * (load_macro(tsrc[0]) - d[0]);            \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
            }                                                                   \
        }                                                                       \
                                                                                \
    /* fill the lower part of the destination matrix */                         \
    for( i = 1; i < size.width; i++ )                                           \
        for( j = 0; j < i; j++ )                                                \
            dst[dststep*i + j] = dst[dststep*j + i];                            \
                                                                                \
    if( col_buf && !local_alloc )                                               \
        cvFree( &col_buf );                                                     \
                                                                                \
    return CV_NO_ERR;                                                           \
}


#define ICV_DEF_MULTRANS_L_FUNC( flavor, srctype, dsttype, load_macro )         \
static CvStatus CV_STDCALL                                                      \
icvMulTransposedL_##flavor( const srctype* src, int srcstep,                    \
                            dsttype* dst, int dststep,                          \
                            dsttype* delta, int deltastep,                      \
                            CvSize size, int delta_cols, double scale )         \
{                                                                               \
    int i, j, k;                                                                \
    dsttype* tdst = dst;                                                        \
                                                                                \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);                       \
    deltastep /= sizeof(delta[0]);                                              \
                                                                                \
    if( !delta )                                                                \
        for( i = 0; i < size.height; i++, tdst += dststep )                     \
            for( j = i; j < size.height; j++ )                                  \
            {                                                                   \
                double s = 0;                                                   \
                const srctype *tsrc1 = src + i*srcstep;                         \
                const srctype *tsrc2 = src + j*srcstep;                         \
                                                                                \
                for( k = 0; k <= size.width - 4; k += 4 )                       \
                    s += tsrc1[k]*tsrc2[k] + tsrc1[k+1]*tsrc2[k+1] +            \
                         tsrc1[k+2]*tsrc2[k+2] + tsrc1[k+3]*tsrc2[k+3];         \
                for( ; k < size.width; k++ )                                    \
                    s += tsrc1[k] * tsrc2[k];                                   \
                tdst[j] = (dsttype)(s*scale);                                   \
            }                                                                   \
    else                                                                        \
    {                                                                           \
        dsttype* row_buf = 0;                                                   \
        int local_alloc = 0;                                                    \
        int buf_size = size.width*sizeof(dsttype);                              \
        dsttype delta_buf[4];                                                   \
        int delta_shift = delta_cols == size.width ? 4 : 0;                     \
                                                                                \
        if( buf_size <= CV_MAX_LOCAL_SIZE )                                     \
        {                                                                       \
            row_buf = (dsttype*)cvStackAlloc( buf_size );                       \
            local_alloc = 1;                                                    \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            row_buf = (dsttype*)cvAlloc( buf_size );                            \
            if( !row_buf )                                                      \
                return CV_OUTOFMEM_ERR;                                         \
        }                                                                       \
                                                                                \
        for( i = 0; i < size.height; i++, tdst += dststep )                     \
        {                                                                       \
            const srctype *tsrc1 = src + i*srcstep;                             \
            const dsttype *tdelta1 = delta + i*deltastep;                       \
                                                                                \
            if( delta_cols < size.width )                                       \
                for( k = 0; k < size.width; k++ )                               \
                    row_buf[k] = tsrc1[k] - tdelta1[0];                         \
            else                                                                \
                for( k = 0; k < size.width; k++ )                               \
                    row_buf[k] = tsrc1[k] - tdelta1[k];                         \
                                                                                \
            for( j = i; j < size.height; j++ )                                  \
            {                                                                   \
                double s = 0;                                                   \
                const srctype *tsrc2 = src + j*srcstep;                         \
                const dsttype *tdelta2 = delta + j*deltastep;                   \
                if( delta_cols < size.width )                                   \
                {                                                               \
                    delta_buf[0] = delta_buf[1] =                               \
                        delta_buf[2] = delta_buf[3] = tdelta2[0];               \
                    tdelta2 = delta_buf;                                        \
                }                                                               \
                for( k = 0; k <= size.width-4; k += 4, tdelta2 += delta_shift ) \
                    s += row_buf[k]*(load_macro(tsrc2[k]) - tdelta2[0]) +       \
                         row_buf[k+1]*(load_macro(tsrc2[k+1]) - tdelta2[1]) +   \
                         row_buf[k+2]*(load_macro(tsrc2[k+2]) - tdelta2[2]) +   \
                         row_buf[k+3]*(load_macro(tsrc2[k+3]) - tdelta2[3]);    \
                for( ; k < size.width; k++, tdelta2++ )                         \
                    s += row_buf[k]*(load_macro(tsrc2[k]) - tdelta2[0]);        \
                tdst[j] = (dsttype)(s*scale);                                   \
            }                                                                   \
        }                                                                       \
                                                                                \
        if( row_buf && !local_alloc )                                           \
            cvFree( &row_buf );                                                 \
    }                                                                           \
                                                                                \
    /* fill the lower part of the destination matrix */                         \
    for( j = 0; j < size.height - 1; j++ )                                      \
        for( i = j; i < size.height; i++ )                                      \
            dst[dststep*i + j] = dst[dststep*j + i];                            \
                                                                                \
    return CV_NO_ERR;                                                           \
}


ICV_DEF_MULTRANS_R_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_MULTRANS_R_FUNC( 8u64f, uchar, double, CV_8TO32F )
ICV_DEF_MULTRANS_R_FUNC( 32f, float, float, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 32f64f, float, double, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 64f, double, double, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16u32f, ushort, float, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16u64f, ushort, double, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16s32f, short, float, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16s64f, short, double, CV_NOP )

ICV_DEF_MULTRANS_L_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_MULTRANS_L_FUNC( 8u64f, uchar, double, CV_8TO32F )
ICV_DEF_MULTRANS_L_FUNC( 32f, float, float, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 32f64f, float, double, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 64f, double, double, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16u32f, ushort, float, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16u64f, ushort, double, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16s32f, short, float, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16s64f, short, double, CV_NOP )


typedef CvStatus (CV_STDCALL * CvMulTransposedFunc)
    ( const void* src, int srcstep,
      void* dst, int dststep, const void* delta,
      int deltastep, CvSize size, int delta_cols, double scale );

CV_IMPL void
cvMulTransposed( const CvArr* srcarr, CvArr* dstarr,
                 int order, const CvArr* deltaarr, double scale )
{
    const int gemm_level = 100; // boundary above which GEMM is faster.
    CvMat* src2 = 0;

    CV_FUNCNAME( "cvMulTransposed" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvMat deltastub, *delta = (CvMat*)deltaarr;
    int stype, dtype;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub ));

    if( !CV_IS_MAT( dst ))
        CV_CALL( dst = cvGetMat( dst, &dstub ));

    if( delta )
    {
        if( !CV_IS_MAT( delta ))
            CV_CALL( delta = cvGetMat( delta, &deltastub ));

        if( !CV_ARE_TYPES_EQ( dst, delta ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( (delta->rows != src->rows && delta->rows != 1) ||
            (delta->cols != src->cols && delta->cols != 1) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }
    else
    {
        delta = &deltastub;
        delta->data.ptr = 0;
        delta->step = 0;
        delta->rows = delta->cols = 0;
    }

    stype = CV_MAT_TYPE( src->type );
    dtype = CV_MAT_TYPE( dst->type );

    if( dst->rows != dst->cols )
        CV_ERROR( CV_StsBadSize, "The destination matrix must be square" );

    if( (order != 0 && src->cols != dst->cols) ||
        (order == 0 && src->rows != dst->rows))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( src->data.ptr == dst->data.ptr || (stype == dtype &&
        (dst->cols >= gemm_level && dst->rows >= gemm_level &&
         src->cols >= gemm_level && src->rows >= gemm_level)))
    {
        if( deltaarr )
        {
            CV_CALL( src2 = cvCreateMat( src->rows, src->cols, src->type ));
            cvRepeat( delta, src2 );
            cvSub( src, src2, src2 );
            src = src2;
        }
        cvGEMM( src, src, scale, 0, 0, dst, order == 0 ? CV_GEMM_B_T : CV_GEMM_A_T );
    }
    else
    {
        CvMulTransposedFunc func =
            stype == CV_8U && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_8u32f :
                    (CvMulTransposedFunc)icvMulTransposedL_8u32f) :
            stype == CV_8U && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_8u64f :
                    (CvMulTransposedFunc)icvMulTransposedL_8u64f) :
            stype == CV_16U && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16u32f :
                    (CvMulTransposedFunc)icvMulTransposedL_16u32f) :
            stype == CV_16U && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16u64f :
                    (CvMulTransposedFunc)icvMulTransposedL_16u64f) :
            stype == CV_16S && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16s32f :
                    (CvMulTransposedFunc)icvMulTransposedL_16s32f) :
            stype == CV_16S && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16s64f :
                    (CvMulTransposedFunc)icvMulTransposedL_16s64f) :
            stype == CV_32F && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_32f :
                    (CvMulTransposedFunc)icvMulTransposedL_32f) :
            stype == CV_32F && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_32f64f :
                    (CvMulTransposedFunc)icvMulTransposedL_32f64f) :
            stype == CV_64F && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_64f :
                    (CvMulTransposedFunc)icvMulTransposedL_64f) : 0;

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, dst->data.ptr, dst->step,
                         delta->data.ptr, delta->step, cvGetMatSize( src ),
                         delta->cols, scale ));
    }

    __END__;

    if( src2 )
        cvReleaseMat( &src2 );
}


/****************************************************************************************\
*                                        cvDotProduct                                    *
\****************************************************************************************/

#define ICV_DEF_DOT_PROD_FUNC_2D( flavor, arrtype, temptype, sumtype )  \
static CvStatus CV_STDCALL                                              \
icvDotProduct_##flavor##_C1R( const arrtype* src1, int step1,           \
                              const arrtype* src2, int step2,           \
                              CvSize size, sumtype* _sum )              \
{                                                                       \
    sumtype sum = 0;                                                    \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]);                 \
                                                                        \
    for( ; size.height--; src1 += step1, src2 += step2 )                \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for( i = 0; i <= size.width - 4; i += 4 )                       \
        {                                                               \
            temptype t0 = (temptype)src1[i]*src2[i];                    \
            temptype t1 = (temptype)src1[i+1]*src2[i+1];                \
            t0 += (temptype)src1[i+2]*src2[i+2];                        \
            t1 += (temptype)src1[i+3]*src2[i+3];                        \
            sum += t0 + t1;                                             \
        }                                                               \
                                                                        \
        for( ; i < size.width; i++ )                                    \
        {                                                               \
            sum += (temptype)src1[i]*src2[i];                           \
        }                                                               \
    }                                                                   \
                                                                        \
    *_sum = sum;                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_DOT_PROD_FUNC_2D( 8u, uchar, int, int64 )
ICV_DEF_DOT_PROD_FUNC_2D( 16u, ushort, int64, int64 )
ICV_DEF_DOT_PROD_FUNC_2D( 16s, short, int64, int64 )
ICV_DEF_DOT_PROD_FUNC_2D( 32s, int, double, double )
ICV_DEF_DOT_PROD_FUNC_2D( 32f, float, double, double )
ICV_DEF_DOT_PROD_FUNC_2D( 64f, double, double, double )

#define icvDotProduct_8s_C1R 0

CV_DEF_INIT_FUNC_TAB_2D( DotProduct, C1R )

CV_IMPL double
cvDotProduct( const CvArr* srcAarr, const CvArr* srcBarr )
{
    static CvFuncTable tab_2d;
    static int inittab = 0;

    Cv64suf result;
    result.f = 0;
    
    CV_FUNCNAME( "cvDotProduct" );

    __BEGIN__;

    CvMat stubA, *srcA = (CvMat*)srcAarr;
    CvMat stubB, *srcB = (CvMat*)srcBarr;
    CvSize size;
    int type, depth;
    CvFunc2D_2A1P func;

    if( !inittab )
    {
        icvInitDotProductC1RTable( &tab_2d );
        inittab = 1;
    }

    if( !CV_IS_MAT( srcA ))
    {
        int coi = 0;
        CV_CALL( srcA = cvGetMat( srcA, &stubA, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( srcBarr == srcAarr )
        srcB = srcA; 
    else
    {
        if( !CV_IS_MAT( srcB ))
        {
            int coi = 0;
            CV_CALL( srcB = cvGetMat( srcB, &stubB, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "coi is not supported" );
        }

        if( !CV_ARE_TYPES_EQ( srcA, srcB ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( !CV_ARE_SIZES_EQ( srcA, srcB ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }

    type = CV_MAT_TYPE( srcA->type );
    size = cvGetMatSize( srcA );

    size.width *= CV_MAT_CN( type );
    depth = CV_MAT_DEPTH( type );

    if( CV_IS_MAT_CONT( srcA->type & srcB->type ))
    {
        size.width *= size.height;

        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
        {
            if( depth == CV_32F )
            {
                float* mA = srcA->data.fl;
                float* mB = srcB->data.fl;
                double sum = 0;
                do
                    sum += (double)mA[size.width - 1]*mB[size.width - 1];
                while( --size.width );
                result.f = sum;
                EXIT;
            }
            
            if( depth == CV_64F )
            {
                double* mA = srcA->data.db;
                double* mB = srcB->data.db;
                double sum = 0;
                do
                    sum += mA[size.width - 1]*mB[size.width - 1];
                while( --size.width );
                result.f = sum;
                EXIT;
            }
        }
        size.height = 1;
    }

    func = (CvFunc2D_2A1P)(tab_2d.fn_2d[depth]);
    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    IPPI_CALL( func( srcA->data.ptr, srcA->step,
                     srcB->data.ptr, srcB->step,
                     size, &result ));

    if( depth < CV_32S )
        result.f = (double)result.i;

    __END__;

    return result.f;
}

/* End of file. */
