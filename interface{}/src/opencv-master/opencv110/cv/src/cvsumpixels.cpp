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

#define ICV_DEF_INTEGRAL_OP_C1( flavor, arrtype, sumtype, sqsumtype, worktype,  \
                                cast_macro, cast_sqr_macro )    \
static CvStatus CV_STDCALL                                      \
icvIntegralImage_##flavor##_C1R( const arrtype* src, int srcstep,\
                                 sumtype* sum, int sumstep,     \
                                 sqsumtype* sqsum, int sqsumstep,\
                                 sumtype* tilted, int tiltedstep,\
                                 CvSize size )                  \
{                                                               \
    int x, y;                                                   \
    sumtype s;                                                  \
    sqsumtype sq;                                               \
    sumtype* buf = 0;                                           \
                                                                \
    srcstep /= sizeof(src[0]);                                  \
                                                                \
    memset( sum, 0, (size.width+1)*sizeof(sum[0]));             \
    sumstep /= sizeof(sum[0]);                                  \
    sum += sumstep + 1;                                         \
                                                                \
    if( sqsum )                                                 \
    {                                                           \
        memset( sqsum, 0, (size.width+1)*sizeof(sqsum[0]));     \
        sqsumstep /= sizeof(sqsum[0]);                          \
        sqsum += sqsumstep + 1;                                 \
    }                                                           \
                                                                \
    if( tilted )                                                \
    {                                                           \
        memset( tilted, 0, (size.width+1)*sizeof(tilted[0]));   \
        tiltedstep /= sizeof(tilted[0]);                        \
        tilted += tiltedstep + 1;                               \
    }                                                           \
                                                                \
    if( sqsum == 0 && tilted == 0 )                             \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                                          sum += sumstep )      \
        {                                                       \
            sum[-1] = 0;                                        \
            for( x = 0, s = 0; x < size.width; x++ )            \
            {                                                   \
                sumtype t = cast_macro(src[x]);                 \
                s += t;                                         \
                sum[x] = sum[x - sumstep] + s;                  \
            }                                                   \
        }                                                       \
    }                                                           \
    else if( tilted == 0 )                                      \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                        sum += sumstep, sqsum += sqsumstep )    \
        {                                                       \
            sum[-1] = 0;                                        \
            sqsum[-1] = 0;                                      \
                                                                \
            for( x = 0, s = 0, sq = 0; x < size.width; x++ )    \
            {                                                   \
                worktype it = src[x];                           \
                sumtype t = cast_macro(it);                     \
                sqsumtype tq = cast_sqr_macro(it);              \
                s += t;                                         \
                sq += tq;                                       \
                t = sum[x - sumstep] + s;                       \
                tq = sqsum[x - sqsumstep] + sq;                 \
                sum[x] = t;                                     \
                sqsum[x] = tq;                                  \
            }                                                   \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        if( sqsum == 0 )                                        \
        {                                                       \
            assert(0);                                          \
            return CV_NULLPTR_ERR;                              \
        }                                                       \
                                                                \
        buf = (sumtype*)cvStackAlloc((size.width + 1 )* sizeof(buf[0]));\
        sum[-1] = tilted[-1] = 0;                               \
        sqsum[-1] = 0;                                          \
                                                                \
        for( x = 0, s = 0, sq = 0; x < size.width; x++ )        \
        {                                                       \
            worktype it = src[x];                               \
            sumtype t = cast_macro(it);                         \
            sqsumtype tq = cast_sqr_macro(it);                  \
            buf[x] = tilted[x] = t;                             \
            s += t;                                             \
            sq += tq;                                           \
            sum[x] = s;                                         \
            sqsum[x] = sq;                                      \
        }                                                       \
                                                                \
        if( size.width == 1 )                                   \
            buf[1] = 0;                                         \
                                                                \
        for( y = 1; y < size.height; y++ )                      \
        {                                                       \
            worktype it;                                        \
            sumtype t0;                                         \
            sqsumtype tq0;                                      \
                                                                \
            src += srcstep;                                     \
            sum += sumstep;                                     \
            sqsum += sqsumstep;                                 \
            tilted += tiltedstep;                               \
                                                                \
            it = src[0/*x*/];                                   \
            s = t0 = cast_macro(it);                            \
            sq = tq0 = cast_sqr_macro(it);                      \
                                                                \
            sum[-1] = 0;                                        \
            sqsum[-1] = 0;                                      \
            /*tilted[-1] = buf[0];*/                            \
            tilted[-1] = tilted[-tiltedstep];                   \
                                                                \
            sum[0] = sum[-sumstep] + t0;                        \
            sqsum[0] = sqsum[-sqsumstep] + tq0;                 \
            tilted[0] = tilted[-tiltedstep] + t0 + buf[1];      \
                                                                \
            for( x = 1; x < size.width - 1; x++ )               \
            {                                                   \
                sumtype t1 = buf[x];                            \
                buf[x-1] = t1 + t0;                             \
                it = src[x];                                    \
                t0 = cast_macro(it);                            \
                tq0 = cast_sqr_macro(it);                       \
                s += t0;                                        \
                sq += tq0;                                      \
                sum[x] = sum[x - sumstep] + s;                  \
                sqsum[x] = sqsum[x - sqsumstep] + sq;           \
                t1 += buf[x+1] + t0 + tilted[x - tiltedstep - 1];\
                tilted[x] = t1;                                 \
            }                                                   \
                                                                \
            if( size.width > 1 )                                \
            {                                                   \
                sumtype t1 = buf[x];                            \
                buf[x-1] = t1 + t0;                             \
                it = src[x];    /*+*/                           \
                t0 = cast_macro(it);                            \
                tq0 = cast_sqr_macro(it);                       \
                s += t0;                                        \
                sq += tq0;                                      \
                sum[x] = sum[x - sumstep] + s;                  \
                sqsum[x] = sqsum[x - sqsumstep] + sq;           \
                tilted[x] = t0 + t1 + tilted[x - tiltedstep - 1];\
                buf[x] = t0;                                    \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


ICV_DEF_INTEGRAL_OP_C1( 8u32s, uchar, int, double, int, CV_NOP, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_C1( 8u64f, uchar, double, double, int, CV_8TO32F, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_C1( 32f64f, float, double, double, double, CV_NOP, CV_SQR )
ICV_DEF_INTEGRAL_OP_C1( 64f, double, double, double, double, CV_NOP, CV_SQR )


#define ICV_DEF_INTEGRAL_OP_CN( flavor, arrtype, sumtype, sqsumtype,    \
                                worktype, cast_macro, cast_sqr_macro )  \
static CvStatus CV_STDCALL                                      \
icvIntegralImage_##flavor##_CnR( const arrtype* src, int srcstep,\
                                 sumtype* sum, int sumstep,     \
                                 sqsumtype* sqsum, int sqsumstep,\
                                 CvSize size, int cn )          \
{                                                               \
    int x, y;                                                   \
    srcstep /= sizeof(src[0]);                                  \
                                                                \
    memset( sum, 0, (size.width+1)*cn*sizeof(sum[0]));          \
    sumstep /= sizeof(sum[0]);                                  \
    sum += sumstep + cn;                                        \
                                                                \
    if( sqsum )                                                 \
    {                                                           \
        memset( sqsum, 0, (size.width+1)*cn*sizeof(sqsum[0]));  \
        sqsumstep /= sizeof(sqsum[0]);                          \
        sqsum += sqsumstep + cn;                                \
    }                                                           \
                                                                \
    size.width *= cn;                                           \
                                                                \
    if( sqsum == 0 )                                            \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                                          sum += sumstep )      \
        {                                                       \
            for( x = -cn; x < 0; x++ )                          \
                sum[x] = 0;                                     \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
                sum[x] = cast_macro(src[x]) + sum[x - cn];      \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
                sum[x] = sum[x] + sum[x - sumstep];             \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                        sum += sumstep, sqsum += sqsumstep )    \
        {                                                       \
            for( x = -cn; x < 0; x++ )                          \
            {                                                   \
                sum[x] = 0;                                     \
                sqsum[x] = 0;                                   \
            }                                                   \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
            {                                                   \
                worktype it = src[x];                           \
                sumtype t = cast_macro(it) + sum[x-cn];         \
                sqsumtype tq = cast_sqr_macro(it) + sqsum[x-cn];\
                sum[x] = t;                                     \
                sqsum[x] = tq;                                  \
            }                                                   \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
            {                                                   \
                sumtype t = sum[x] + sum[x - sumstep];          \
                sqsumtype tq = sqsum[x] + sqsum[x - sqsumstep]; \
                sum[x] = t;                                     \
                sqsum[x] = tq;                                  \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


ICV_DEF_INTEGRAL_OP_CN( 8u32s, uchar, int, double, int, CV_NOP, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_CN( 8u64f, uchar, double, double, int, CV_8TO32F, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_CN( 32f64f, float, double, double, double, CV_NOP, CV_SQR )
ICV_DEF_INTEGRAL_OP_CN( 64f, double, double, double, double, CV_NOP, CV_SQR )


static void icvInitIntegralImageTable( CvFuncTable* table_c1, CvFuncTable* table_cn )
{
    table_c1->fn_2d[CV_8U] = (void*)icvIntegralImage_8u64f_C1R;
    table_c1->fn_2d[CV_32F] = (void*)icvIntegralImage_32f64f_C1R;
    table_c1->fn_2d[CV_64F] = (void*)icvIntegralImage_64f_C1R;

    table_cn->fn_2d[CV_8U] = (void*)icvIntegralImage_8u64f_CnR;
    table_cn->fn_2d[CV_32F] = (void*)icvIntegralImage_32f64f_CnR;
    table_cn->fn_2d[CV_64F] = (void*)icvIntegralImage_64f_CnR;
}


typedef CvStatus (CV_STDCALL * CvIntegralImageFuncC1)(
    const void* src, int srcstep, void* sum, int sumstep,
    void* sqsum, int sqsumstep, void* tilted, int tiltedstep,
    CvSize size );

typedef CvStatus (CV_STDCALL * CvIntegralImageFuncCn)(
    const void* src, int srcstep, void* sum, int sumstep,
    void* sqsum, int sqsumstep, CvSize size, int cn );

icvIntegral_8u32s_C1R_t icvIntegral_8u32s_C1R_p = 0;
icvSqrIntegral_8u32s64f_C1R_t icvSqrIntegral_8u32s64f_C1R_p = 0;

CV_IMPL void
cvIntegral( const CvArr* image, CvArr* sumImage,
            CvArr* sumSqImage, CvArr* tiltedSumImage )
{
    static CvFuncTable tab_c1, tab_cn;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvIntegralImage" );

    __BEGIN__;

    CvMat src_stub, *src = (CvMat*)image;
    CvMat sum_stub, *sum = (CvMat*)sumImage;
    CvMat sqsum_stub, *sqsum = (CvMat*)sumSqImage;
    CvMat tilted_stub, *tilted = (CvMat*)tiltedSumImage;
    int coi0 = 0, coi1 = 0, coi2 = 0, coi3 = 0;
    int depth, cn;
    int src_step, sum_step, sqsum_step, tilted_step;
    CvIntegralImageFuncC1 func_c1 = 0;
    CvIntegralImageFuncCn func_cn = 0;
    CvSize size;

    if( !inittab )
    {
        icvInitIntegralImageTable( &tab_c1, &tab_cn );
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( src, &src_stub, &coi0 ));
    CV_CALL( sum = cvGetMat( sum, &sum_stub, &coi1 ));
    
    if( sum->width != src->width + 1 ||
        sum->height != src->height + 1 )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( (CV_MAT_DEPTH( sum->type ) != CV_64F &&
        (CV_MAT_DEPTH( src->type ) != CV_8U ||
         CV_MAT_DEPTH( sum->type ) != CV_32S )) ||
        !CV_ARE_CNS_EQ( src, sum ))
        CV_ERROR( CV_StsUnsupportedFormat,
        "Sum array must have 64f type (or 32s type in case of 8u source array) "
        "and the same number of channels as the source array" );

    if( sqsum )
    {
        CV_CALL( sqsum = cvGetMat( sqsum, &sqsum_stub, &coi2 ));
        if( !CV_ARE_SIZES_EQ( sum, sqsum ) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        if( CV_MAT_DEPTH( sqsum->type ) != CV_64F || !CV_ARE_CNS_EQ( src, sqsum ))
            CV_ERROR( CV_StsUnsupportedFormat,
                      "Squares sum array must be 64f "
                      "and the same number of channels as the source array" );
    }

    if( tilted )
    {
        if( !sqsum )
            CV_ERROR( CV_StsNullPtr,
            "Squared sum array must be passed if tilted sum array is passed" );

        CV_CALL( tilted = cvGetMat( tilted, &tilted_stub, &coi3 ));
        if( !CV_ARE_SIZES_EQ( sum, tilted ) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        if( !CV_ARE_TYPES_EQ( sum, tilted ) )
            CV_ERROR( CV_StsUnmatchedFormats,
                      "Sum and tilted sum must have the same types" );
        if( CV_MAT_CN(tilted->type) != 1 )
            CV_ERROR( CV_StsNotImplemented,
                      "Tilted sum can not be computed for multi-channel arrays" );
    }

    if( coi0 || coi1 || coi2 || coi3 )
        CV_ERROR( CV_BadCOI, "COI is not supported by the function" );

    depth = CV_MAT_DEPTH(src->type);
    cn = CV_MAT_CN(src->type);

    if( CV_MAT_DEPTH( sum->type ) == CV_32S )
    {
        func_c1 = (CvIntegralImageFuncC1)icvIntegralImage_8u32s_C1R;
        func_cn = (CvIntegralImageFuncCn)icvIntegralImage_8u32s_CnR;
    }
    else
    {
        func_c1 = (CvIntegralImageFuncC1)tab_c1.fn_2d[depth];
        func_cn = (CvIntegralImageFuncCn)tab_cn.fn_2d[depth];
        if( !func_c1 && !func_cn )
            CV_ERROR( CV_StsUnsupportedFormat, "This source image format is unsupported" );
    }

    size = cvGetMatSize(src);
    src_step = src->step ? src->step : CV_STUB_STEP;
    sum_step = sum->step ? sum->step : CV_STUB_STEP;
    sqsum_step = !sqsum ? 0 : sqsum->step ? sqsum->step : CV_STUB_STEP;
    tilted_step = !tilted ? 0 : tilted->step ? tilted->step : CV_STUB_STEP;

    if( cn == 1 )
    {
        if( depth == CV_8U && !tilted && CV_MAT_DEPTH(sum->type) == CV_32S )
        {
            if( !sqsum && icvIntegral_8u32s_C1R_p &&
                icvIntegral_8u32s_C1R_p( src->data.ptr, src_step,
                            sum->data.i, sum_step, size, 0 ) >= 0 )
                EXIT;
            
            if( sqsum && icvSqrIntegral_8u32s64f_C1R_p &&
                icvSqrIntegral_8u32s64f_C1R_p( src->data.ptr, src_step, sum->data.i,
                            sum_step, sqsum->data.db, sqsum_step, size, 0, 0 ) >= 0 )
                EXIT;
        }

        IPPI_CALL( func_c1( src->data.ptr, src_step, sum->data.ptr, sum_step,
                        sqsum ? sqsum->data.ptr : 0, sqsum_step,
                        tilted ? tilted->data.ptr : 0, tilted_step, size ));
    }
    else
    {
        IPPI_CALL( func_cn( src->data.ptr, src_step, sum->data.ptr, sum_step,
                        sqsum ? sqsum->data.ptr : 0, sqsum_step, size, cn ));
    }

    __END__;
}


/* End of file. */
