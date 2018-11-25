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

#define  ICV_DEF_ACC_FUNC( name, srctype, dsttype, cvtmacro )           \
IPCVAPI_IMPL( CvStatus,                                                 \
name,( const srctype *src, int srcstep, dsttype *dst,                   \
       int dststep, CvSize size ), (src, srcstep, dst, dststep, size )) \
                                                                        \
{                                                                       \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 4; x += 4 )                       \
        {                                                               \
            dsttype t0 = dst[x] + cvtmacro(src[x]);                     \
            dsttype t1 = dst[x + 1] + cvtmacro(src[x + 1]);             \
            dst[x] = t0;  dst[x + 1] = t1;                              \
                                                                        \
            t0 = dst[x + 2] + cvtmacro(src[x + 2]);                     \
            t1 = dst[x + 3] + cvtmacro(src[x + 3]);                     \
            dst[x + 2] = t0;  dst[x + 3] = t1;                          \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            dst[x] += cvtmacro(src[x]);                                 \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_ACC_FUNC( icvAdd_8u32f_C1IR, uchar, float, CV_8TO32F )
ICV_DEF_ACC_FUNC( icvAdd_32f_C1IR, float, float, CV_NOP )
ICV_DEF_ACC_FUNC( icvAddSquare_8u32f_C1IR, uchar, float, CV_8TO32F_SQR )
ICV_DEF_ACC_FUNC( icvAddSquare_32f_C1IR, float, float, CV_SQR )


#define  ICV_DEF_ACCPROD_FUNC( flavor, srctype, dsttype, cvtmacro )         \
IPCVAPI_IMPL( CvStatus, icvAddProduct_##flavor##_C1IR,                      \
( const srctype *src1, int step1, const srctype *src2, int step2,           \
  dsttype *dst, int dststep, CvSize size ),                                 \
 (src1, step1, src2, step2, dst, dststep, size) )                           \
{                                                                           \
    step1 /= sizeof(src1[0]);                                               \
    step2 /= sizeof(src2[0]);                                               \
    dststep /= sizeof(dst[0]);                                              \
                                                                            \
    for( ; size.height--; src1 += step1, src2 += step2, dst += dststep )    \
    {                                                                       \
        int x;                                                              \
        for( x = 0; x <= size.width - 4; x += 4 )                           \
        {                                                                   \
            dsttype t0 = dst[x] + cvtmacro(src1[x])*cvtmacro(src2[x]);      \
            dsttype t1 = dst[x+1] + cvtmacro(src1[x+1])*cvtmacro(src2[x+1]);\
            dst[x] = t0;  dst[x + 1] = t1;                                  \
                                                                            \
            t0 = dst[x + 2] + cvtmacro(src1[x + 2])*cvtmacro(src2[x + 2]);  \
            t1 = dst[x + 3] + cvtmacro(src1[x + 3])*cvtmacro(src2[x + 3]);  \
            dst[x + 2] = t0;  dst[x + 3] = t1;                              \
        }                                                                   \
                                                                            \
        for( ; x < size.width; x++ )                                        \
            dst[x] += cvtmacro(src1[x])*cvtmacro(src2[x]);                  \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


ICV_DEF_ACCPROD_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCPROD_FUNC( 32f, float, float, CV_NOP )


#define  ICV_DEF_ACCWEIGHT_FUNC( flavor, srctype, dsttype, cvtmacro )   \
IPCVAPI_IMPL( CvStatus, icvAddWeighted_##flavor##_C1IR,                 \
( const srctype *src, int srcstep, dsttype *dst, int dststep,           \
  CvSize size, dsttype alpha ), (src, srcstep, dst, dststep, size, alpha) )\
{                                                                       \
    dsttype beta = (dsttype)(1 - alpha);                                \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 4; x += 4 )                       \
        {                                                               \
            dsttype t0 = dst[x]*beta + cvtmacro(src[x])*alpha;          \
            dsttype t1 = dst[x+1]*beta + cvtmacro(src[x+1])*alpha;      \
            dst[x] = t0; dst[x + 1] = t1;                               \
                                                                        \
            t0 = dst[x + 2]*beta + cvtmacro(src[x + 2])*alpha;          \
            t1 = dst[x + 3]*beta + cvtmacro(src[x + 3])*alpha;          \
            dst[x + 2] = t0; dst[x + 3] = t1;                           \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            dst[x] = dst[x]*beta + cvtmacro(src[x])*alpha;              \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_ACCWEIGHT_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCWEIGHT_FUNC( 32f, float, float, CV_NOP )


#define  ICV_DEF_ACCMASK_FUNC_C1( name, srctype, dsttype, cvtmacro )    \
IPCVAPI_IMPL( CvStatus,                                                 \
name,( const srctype *src, int srcstep, const uchar* mask, int maskstep,\
       dsttype *dst, int dststep, CvSize size ),                        \
       (src, srcstep, mask, maskstep, dst, dststep, size ))             \
{                                                                       \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 2; x += 2 )                       \
        {                                                               \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src[x]);                             \
            if( mask[x+1] )                                             \
                dst[x+1] += cvtmacro(src[x+1]);                         \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src[x]);                             \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_ACCMASK_FUNC_C1( icvAdd_8u32f_C1IMR, uchar, float, CV_8TO32F )
ICV_DEF_ACCMASK_FUNC_C1( icvAdd_32f_C1IMR, float, float, CV_NOP )
ICV_DEF_ACCMASK_FUNC_C1( icvAddSquare_8u32f_C1IMR, uchar, float, CV_8TO32F_SQR )
ICV_DEF_ACCMASK_FUNC_C1( icvAddSquare_32f_C1IMR, float, float, CV_SQR )


#define  ICV_DEF_ACCPRODUCTMASK_FUNC_C1( flavor, srctype, dsttype, cvtmacro )  \
IPCVAPI_IMPL( CvStatus, icvAddProduct_##flavor##_C1IMR,                 \
( const srctype *src1, int step1, const srctype* src2, int step2,       \
  const uchar* mask, int maskstep, dsttype *dst, int dststep, CvSize size ),\
  (src1, step1, src2, step2, mask, maskstep, dst, dststep, size ))      \
{                                                                       \
    step1 /= sizeof(src1[0]);                                           \
    step2 /= sizeof(src2[0]);                                           \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src1 += step1, src2 += step2,                 \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 2; x += 2 )                       \
        {                                                               \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src1[x])*cvtmacro(src2[x]);          \
            if( mask[x+1] )                                             \
                dst[x+1] += cvtmacro(src1[x+1])*cvtmacro(src2[x+1]);    \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src1[x])*cvtmacro(src2[x]);          \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_ACCPRODUCTMASK_FUNC_C1( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCPRODUCTMASK_FUNC_C1( 32f, float, float, CV_NOP )

#define  ICV_DEF_ACCWEIGHTMASK_FUNC_C1( flavor, srctype, dsttype, cvtmacro ) \
IPCVAPI_IMPL( CvStatus, icvAddWeighted_##flavor##_C1IMR,                \
( const srctype *src, int srcstep, const uchar* mask, int maskstep,     \
  dsttype *dst, int dststep, CvSize size, dsttype alpha ),              \
  (src, srcstep, mask, maskstep, dst, dststep, size, alpha ))           \
{                                                                       \
    dsttype beta = (dsttype)(1 - alpha);                                \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 2; x += 2 )                       \
        {                                                               \
            if( mask[x] )                                               \
                dst[x] = dst[x]*beta + cvtmacro(src[x])*alpha;          \
            if( mask[x+1] )                                             \
                dst[x+1] = dst[x+1]*beta + cvtmacro(src[x+1])*alpha;    \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            if( mask[x] )                                               \
                dst[x] = dst[x]*beta + cvtmacro(src[x])*alpha;          \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

ICV_DEF_ACCWEIGHTMASK_FUNC_C1( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCWEIGHTMASK_FUNC_C1( 32f, float, float, CV_NOP )


#define  ICV_DEF_ACCMASK_FUNC_C3( name, srctype, dsttype, cvtmacro )    \
IPCVAPI_IMPL( CvStatus,                                                 \
name,( const srctype *src, int srcstep, const uchar* mask, int maskstep,\
       dsttype *dst, int dststep, CvSize size ),                        \
       (src, srcstep, mask, maskstep, dst, dststep, size ))             \
{                                                                       \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x < size.width; x++ )                               \
            if( mask[x] )                                               \
            {                                                           \
                dsttype t0, t1, t2;                                     \
                t0 = dst[x*3] + cvtmacro(src[x*3]);                     \
                t1 = dst[x*3+1] + cvtmacro(src[x*3+1]);                 \
                t2 = dst[x*3+2] + cvtmacro(src[x*3+2]);                 \
                dst[x*3] = t0;                                          \
                dst[x*3+1] = t1;                                        \
                dst[x*3+2] = t2;                                        \
            }                                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_ACCMASK_FUNC_C3( icvAdd_8u32f_C3IMR, uchar, float, CV_8TO32F )
ICV_DEF_ACCMASK_FUNC_C3( icvAdd_32f_C3IMR, float, float, CV_NOP )
ICV_DEF_ACCMASK_FUNC_C3( icvAddSquare_8u32f_C3IMR, uchar, float, CV_8TO32F_SQR )
ICV_DEF_ACCMASK_FUNC_C3( icvAddSquare_32f_C3IMR, float, float, CV_SQR )


#define  ICV_DEF_ACCPRODUCTMASK_FUNC_C3( flavor, srctype, dsttype, cvtmacro )  \
IPCVAPI_IMPL( CvStatus, icvAddProduct_##flavor##_C3IMR,                 \
( const srctype *src1, int step1, const srctype* src2, int step2,       \
  const uchar* mask, int maskstep, dsttype *dst, int dststep, CvSize size ),\
  (src1, step1, src2, step2, mask, maskstep, dst, dststep, size ))      \
{                                                                       \
    step1 /= sizeof(src1[0]);                                           \
    step2 /= sizeof(src2[0]);                                           \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src1 += step1, src2 += step2,                 \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x < size.width; x++ )                               \
            if( mask[x] )                                               \
            {                                                           \
                dsttype t0, t1, t2;                                     \
                t0 = dst[x*3]+cvtmacro(src1[x*3])*cvtmacro(src2[x*3]);  \
                t1 = dst[x*3+1]+cvtmacro(src1[x*3+1])*cvtmacro(src2[x*3+1]);\
                t2 = dst[x*3+2]+cvtmacro(src1[x*3+2])*cvtmacro(src2[x*3+2]);\
                dst[x*3] = t0;                                          \
                dst[x*3+1] = t1;                                        \
                dst[x*3+2] = t2;                                        \
            }                                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_ACCPRODUCTMASK_FUNC_C3( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCPRODUCTMASK_FUNC_C3( 32f, float, float, CV_NOP )


#define  ICV_DEF_ACCWEIGHTMASK_FUNC_C3( flavor, srctype, dsttype, cvtmacro ) \
IPCVAPI_IMPL( CvStatus, icvAddWeighted_##flavor##_C3IMR,                \
( const srctype *src, int srcstep, const uchar* mask, int maskstep,     \
  dsttype *dst, int dststep, CvSize size, dsttype alpha ),              \
  (src, srcstep, mask, maskstep, dst, dststep, size, alpha ))           \
{                                                                       \
    dsttype beta = (dsttype)(1 - alpha);                                \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x < size.width; x++ )                               \
            if( mask[x] )                                               \
            {                                                           \
                dsttype t0, t1, t2;                                     \
                t0 = dst[x*3]*beta + cvtmacro(src[x*3])*alpha;          \
                t1 = dst[x*3+1]*beta + cvtmacro(src[x*3+1])*alpha;      \
                t2 = dst[x*3+2]*beta + cvtmacro(src[x*3+2])*alpha;      \
                dst[x*3] = t0;                                          \
                dst[x*3+1] = t1;                                        \
                dst[x*3+2] = t2;                                        \
            }                                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

ICV_DEF_ACCWEIGHTMASK_FUNC_C3( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCWEIGHTMASK_FUNC_C3( 32f, float, float, CV_NOP )


#define  ICV_DEF_INIT_ACC_TAB( FUNCNAME )                                           \
static  void  icvInit##FUNCNAME##Table( CvFuncTable* tab, CvBigFuncTable* masktab ) \
{                                                                                   \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u32f_C1IR;                          \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_C1IR;                           \
                                                                                    \
    masktab->fn_2d[CV_8UC1] = (void*)icv##FUNCNAME##_8u32f_C1IMR;                   \
    masktab->fn_2d[CV_32FC1] = (void*)icv##FUNCNAME##_32f_C1IMR;                    \
                                                                                    \
    masktab->fn_2d[CV_8UC3] = (void*)icv##FUNCNAME##_8u32f_C3IMR;                   \
    masktab->fn_2d[CV_32FC3] = (void*)icv##FUNCNAME##_32f_C3IMR;                    \
}


ICV_DEF_INIT_ACC_TAB( Add )
ICV_DEF_INIT_ACC_TAB( AddSquare )
ICV_DEF_INIT_ACC_TAB( AddProduct )
ICV_DEF_INIT_ACC_TAB( AddWeighted )


CV_IMPL void
cvAcc( const void* arr, void* sumarr, const void* maskarr )
{
    static CvFuncTable acc_tab;
    static CvBigFuncTable accmask_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvAcc" );

    __BEGIN__;

    int type, sumdepth;
    int mat_step, sum_step, mask_step = 0;
    CvSize size;
    CvMat stub, *mat = (CvMat*)arr;
    CvMat sumstub, *sum = (CvMat*)sumarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;

    if( !inittab )
    {
        icvInitAddTable( &acc_tab, &accmask_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT( mat ) || !CV_IS_MAT( sum ))
    {
        int coi1 = 0, coi2 = 0;
        CV_CALL( mat = cvGetMat( mat, &stub, &coi1 ));
        CV_CALL( sum = cvGetMat( sum, &sumstub, &coi2 ));
        if( coi1 + coi2 != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( CV_MAT_DEPTH( sum->type ) != CV_32F )
        CV_ERROR( CV_BadDepth, "" );

    if( !CV_ARE_CNS_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    sumdepth = CV_MAT_DEPTH( sum->type );
    if( sumdepth != CV_32F && (maskarr != 0 || sumdepth != CV_64F))
        CV_ERROR( CV_BadDepth, "Bad accumulator type" );

    if( !CV_ARE_SIZES_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( mat );
    type = CV_MAT_TYPE( mat->type );

    mat_step = mat->step;
    sum_step = sum->step;

    if( !mask )
    {
        CvFunc2D_2A func=(CvFunc2D_2A)acc_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "Unsupported type combination" );

        size.width *= CV_MAT_CN(type);
        if( CV_IS_MAT_CONT( mat->type & sum->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step, sum->data.ptr, sum_step, size ));
    }
    else
    {
        CvFunc2D_3A func = (CvFunc2D_3A)accmask_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );            

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & sum->type & mask->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr, mask_step,
                         sum->data.ptr, sum_step, size ));
    }

    __END__;
}


CV_IMPL void
cvSquareAcc( const void* arr, void* sq_sum, const void* maskarr )
{
    static CvFuncTable acc_tab;
    static CvBigFuncTable accmask_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvSquareAcc" );

    __BEGIN__;

    int coi1, coi2;
    int type;
    int mat_step, sum_step, mask_step = 0;
    CvSize size;
    CvMat stub, *mat = (CvMat*)arr;
    CvMat sumstub, *sum = (CvMat*)sq_sum;
    CvMat maskstub, *mask = (CvMat*)maskarr;

    if( !inittab )
    {
        icvInitAddSquareTable( &acc_tab, &accmask_tab );
        inittab = 1;
    }

    CV_CALL( mat = cvGetMat( mat, &stub, &coi1 ));
    CV_CALL( sum = cvGetMat( sum, &sumstub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_CNS_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_DEPTH( sum->type ) != CV_32F )
        CV_ERROR( CV_BadDepth, "" );

    if( !CV_ARE_SIZES_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( mat );
    type = CV_MAT_TYPE( mat->type );

    mat_step = mat->step;
    sum_step = sum->step;

    if( !mask )
    {
        CvFunc2D_2A func = (CvFunc2D_2A)acc_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        size.width *= CV_MAT_CN(type);

        if( CV_IS_MAT_CONT( mat->type & sum->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = CV_STUB_STEP;;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step, sum->data.ptr, sum_step, size ));
    }
    else
    {
        CvFunc2D_3A func = (CvFunc2D_3A)accmask_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );            

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & sum->type & mask->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr, mask_step,
                         sum->data.ptr, sum_step, size ));
    }

    __END__;
}


CV_IMPL void
cvMultiplyAcc( const void* arrA, const void* arrB,
               void* acc, const void* maskarr )
{
    static CvFuncTable acc_tab;
    static CvBigFuncTable accmask_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvMultiplyAcc" );

    __BEGIN__;

    int coi1, coi2, coi3;
    int type;
    int mat1_step, mat2_step, sum_step, mask_step = 0;
    CvSize size;
    CvMat stub1, *mat1 = (CvMat*)arrA;
    CvMat stub2, *mat2 = (CvMat*)arrB;
    CvMat sumstub, *sum = (CvMat*)acc;
    CvMat maskstub, *mask = (CvMat*)maskarr;

    if( !inittab )
    {
        icvInitAddProductTable( &acc_tab, &accmask_tab );
        inittab = 1;
    }

    CV_CALL( mat1 = cvGetMat( mat1, &stub1, &coi1 ));
    CV_CALL( mat2 = cvGetMat( mat2, &stub2, &coi2 ));
    CV_CALL( sum = cvGetMat( sum, &sumstub, &coi3 ));

    if( coi1 != 0 || coi2 != 0 || coi3 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_CNS_EQ( mat1, mat2 ) || !CV_ARE_CNS_EQ( mat1, sum ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_DEPTH( sum->type ) != CV_32F )
        CV_ERROR( CV_BadDepth, "" );

    if( !CV_ARE_SIZES_EQ( mat1, sum ) || !CV_ARE_SIZES_EQ( mat2, sum ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( mat1 );
    type = CV_MAT_TYPE( mat1->type );

    mat1_step = mat1->step;
    mat2_step = mat2->step;
    sum_step = sum->step;

    if( !mask )
    {
        CvFunc2D_3A func = (CvFunc2D_3A)acc_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        size.width *= CV_MAT_CN(type);

        if( CV_IS_MAT_CONT( mat1->type & mat2->type & sum->type ))
        {
            size.width *= size.height;
            mat1_step = mat2_step = sum_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                         sum->data.ptr, sum_step, size ));
    }
    else
    {
        CvFunc2D_4A func = (CvFunc2D_4A)accmask_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat1, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat1->type & mat2->type & sum->type & mask->type ))
        {
            size.width *= size.height;
            mat1_step = mat2_step = sum_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                         mask->data.ptr, mask_step,
                         sum->data.ptr, sum_step, size ));
    }

    __END__;
}


typedef CvStatus (CV_STDCALL *CvAddWeightedFunc)( const void* src, int srcstep,
                                                  void* dst, int dststep,
                                                  CvSize size, float alpha );

typedef CvStatus (CV_STDCALL *CvAddWeightedMaskFunc)( const void* src, int srcstep,
                                                      void* dst, int dststep,
                                                      const void* mask, int maskstep,
                                                      CvSize size, float alpha );

CV_IMPL void
cvRunningAvg( const void* arrY, void* arrU,
              double alpha, const void* maskarr )
{
    static CvFuncTable acc_tab;
    static CvBigFuncTable accmask_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvRunningAvg" );

    __BEGIN__;

    int coi1, coi2;
    int type;
    int mat_step, sum_step, mask_step = 0;
    CvSize size;
    CvMat stub, *mat = (CvMat*)arrY;
    CvMat sumstub, *sum = (CvMat*)arrU;
    CvMat maskstub, *mask = (CvMat*)maskarr;

    if( !inittab )
    {
        icvInitAddWeightedTable( &acc_tab, &accmask_tab );
        inittab = 1;
    }

    CV_CALL( mat = cvGetMat( mat, &stub, &coi1 ));
    CV_CALL( sum = cvGetMat( sum, &sumstub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_CNS_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_DEPTH( sum->type ) != CV_32F )
        CV_ERROR( CV_BadDepth, "" );

    if( !CV_ARE_SIZES_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( mat );
    type = CV_MAT_TYPE( mat->type );

    mat_step = mat->step;
    sum_step = sum->step;

    if( !mask )
    {
        CvAddWeightedFunc func = (CvAddWeightedFunc)acc_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        size.width *= CV_MAT_CN(type);
        if( CV_IS_MAT_CONT( mat->type & sum->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step,
                         sum->data.ptr, sum_step, size, (float)alpha ));
    }
    else
    {
        CvAddWeightedMaskFunc func = (CvAddWeightedMaskFunc)accmask_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & sum->type & mask->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr, mask_step,
                         sum->data.ptr, sum_step, size, (float)alpha ));
    }

    __END__;
}


/* End of file. */
