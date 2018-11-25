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
#include <float.h>

/****************************************************************************************\
*                              Mean value over the region                                *
\****************************************************************************************/

#define ICV_MEAN_CASE_C1( len )         \
    for( ; x <= (len) - 2; x += 2 )     \
    {                                   \
        if( mask[x] )                   \
             s0 += src[x], pix++;       \
        if( mask[x+1] )                 \
            s0 += src[x+1], pix++;      \
    }                                   \
                                        \
    for( ; x < (len); x++ )             \
        if( mask[x] )                   \
            s0 += src[x], pix++


#define ICV_MEAN_CASE_C2( len )         \
    for( ; x < (len); x++ )             \
        if( mask[x] )                   \
        {                               \
            s0 += src[x*2];             \
            s1 += src[x*2+1];           \
            pix++;                      \
        }


#define ICV_MEAN_CASE_C3( len )         \
    for( ; x < (len); x++ )             \
        if( mask[x] )                   \
        {                               \
            s0 += src[x*3];             \
            s1 += src[x*3+1];           \
            s2 += src[x*3+2];           \
            pix++;                      \
        }


#define ICV_MEAN_CASE_C4( len )         \
    for( ; x < (len); x++ )             \
        if( mask[x] )                   \
        {                               \
            s0 += src[x*4];             \
            s1 += src[x*4+1];           \
            s2 += src[x*4+2];           \
            s3 += src[x*4+3];           \
            pix++;                      \
        }


#define ICV_MEAN_COI_CASE( len, cn )    \
    for( ; x <= (len) - 2; x += 2 )     \
    {                                   \
        if( mask[x] )                   \
             s0 += src[x*(cn)], pix++;  \
        if( mask[x+1] )                 \
            s0+=src[(x+1)*(cn)], pix++; \
    }                                   \
                                        \
    for( ; x < (len); x++ )             \
        if( mask[x] )                   \
            s0 += src[x*(cn)], pix++;


////////////////////////////////////// entry macros //////////////////////////////////////

#define ICV_MEAN_ENTRY_COMMON()         \
    int pix = 0;                        \
    step /= sizeof(src[0])

#define ICV_MEAN_ENTRY_C1( sumtype )    \
    sumtype s0 = 0;                     \
    ICV_MEAN_ENTRY_COMMON()

#define ICV_MEAN_ENTRY_C2( sumtype )    \
    sumtype s0 = 0, s1 = 0;             \
    ICV_MEAN_ENTRY_COMMON()

#define ICV_MEAN_ENTRY_C3( sumtype )    \
    sumtype s0 = 0, s1 = 0, s2 = 0;     \
    ICV_MEAN_ENTRY_COMMON()

#define ICV_MEAN_ENTRY_C4( sumtype )        \
    sumtype s0 = 0, s1 = 0, s2 = 0, s3 = 0; \
    ICV_MEAN_ENTRY_COMMON()


#define ICV_MEAN_ENTRY_BLOCK_COMMON( block_size ) \
    int remaining = block_size;                   \
    ICV_MEAN_ENTRY_COMMON()

#define ICV_MEAN_ENTRY_BLOCK_C1( sumtype, worktype, block_size )\
    sumtype sum0 = 0;                                           \
    worktype s0 = 0;                                            \
    ICV_MEAN_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_ENTRY_BLOCK_C2( sumtype, worktype, block_size )\
    sumtype sum0 = 0, sum1 = 0;                                 \
    worktype s0 = 0, s1 = 0;                                    \
    ICV_MEAN_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_ENTRY_BLOCK_C3( sumtype, worktype, block_size )\
    sumtype sum0 = 0, sum1 = 0, sum2 = 0;                       \
    worktype s0 = 0, s1 = 0, s2 = 0;                            \
    ICV_MEAN_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_ENTRY_BLOCK_C4( sumtype, worktype, block_size )\
    sumtype sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;             \
    worktype s0 = 0, s1 = 0, s2 = 0, s3 = 0;                    \
    ICV_MEAN_ENTRY_BLOCK_COMMON( block_size )


/////////////////////////////////////// exit macros //////////////////////////////////////

#define ICV_MEAN_EXIT_COMMON()          \
    double scale = pix ? 1./pix : 0

#define ICV_MEAN_EXIT_C1( tmp )         \
    ICV_MEAN_EXIT_COMMON();             \
    mean[0] = scale*(double)tmp##0

#define ICV_MEAN_EXIT_C2( tmp )         \
    ICV_MEAN_EXIT_COMMON();             \
    double t0 = scale*(double)tmp##0;   \
    double t1 = scale*(double)tmp##1;   \
    mean[0] = t0;                       \
    mean[1] = t1

#define ICV_MEAN_EXIT_C3( tmp )         \
    ICV_MEAN_EXIT_COMMON();             \
    double t0 = scale*(double)tmp##0;   \
    double t1 = scale*(double)tmp##1;   \
    double t2 = scale*(double)tmp##2;   \
    mean[0] = t0;                       \
    mean[1] = t1;                       \
    mean[2] = t2

#define ICV_MEAN_EXIT_C4( tmp )         \
    ICV_MEAN_EXIT_COMMON();             \
    double t0 = scale*(double)tmp##0;   \
    double t1 = scale*(double)tmp##1;   \
    mean[0] = t0;                       \
    mean[1] = t1;                       \
    t0 = scale*(double)tmp##2;          \
    t1 = scale*(double)tmp##3;          \
    mean[2] = t0;                       \
    mean[3] = t1

#define ICV_MEAN_EXIT_BLOCK_C1()    \
    sum0 += s0;                     \
    ICV_MEAN_EXIT_C1( sum )

#define ICV_MEAN_EXIT_BLOCK_C2()    \
    sum0 += s0; sum1 += s1;         \
    ICV_MEAN_EXIT_C2( sum )

#define ICV_MEAN_EXIT_BLOCK_C3()    \
    sum0 += s0; sum1 += s1;         \
    sum2 += s2;                     \
    ICV_MEAN_EXIT_C3( sum )

#define ICV_MEAN_EXIT_BLOCK_C4()    \
    sum0 += s0; sum1 += s1;         \
    sum2 += s2; sum3 += s3;         \
    ICV_MEAN_EXIT_C4( sum )

////////////////////////////////////// update macros /////////////////////////////////////

#define ICV_MEAN_UPDATE_COMMON( block_size )\
    remaining = block_size

#define ICV_MEAN_UPDATE_C1( block_size )    \
    ICV_MEAN_UPDATE_COMMON( block_size );   \
    sum0 += s0;                             \
    s0 = 0

#define ICV_MEAN_UPDATE_C2( block_size )    \
    ICV_MEAN_UPDATE_COMMON( block_size );   \
    sum0 += s0; sum1 += s1;                 \
    s0 = s1 = 0

#define ICV_MEAN_UPDATE_C3( block_size )    \
    ICV_MEAN_UPDATE_COMMON( block_size );   \
    sum0 += s0; sum1 += s1; sum2 += s2;     \
    s0 = s1 = s2 = 0

#define ICV_MEAN_UPDATE_C4( block_size )    \
    ICV_MEAN_UPDATE_COMMON( block_size );   \
    sum0 += s0; sum1 += s1;                 \
    sum2 += s2; sum3 += s3;                 \
    s0 = s1 = s2 = s3 = 0


#define ICV_IMPL_MEAN_BLOCK_FUNC_2D( flavor, cn,                \
    arrtype, sumtype, worktype, block_size )                    \
IPCVAPI_IMPL( CvStatus, icvMean_##flavor##_C##cn##MR,           \
    ( const arrtype* src, int step,                             \
      const uchar* mask, int maskstep,                          \
      CvSize size, double* mean ),                              \
    (src, step, mask, maskstep, size, mean))                    \
{                                                               \
    ICV_MEAN_ENTRY_BLOCK_C##cn( sumtype, worktype, block_size );\
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_MEAN_CASE_C##cn( limit );                       \
            if( remaining == 0 )                                \
            {                                                   \
                ICV_MEAN_UPDATE_C##cn( block_size );            \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    { ICV_MEAN_EXIT_BLOCK_C##cn(); }                            \
    return CV_OK;                                               \
}


#define ICV_IMPL_MEAN_FUNC_2D( flavor, cn,                      \
                arrtype, sumtype, worktype )                    \
IPCVAPI_IMPL( CvStatus, icvMean_##flavor##_C##cn##MR,           \
    ( const arrtype* src, int step,                             \
      const uchar* mask, int maskstep,                          \
      CvSize size, double* mean),                               \
    (src, step, mask, maskstep, size, mean))                    \
{                                                               \
    ICV_MEAN_ENTRY_C##cn( sumtype );                            \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        ICV_MEAN_CASE_C##cn( size.width );                      \
    }                                                           \
                                                                \
    { ICV_MEAN_EXIT_C##cn( s ); }                               \
    return CV_OK;                                               \
}


#define ICV_IMPL_MEAN_BLOCK_FUNC_2D_COI( flavor,                \
        arrtype, sumtype, worktype, block_size )                \
static CvStatus CV_STDCALL                                      \
icvMean_##flavor##_CnCMR( const arrtype* src, int step,         \
                          const uchar* mask, int maskstep,      \
                          CvSize size, int cn,                  \
                          int coi, double* mean )               \
{                                                               \
    ICV_MEAN_ENTRY_BLOCK_C1( sumtype, worktype, block_size );   \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_MEAN_COI_CASE( limit, cn );                     \
            if( remaining == 0 )                                \
            {                                                   \
                ICV_MEAN_UPDATE_C1( block_size );               \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    { ICV_MEAN_EXIT_BLOCK_C1(); }                               \
    return CV_OK;                                               \
}


#define ICV_IMPL_MEAN_FUNC_2D_COI( flavor,                      \
                arrtype, sumtype, worktype )                    \
static CvStatus CV_STDCALL                                      \
icvMean_##flavor##_CnCMR( const arrtype* src, int step,         \
                          const uchar* mask, int maskstep,      \
                          CvSize size, int cn,                  \
                          int coi, double* mean )               \
{                                                               \
    ICV_MEAN_ENTRY_C1( sumtype );                               \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        ICV_MEAN_COI_CASE( size.width, cn );                    \
    }                                                           \
                                                                \
    { ICV_MEAN_EXIT_C1( s ); }                                  \
    return CV_OK;                                               \
}


#define ICV_IMPL_MEAN_BLOCK_ALL( flavor, arrtype, sumtype,      \
                                 worktype, block_size )         \
    ICV_IMPL_MEAN_BLOCK_FUNC_2D( flavor, 1, arrtype, sumtype,   \
                                 worktype, block_size )         \
    ICV_IMPL_MEAN_BLOCK_FUNC_2D( flavor, 2, arrtype, sumtype,   \
                                 worktype, block_size )         \
    ICV_IMPL_MEAN_BLOCK_FUNC_2D( flavor, 3, arrtype, sumtype,   \
                                 worktype, block_size )         \
    ICV_IMPL_MEAN_BLOCK_FUNC_2D( flavor, 4, arrtype, sumtype,   \
                                 worktype, block_size )         \
    ICV_IMPL_MEAN_BLOCK_FUNC_2D_COI( flavor, arrtype, sumtype,  \
                                 worktype, block_size )

#define ICV_IMPL_MEAN_ALL( flavor, arrtype, sumtype, worktype )     \
    ICV_IMPL_MEAN_FUNC_2D( flavor, 1, arrtype, sumtype, worktype )  \
    ICV_IMPL_MEAN_FUNC_2D( flavor, 2, arrtype, sumtype, worktype )  \
    ICV_IMPL_MEAN_FUNC_2D( flavor, 3, arrtype, sumtype, worktype )  \
    ICV_IMPL_MEAN_FUNC_2D( flavor, 4, arrtype, sumtype, worktype )  \
    ICV_IMPL_MEAN_FUNC_2D_COI( flavor, arrtype, sumtype, worktype )

ICV_IMPL_MEAN_BLOCK_ALL( 8u, uchar, int64, unsigned, 1 << 24 )
ICV_IMPL_MEAN_BLOCK_ALL( 16u, ushort, int64, unsigned, 1 << 16 )
ICV_IMPL_MEAN_BLOCK_ALL( 16s, short, int64, int, 1 << 16 )
ICV_IMPL_MEAN_ALL( 32s, int, double, double )
ICV_IMPL_MEAN_ALL( 32f, float, double, double )
ICV_IMPL_MEAN_ALL( 64f, double, double, double )

#define icvMean_8s_C1MR 0
#define icvMean_8s_C2MR 0
#define icvMean_8s_C3MR 0
#define icvMean_8s_C4MR 0
#define icvMean_8s_CnCMR 0

CV_DEF_INIT_BIG_FUNC_TAB_2D( Mean, MR )
CV_DEF_INIT_FUNC_TAB_2D( Mean, CnCMR )

CV_IMPL  CvScalar
cvAvg( const void* img, const void* maskarr )
{
    CvScalar mean = {{0,0,0,0}};

    static CvBigFuncTable mean_tab;
    static CvFuncTable meancoi_tab;
    static int inittab = 0;

    CV_FUNCNAME("cvAvg");

    __BEGIN__;

    CvSize size;
    double scale;

    if( !maskarr )
    {
        CV_CALL( mean = cvSum(img));
        size = cvGetSize( img );
        size.width *= size.height;
        scale = size.width ? 1./size.width : 0;

        mean.val[0] *= scale;
        mean.val[1] *= scale;
        mean.val[2] *= scale;
        mean.val[3] *= scale;
    }
    else
    {
        int type, coi = 0;
        int mat_step, mask_step;

        CvMat stub, maskstub, *mat = (CvMat*)img, *mask = (CvMat*)maskarr;

        if( !inittab )
        {
            icvInitMeanMRTable( &mean_tab );
            icvInitMeanCnCMRTable( &meancoi_tab );
            inittab = 1;
        }

        if( !CV_IS_MAT(mat) )
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));

        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask) )
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, mask ) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        type = CV_MAT_TYPE( mat->type );
        size = cvGetMatSize( mat );

        mat_step = mat->step;
        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & mask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            mat_step = mask_step = CV_STUB_STEP;
        }

        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            CvFunc2D_2A1P func;

            if( CV_MAT_CN(type) > 4 )
                CV_ERROR( CV_StsOutOfRange, "The input array must have at most 4 channels unless COI is set" );

            func = (CvFunc2D_2A1P)(mean_tab.fn_2d[type]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr,
                             mask_step, size, mean.val ));
        }
        else
        {
            CvFunc2DnC_2A1P func = (CvFunc2DnC_2A1P)(
                meancoi_tab.fn_2d[CV_MAT_DEPTH(type)]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr,
                             mask_step, size, CV_MAT_CN(type), coi, mean.val ));
        }
    }

    __END__;

    return  mean;
}

/*  End of file  */
