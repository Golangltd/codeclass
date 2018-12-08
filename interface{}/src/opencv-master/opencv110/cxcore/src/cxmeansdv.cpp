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
*                             Mean and StdDev calculation                                *
\****************************************************************************************/

#define ICV_MEAN_SDV_COI_CASE( worktype, sqsumtype, \
                               sqr_macro, len, cn ) \
    for( ; x <= (len) - 4*(cn); x += 4*(cn))\
    {                                       \
        worktype t0 = src[x];               \
        worktype t1 = src[x + (cn)];        \
                                            \
        s0  += t0 + t1;                     \
        sq0 += (sqsumtype)(sqr_macro(t0)) + \
               (sqsumtype)(sqr_macro(t1));  \
                                            \
        t0 = src[x + 2*(cn)];               \
        t1 = src[x + 3*(cn)];               \
                                            \
        s0  += t0 + t1;                     \
        sq0 += (sqsumtype)(sqr_macro(t0)) + \
               (sqsumtype)(sqr_macro(t1));  \
    }                                       \
                                            \
    for( ; x < (len); x += (cn) )           \
    {                                       \
        worktype t0 = src[x];               \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
    }


#define ICV_MEAN_SDV_CASE_C1( worktype, sqsumtype, sqr_macro, len ) \
    ICV_MEAN_SDV_COI_CASE( worktype, sqsumtype, sqr_macro, len, 1 )


#define ICV_MEAN_SDV_CASE_C2( worktype, sqsumtype, \
                              sqr_macro, len ) \
    for( ; x < (len); x += 2 )              \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x + 1];         \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
        s1 += t1;                           \
        sq1 += (sqsumtype)(sqr_macro(t1));  \
    }


#define ICV_MEAN_SDV_CASE_C3( worktype, sqsumtype, \
                              sqr_macro, len ) \
    for( ; x < (len); x += 3 )              \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x + 1];         \
        worktype t2 = (src)[x + 2];         \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
        s1 += t1;                           \
        sq1 += (sqsumtype)(sqr_macro(t1));  \
        s2 += t2;                           \
        sq2 += (sqsumtype)(sqr_macro(t2));  \
    }


#define ICV_MEAN_SDV_CASE_C4( worktype, sqsumtype, \
                              sqr_macro, len ) \
    for( ; x < (len); x += 4 )              \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x + 1];         \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
        s1 += t1;                           \
        sq1 += (sqsumtype)(sqr_macro(t1));  \
                                            \
        t0 = (src)[x + 2];                  \
        t1 = (src)[x + 3];                  \
                                            \
        s2 += t0;                           \
        sq2 += (sqsumtype)(sqr_macro(t0));  \
        s3 += t1;                           \
        sq3 += (sqsumtype)(sqr_macro(t1));  \
    }


#define ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqsumtype, \
                                    sqr_macro, len, cn ) \
    for( ; x <= (len) - 4; x += 4 )             \
    {                                           \
        worktype t0;                            \
        if( mask[x] )                           \
        {                                       \
            t0 = src[x*(cn)]; pix++;            \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
                                                \
        if( mask[x+1] )                         \
        {                                       \
            t0 = src[(x+1)*(cn)]; pix++;        \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
                                                \
        if( mask[x+2] )                         \
        {                                       \
            t0 = src[(x+2)*(cn)]; pix++;        \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
                                                \
        if( mask[x+3] )                         \
        {                                       \
            t0 = src[(x+3)*(cn)]; pix++;        \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
    }                                           \
                                                \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*(cn)]; pix++;   \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
    }


#define ICV_MEAN_SDV_MASK_CASE_C1( worktype, sqsumtype, sqr_macro, len )    \
    ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqsumtype, sqr_macro, len, 1 )


#define ICV_MEAN_SDV_MASK_CASE_C2( worktype, sqsumtype,\
                                   sqr_macro, len )    \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*2];             \
            worktype t1 = src[x*2+1];           \
            pix++;                              \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
            s1 += t1;                           \
            sq1 += sqsumtype(sqr_macro(t1));    \
        }                                       \
    }


#define ICV_MEAN_SDV_MASK_CASE_C3( worktype, sqsumtype,\
                                   sqr_macro, len )    \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*3];             \
            worktype t1 = src[x*3+1];           \
            worktype t2 = src[x*3+2];           \
            pix++;                              \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
            s1 += t1;                           \
            sq1 += sqsumtype(sqr_macro(t1));    \
            s2 += t2;                           \
            sq2 += sqsumtype(sqr_macro(t2));    \
        }                                       \
    }


#define ICV_MEAN_SDV_MASK_CASE_C4( worktype, sqsumtype,\
                                   sqr_macro, len )    \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*4];             \
            worktype t1 = src[x*4+1];           \
            pix++;                              \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
            s1 += t1;                           \
            sq1 += sqsumtype(sqr_macro(t1));    \
            t0 = src[x*4+2];                    \
            t1 = src[x*4+3];                    \
            s2 += t0;                           \
            sq2 += sqsumtype(sqr_macro(t0));    \
            s3 += t1;                           \
            sq3 += sqsumtype(sqr_macro(t1));    \
        }                                       \
    }


////////////////////////////////////// entry macros //////////////////////////////////////

#define ICV_MEAN_SDV_ENTRY_COMMON()                 \
    int pix;                                        \
    double scale, tmp;                              \
    step /= sizeof(src[0])

#define ICV_MEAN_SDV_ENTRY_C1( sumtype, sqsumtype ) \
    sumtype s0 = 0;                                 \
    sqsumtype sq0 = 0;                              \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_C2( sumtype, sqsumtype ) \
    sumtype s0 = 0, s1 = 0;                         \
    sqsumtype sq0 = 0, sq1 = 0;                     \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_C3( sumtype, sqsumtype ) \
    sumtype s0 = 0, s1 = 0, s2 = 0;                 \
    sqsumtype sq0 = 0, sq1 = 0, sq2 = 0;            \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_C4( sumtype, sqsumtype ) \
    sumtype s0 = 0, s1 = 0, s2 = 0, s3 = 0;         \
    sqsumtype sq0 = 0, sq1 = 0, sq2 = 0, sq3 = 0;   \
    ICV_MEAN_SDV_ENTRY_COMMON()


#define ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )   \
    int remaining = block_size;                         \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_BLOCK_C1( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0;                                           \
    sqsumtype sqsum0 = 0;                                       \
    worktype s0 = 0;                                            \
    sqworktype sq0 = 0;                                         \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_SDV_ENTRY_BLOCK_C2( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0, sum1 = 0;                                 \
    sqsumtype sqsum0 = 0, sqsum1 = 0;                           \
    worktype s0 = 0, s1 = 0;                                    \
    sqworktype sq0 = 0, sq1 = 0;                                \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_SDV_ENTRY_BLOCK_C3( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0, sum1 = 0, sum2 = 0;                       \
    sqsumtype sqsum0 = 0, sqsum1 = 0, sqsum2 = 0;               \
    worktype s0 = 0, s1 = 0, s2 = 0;                            \
    sqworktype sq0 = 0, sq1 = 0, sq2 = 0;                       \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_SDV_ENTRY_BLOCK_C4( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;             \
    sqsumtype sqsum0 = 0, sqsum1 = 0, sqsum2 = 0, sqsum3 = 0;   \
    worktype s0 = 0, s1 = 0, s2 = 0, s3 = 0;                    \
    sqworktype sq0 = 0, sq1 = 0, sq2 = 0, sq3 = 0;              \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )


/////////////////////////////////////// exit macros //////////////////////////////////////

#define ICV_MEAN_SDV_EXIT_COMMON()              \
    scale = pix ? 1./pix : 0

#define ICV_MEAN_SDV_EXIT_CN( total, sqtotal, idx ) \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    mean[idx] = tmp = scale*(double)total##idx; \
    tmp = scale*(double)sqtotal##idx - tmp*tmp; \
    sdv[idx] = sqrt(MAX(tmp,0.))

#define ICV_MEAN_SDV_EXIT_C1( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 )

#define ICV_MEAN_SDV_EXIT_C2( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 1 )

#define ICV_MEAN_SDV_EXIT_C3( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 1 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 2 )

#define ICV_MEAN_SDV_EXIT_C4( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 1 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 2 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 3 )

////////////////////////////////////// update macros /////////////////////////////////////

#define ICV_MEAN_SDV_UPDATE_COMMON( block_size )\
    remaining = block_size

#define ICV_MEAN_SDV_UPDATE_C1( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    s0 = 0; sq0 = 0

#define ICV_MEAN_SDV_UPDATE_C2( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    sum1 += s1; sqsum1 += sq1;                  \
    s0 = s1 = 0; sq0 = sq1 = 0

#define ICV_MEAN_SDV_UPDATE_C3( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    sum1 += s1; sqsum1 += sq1;                  \
    sum2 += s2; sqsum2 += sq2;                  \
    s0 = s1 = s2 = 0; sq0 = sq1 = sq2 = 0

#define ICV_MEAN_SDV_UPDATE_C4( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    sum1 += s1; sqsum1 += sq1;                  \
    sum2 += s2; sqsum2 += sq2;                  \
    sum3 += s3; sqsum3 += sq3;                  \
    s0 = s1 = s2 = s3 = 0; sq0 = sq1 = sq2 = sq3 = 0



#define ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, cn, arrtype,        \
                                sumtype, sqsumtype, worktype,       \
                                sqworktype, block_size, sqr_macro ) \
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##R,         \
                        ( const arrtype* src, int step,             \
                          CvSize size, double* mean, double* sdv ), \
                          (src, step, size, mean, sdv) )            \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C##cn( sumtype, sqsumtype,             \
                worktype, sqworktype, (block_size)*(cn) );          \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_CASE_C##cn( worktype, sqworktype,          \
                                     sqr_macro, limit );            \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C##cn( (block_size)*(cn) );     \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C##cn(0);                                   \
    ICV_MEAN_SDV_EXIT_C##cn( sum, sqsum );                          \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_FUNC_2D( flavor, cn, arrtype,              \
                                  sumtype, sqsumtype, worktype )    \
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##R,         \
                        ( const arrtype* src, int step,             \
                          CvSize size, double* mean, double* sdv ), \
                          (src, step, size, mean, sdv) )            \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C##cn( sumtype, sqsumtype );                 \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_CASE_C##cn( worktype, sqsumtype,               \
                                 CV_SQR, size.width );              \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C##cn( s, sq );                               \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D_COI( flavor, arrtype,        \
                                sumtype, sqsumtype, worktype,       \
                                sqworktype, block_size, sqr_macro ) \
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCR           \
                        ( const arrtype* src, int step,             \
                          CvSize size, int cn, int coi,             \
                          double* mean, double* sdv )               \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C1( sumtype, sqsumtype,                \
                worktype, sqworktype, (block_size)*(cn) );          \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_COI_CASE( worktype, sqworktype,            \
                                   sqr_macro, limit, cn);           \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C1( (block_size)*(cn) );        \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C1(0);                                      \
    ICV_MEAN_SDV_EXIT_C1( sum, sqsum );                             \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_FUNC_2D_COI( flavor, arrtype,              \
                                      sumtype, sqsumtype, worktype )\
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCR           \
                        ( const arrtype* src, int step, CvSize size,\
                        int cn, int coi, double* mean, double* sdv )\
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C1( sumtype, sqsumtype );                    \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_COI_CASE( worktype, sqsumtype,                 \
                               CV_SQR, size.width, cn );            \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C1( s, sq );                                  \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, cn,            \
                        arrtype, sumtype, sqsumtype, worktype,      \
                        sqworktype, block_size, sqr_macro )         \
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##MR,        \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, double* mean, double* sdv ), \
                       (src, step, mask, maskstep, size, mean, sdv))\
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C##cn( sumtype, sqsumtype,             \
                    worktype, sqworktype, block_size );             \
    pix = 0;                                                        \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_MASK_CASE_C##cn( worktype, sqworktype,     \
                                          sqr_macro, limit );       \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C##cn( block_size );            \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C##cn(0);                                   \
    ICV_MEAN_SDV_EXIT_C##cn( sum, sqsum );                          \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_FUNC_2D( flavor, cn, arrtype,         \
                                       sumtype, sqsumtype, worktype)\
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##MR,        \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, double* mean, double* sdv ), \
                       (src, step, mask, maskstep, size, mean, sdv))\
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C##cn( sumtype, sqsumtype );                 \
    pix = 0;                                                        \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_MASK_CASE_C##cn( worktype, sqsumtype,          \
                                      CV_SQR, size.width );         \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C##cn( s, sq );                               \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D_COI( flavor,            \
                            arrtype, sumtype, sqsumtype, worktype,  \
                            sqworktype, block_size, sqr_macro )     \
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCMR          \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, int cn, int coi,             \
                          double* mean, double* sdv )               \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C1( sumtype, sqsumtype,                \
                    worktype, sqworktype, block_size );             \
    pix = 0;                                                        \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqworktype,       \
                                        sqr_macro, limit, cn );     \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C1( block_size );               \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C1(0);                                      \
    ICV_MEAN_SDV_EXIT_C1( sum, sqsum );                             \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_FUNC_2D_COI( flavor, arrtype,         \
                                    sumtype, sqsumtype, worktype )  \
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCMR          \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, int cn, int coi,             \
                          double* mean, double* sdv )               \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C1( sumtype, sqsumtype );                    \
    pix = 0;                                                        \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqsumtype,            \
                                    CV_SQR, size.width, cn );       \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C1( s, sq );                                  \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_BLOCK_ALL( flavor, arrtype, sumtype, sqsumtype,\
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 1, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 2, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 3, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 4, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D_COI( flavor, arrtype, sumtype, sqsumtype,\
                            worktype, sqworktype, block_size, sqr_macro)\
                                                                        \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 1, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 2, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 3, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 4, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D_COI( flavor, arrtype, sumtype,      \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )

#define ICV_DEF_MEAN_SDV_ALL( flavor, arrtype, sumtype, sqsumtype, worktype )   \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 1, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 2, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 3, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 4, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D_COI( flavor, arrtype, sumtype, sqsumtype, worktype )   \
                                                                                \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 1, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 2, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 3, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 4, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D_COI( flavor, arrtype, sumtype, sqsumtype, worktype )


ICV_DEF_MEAN_SDV_BLOCK_ALL( 8u, uchar, int64, int64, unsigned, unsigned, 1 << 16, CV_SQR_8U )
ICV_DEF_MEAN_SDV_BLOCK_ALL( 16u, ushort, int64, int64, unsigned, int64, 1 << 16, CV_SQR )
ICV_DEF_MEAN_SDV_BLOCK_ALL( 16s, short, int64, int64, int, int64, 1 << 16, CV_SQR )

ICV_DEF_MEAN_SDV_ALL( 32s, int, double, double, double )
ICV_DEF_MEAN_SDV_ALL( 32f, float, double, double, double )
ICV_DEF_MEAN_SDV_ALL( 64f, double, double, double, double )

#define icvMean_StdDev_8s_C1R  0
#define icvMean_StdDev_8s_C2R  0
#define icvMean_StdDev_8s_C3R  0
#define icvMean_StdDev_8s_C4R  0
#define icvMean_StdDev_8s_CnCR 0

#define icvMean_StdDev_8s_C1MR  0
#define icvMean_StdDev_8s_C2MR  0
#define icvMean_StdDev_8s_C3MR  0
#define icvMean_StdDev_8s_C4MR  0
#define icvMean_StdDev_8s_CnCMR 0

CV_DEF_INIT_BIG_FUNC_TAB_2D( Mean_StdDev, R )
CV_DEF_INIT_FUNC_TAB_2D( Mean_StdDev, CnCR )
CV_DEF_INIT_BIG_FUNC_TAB_2D( Mean_StdDev, MR )
CV_DEF_INIT_FUNC_TAB_2D( Mean_StdDev, CnCMR )

CV_IMPL  void
cvAvgSdv( const CvArr* img, CvScalar* _mean, CvScalar* _sdv, const void* mask )
{
    CvScalar mean = {{0,0,0,0}};
    CvScalar sdv = {{0,0,0,0}};

    static CvBigFuncTable meansdv_tab;
    static CvFuncTable meansdvcoi_tab;
    static CvBigFuncTable meansdvmask_tab;
    static CvFuncTable meansdvmaskcoi_tab;
    static int inittab = 0;

    CV_FUNCNAME("cvMean_StdDev");

    __BEGIN__;

    int type, coi = 0;
    int mat_step, mask_step = 0;
    CvSize size;
    CvMat stub, maskstub, *mat = (CvMat*)img, *matmask = (CvMat*)mask;

    if( !inittab )
    {
        icvInitMean_StdDevRTable( &meansdv_tab );
        icvInitMean_StdDevCnCRTable( &meansdvcoi_tab );
        icvInitMean_StdDevMRTable( &meansdvmask_tab );
        icvInitMean_StdDevCnCMRTable( &meansdvmaskcoi_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub, &coi ));

    type = CV_MAT_TYPE( mat->type );

    if( CV_MAT_CN(type) > 4 && coi == 0 )
        CV_ERROR( CV_StsOutOfRange, "The input array must have at most 4 channels unless COI is set" );

    size = cvGetMatSize( mat );
    mat_step = mat->step;

    if( !mask )
    {
        if( CV_IS_MAT_CONT( mat->type ))
        {
            size.width *= size.height;
            size.height = 1;
            mat_step = CV_STUB_STEP;
        }

        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            CvFunc2D_1A2P func = (CvFunc2D_1A2P)(meansdv_tab.fn_2d[type]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size, mean.val, sdv.val ));
        }
        else
        {
            CvFunc2DnC_1A2P func = (CvFunc2DnC_1A2P)
                (meansdvcoi_tab.fn_2d[CV_MAT_DEPTH(type)]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size,
                             CV_MAT_CN(type), coi, mean.val, sdv.val ));
        }
    }
    else
    {
        CV_CALL( matmask = cvGetMat( matmask, &maskstub ));

        mask_step = matmask->step;

        if( !CV_IS_MASK_ARR( matmask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, matmask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        if( CV_IS_MAT_CONT( mat->type & matmask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            mat_step = mask_step = CV_STUB_STEP;
        }

        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            CvFunc2D_2A2P func = (CvFunc2D_2A2P)(meansdvmask_tab.fn_2d[type]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, matmask->data.ptr,
                             mask_step, size, mean.val, sdv.val ));
        }
        else
        {
            CvFunc2DnC_2A2P func = (CvFunc2DnC_2A2P)
                (meansdvmaskcoi_tab.fn_2d[CV_MAT_DEPTH(type)]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step,
                             matmask->data.ptr, mask_step,
                             size, CV_MAT_CN(type), coi, mean.val, sdv.val ));
        }
    }

    __END__;

    if( _mean )
        *_mean = mean;

    if( _sdv )
        *_sdv = sdv;
}


/*  End of file  */
