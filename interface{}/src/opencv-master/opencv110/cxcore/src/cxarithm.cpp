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

/* ////////////////////////////////////////////////////////////////////
//
//  CvMat arithmetic operations: +, - ...
//
// */

#include "_cxcore.h"

/****************************************************************************************\
*                      Arithmetic operations (+, -) without mask                         *
\****************************************************************************************/

#define ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype, cast_macro, len )\
{                                                                   \
    int i;                                                          \
                                                                    \
    for( i = 0; i <= (len) - 4; i += 4 )                            \
    {                                                               \
        worktype t0 = __op__((src1)[i], (src2)[i]);                 \
        worktype t1 = __op__((src1)[i+1], (src2)[i+1]);             \
                                                                    \
        (dst)[i] = cast_macro( t0 );                                \
        (dst)[i+1] = cast_macro( t1 );                              \
                                                                    \
        t0 = __op__((src1)[i+2],(src2)[i+2]);                       \
        t1 = __op__((src1)[i+3],(src2)[i+3]);                       \
                                                                    \
        (dst)[i+2] = cast_macro( t0 );                              \
        (dst)[i+3] = cast_macro( t1 );                              \
    }                                                               \
                                                                    \
    for( ; i < (len); i++ )                                         \
    {                                                               \
        worktype t0 = __op__((src1)[i],(src2)[i]);                  \
        (dst)[i] = cast_macro( t0 );                                \
    }                                                               \
}

#define ICV_DEF_BIN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )   \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size ),                                   \
      (src1, step1, src2, step2, dst, step, size) )                         \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_BIN_ARI_OP_2D_SFS(__op__, name, type, worktype, cast_macro) \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size, int /*scalefactor*/ ),              \
      (src1, step1, src2, step2, dst, step, size, 0) )                      \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,               \
                                src, scalar, dst, len )                     \
{                                                                           \
    int i;                                                                  \
                                                                            \
    for( ; ((len) -= 12) >= 0; (dst) += 12, (src) += 12 )                   \
    {                                                                       \
        worktype t0 = __op__((scalar)[0], (src)[0]);                        \
        worktype t1 = __op__((scalar)[1], (src)[1]);                        \
                                                                            \
        (dst)[0] = cast_macro( t0 );                                        \
        (dst)[1] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[2], (src)[2]);                                 \
        t1 = __op__((scalar)[3], (src)[3]);                                 \
                                                                            \
        (dst)[2] = cast_macro( t0 );                                        \
        (dst)[3] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[4], (src)[4]);                                 \
        t1 = __op__((scalar)[5], (src)[5]);                                 \
                                                                            \
        (dst)[4] = cast_macro( t0 );                                        \
        (dst)[5] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[6], (src)[6]);                                 \
        t1 = __op__((scalar)[7], (src)[7]);                                 \
                                                                            \
        (dst)[6] = cast_macro( t0 );                                        \
        (dst)[7] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[8], (src)[8]);                                 \
        t1 = __op__((scalar)[9], (src)[9]);                                 \
                                                                            \
        (dst)[8] = cast_macro( t0 );                                        \
        (dst)[9] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[10], (src)[10]);                               \
        t1 = __op__((scalar)[11], (src)[11]);                               \
                                                                            \
        (dst)[10] = cast_macro( t0 );                                       \
        (dst)[11] = cast_macro( t1 );                                       \
    }                                                                       \
                                                                            \
    for( (len) += 12, i = 0; i < (len); i++ )                               \
    {                                                                       \
        worktype t0 = __op__((scalar)[i],(src)[i]);                         \
        (dst)[i] = cast_macro( t0 );                                        \
    }                                                                       \
}


#define ICV_DEF_UN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )    \
static CvStatus CV_STDCALL name                                             \
    ( const type* src, int step1, type* dst, int step,                      \
      CvSize size, const worktype* scalar )                                 \
{                                                                           \
    step1 /= sizeof(src[0]); step /= sizeof(dst[0]);                        \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            worktype t0 = __op__(*(scalar),*(src));                         \
            *(dst) = cast_macro( t0 );                                      \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            const type *tsrc = src;                                         \
            type *tdst = dst;                                               \
            int width = size.width;                                         \
                                                                            \
            ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,           \
                                    tsrc, scalar, tdst, width );            \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_BIN_ARI_ALL( __op__, name, cast_8u )                                \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_8u_C1R, uchar, int, cast_8u )        \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )  \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )   \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )         \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )     \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )

#define ICV_DEF_UN_ARI_ALL( __op__, name )                                          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_8u_C1R, uchar, int, CV_CAST_8U )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )       \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )        \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )      \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )

#undef CV_SUB_R
#define CV_SUB_R(a,b) ((b) - (a))

ICV_DEF_BIN_ARI_ALL( CV_ADD, Add, CV_FAST_CAST_8U )
ICV_DEF_BIN_ARI_ALL( CV_SUB_R, Sub, CV_FAST_CAST_8U )

ICV_DEF_UN_ARI_ALL( CV_ADD, AddC )
ICV_DEF_UN_ARI_ALL( CV_SUB, SubRC )

#define ICV_DEF_INIT_ARITHM_FUNC_TAB( FUNCNAME, FLAG )          \
static  void  icvInit##FUNCNAME##FLAG##Table( CvFuncTable* tab )\
{                                                               \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u_##FLAG;       \
    tab->fn_2d[CV_8S] = 0;                                      \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##FLAG;     \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##FLAG;     \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##FLAG;     \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##FLAG;     \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##FLAG;     \
}

ICV_DEF_INIT_ARITHM_FUNC_TAB( Sub, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( SubRC, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( Add, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( AddC, C1R )

/****************************************************************************************\
*                       External Functions for Arithmetic Operations                     *
\****************************************************************************************/

/*************************************** S U B ******************************************/

CV_IMPL void
cvSub( const void* srcarr1, const void* srcarr2,
       void* dstarr, const void* maskarr )
{
    static CvFuncTable sub_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvSub" );

    __BEGIN__;

    const CvArr* tmp;
    int y, dy, type, depth, cn, cont_flag = 0;
    int src1_step, src2_step, dst_step, tdst_step, mask_step;
    CvMat srcstub1, srcstub2, *src1, *src2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_3A func;
    CvFunc2D_3A1I func_sfs;
    CvCopyMaskFunc copym_func;
    CvSize size, tsize;

    CV_SWAP( srcarr1, srcarr2, tmp ); // to comply with IPP
    src1 = (CvMat*)srcarr1;
    src2 = (CvMat*)srcarr2;

    if( !CV_IS_MAT(src1) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src1) || CV_IS_MATND(src2) || CV_IS_MATND(dst))
        {
            CvArr* arrs[] = { src1, src2, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            if( maskarr )
                CV_ERROR( CV_StsBadMask,
                "This operation on multi-dimensional arrays does not support mask" );

            CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

            type = iterator.hdr[0]->type;
            iterator.size.width *= CV_MAT_CN(type);

            if( !inittab )
            {
                icvInitSubC1RTable( &sub_tab );
                inittab = 1;
            }

            depth = CV_MAT_DEPTH(type);
            if( depth <= CV_16S )
            {
                func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
                if( !func_sfs )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func_sfs( iterator.ptr[0], CV_STUB_STEP,
                                         iterator.ptr[1], CV_STUB_STEP,
                                         iterator.ptr[2], CV_STUB_STEP,
                                         iterator.size, 0 ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.ptr[2], CV_STUB_STEP,
                                     iterator.size ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0, coi3 = 0;
            
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1 ));
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3 ));
            if( coi1 + coi2 + coi3 != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    if( !mask )
    {
        if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
        {
            int len = size.width*size.height*cn;

            if( len <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = (const float*)(src1->data.ptr);
                    const float* src2data = (const float*)(src2->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = (float)(src2data[len-1] - src1data[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = (const double*)(src1->data.ptr);
                    const double* src2data = (const double*)(src2->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = src2data[len-1] - src1data[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }

        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    if( !inittab )
    {
        icvInitSubC1RTable( &sub_tab );
        inittab = 1;
    }

    if( depth <= CV_16S )
    {
        func = 0;
        func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
        if( !func_sfs )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }
    else
    {
        func_sfs = 0;
        func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src1_step = src2_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( depth <= CV_16S ?
            func_sfs( src1->data.ptr + y*src1->step, src1_step,
                      src2->data.ptr + y*src2->step, src2_step,
                      tdst->data.ptr, tdst_step,
                      cvSize( tsize.width*cn, tsize.height ), 0 ) :
            func( src1->data.ptr + y*src1->step, src1_step,
                  src2->data.ptr + y*src2->step, src2_step,
                  tdst->data.ptr, tdst_step,
                  cvSize( tsize.width*cn, tsize.height )));

        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


CV_IMPL void
cvSubRS( const void* srcarr, CvScalar scalar, void* dstarr, const void* maskarr )
{
    static CvFuncTable subr_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvSubRS" );

    __BEGIN__;

    int sctype, y, dy, type, depth, cn, coi = 0, cont_flag = 0;
    int src_step, dst_step, tdst_step, mask_step;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_2A1P func;
    CvCopyMaskFunc copym_func;
    double buf[12];
    int is_nd = 0;
    CvSize size, tsize; 

    if( !inittab )
    {
        icvInitSubRCC1RTable( &subr_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src) )
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
        {
            CV_CALL( src = cvGetMat( src, &srcstub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
        {
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src, dst };
        CvMatND stubs[2];
        CvNArrayIterator iterator;

        if( maskarr )
            CV_ERROR( CV_StsBadMask,
            "This operation on multi-dimensional arrays does not support mask" );

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator ));

        sctype = type = CV_MAT_TYPE(iterator.hdr[0]->type);
        if( CV_MAT_DEPTH(sctype) < CV_32S )
            sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;
        iterator.size.width *= CV_MAT_CN(type);

        func = (CvFunc2D_2A1P)(subr_tab.fn_2d[CV_MAT_DEPTH(type)]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
       
        CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

        do
        {
            IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                             iterator.ptr[1], CV_STUB_STEP,
                             iterator.size, buf ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    sctype = type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( depth < CV_32S )
        sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;

    size = cvGetMatSize( src );

    if( !maskarr )
    {
        if( CV_IS_MAT_CONT( src->type & dst->type ))
        {
            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
            {
                int len = size.width * size.height;

                if( type == CV_32FC1 )
                {
                    const float* srcdata = (const float*)(src->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = (float)(scalar.val[0] - srcdata[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( type == CV_64FC1 )
                {
                    const double* srcdata = (const double*)(src->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = scalar.val[0] - srcdata[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }
        
        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    func = (CvFunc2D_2A1P)(subr_tab.fn_2d[depth]);
    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    src_step = src->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( func( src->data.ptr + y*src->step, src_step,
                         tdst->data.ptr, tdst_step,
                         cvSize( tsize.width*cn, tsize.height ), buf ));
        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


/******************************* A D D ********************************/

CV_IMPL void
cvAdd( const void* srcarr1, const void* srcarr2,
       void* dstarr, const void* maskarr )
{
    static CvFuncTable add_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvAdd" );

    __BEGIN__;

    int y, dy, type, depth, cn, cont_flag = 0;
    int src1_step, src2_step, dst_step, tdst_step, mask_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_3A func;
    CvFunc2D_3A1I func_sfs;
    CvCopyMaskFunc copym_func;
    CvSize size, tsize;

    if( !CV_IS_MAT(src1) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src1) || CV_IS_MATND(src2) || CV_IS_MATND(dst))
        {
            CvArr* arrs[] = { src1, src2, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            if( maskarr )
                CV_ERROR( CV_StsBadMask,
                "This operation on multi-dimensional arrays does not support mask" );

            CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

            type = iterator.hdr[0]->type;
            iterator.size.width *= CV_MAT_CN(type);

            if( !inittab )
            {
                icvInitAddC1RTable( &add_tab );
                inittab = 1;
            }

            depth = CV_MAT_DEPTH(type);
            if( depth <= CV_16S )
            {
                func_sfs = (CvFunc2D_3A1I)(add_tab.fn_2d[depth]);
                if( !func_sfs )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func_sfs( iterator.ptr[0], CV_STUB_STEP,
                                         iterator.ptr[1], CV_STUB_STEP,
                                         iterator.ptr[2], CV_STUB_STEP,
                                         iterator.size, 0 ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                func = (CvFunc2D_3A)(add_tab.fn_2d[depth]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.ptr[2], CV_STUB_STEP,
                                     iterator.size ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0, coi3 = 0;
            
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1 ));
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3 ));
            if( coi1 + coi2 + coi3 != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    if( !mask )
    {
        if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
        {
            int len = size.width*size.height*cn;

            if( len <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = (const float*)(src1->data.ptr);
                    const float* src2data = (const float*)(src2->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = (float)(src1data[len-1] + src2data[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = (const double*)(src1->data.ptr);
                    const double* src2data = (const double*)(src2->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = src1data[len-1] + src2data[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }

        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    if( !inittab )
    {
        icvInitAddC1RTable( &add_tab );
        inittab = 1;
    }

    if( depth <= CV_16S )
    {
        func = 0;
        func_sfs = (CvFunc2D_3A1I)(add_tab.fn_2d[depth]);
        if( !func_sfs )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }
    else
    {
        func_sfs = 0;
        func = (CvFunc2D_3A)(add_tab.fn_2d[depth]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src1_step = src2_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( depth <= CV_16S ?
            func_sfs( src1->data.ptr + y*src1->step, src1_step,
                      src2->data.ptr + y*src2->step, src2_step,
                      tdst->data.ptr, tdst_step,
                      cvSize( tsize.width*cn, tsize.height ), 0 ) :
            func( src1->data.ptr + y*src1->step, src1_step,
                  src2->data.ptr + y*src2->step, src2_step,
                  tdst->data.ptr, tdst_step,
                  cvSize( tsize.width*cn, tsize.height )));

        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


CV_IMPL void
cvAddS( const void* srcarr, CvScalar scalar, void* dstarr, const void* maskarr )
{
    static CvFuncTable add_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvAddS" );

    __BEGIN__;

    int sctype, y, dy, type, depth, cn, coi = 0, cont_flag = 0;
    int src_step, dst_step, tdst_step, mask_step;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_2A1P func;
    CvCopyMaskFunc copym_func;
    double buf[12];
    int is_nd = 0;
    CvSize size, tsize; 

    if( !inittab )
    {
        icvInitAddCC1RTable( &add_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src) )
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
        {
            CV_CALL( src = cvGetMat( src, &srcstub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
        {
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src, dst };
        CvMatND stubs[2];
        CvNArrayIterator iterator;

        if( maskarr )
            CV_ERROR( CV_StsBadMask,
            "This operation on multi-dimensional arrays does not support mask" );

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator ));

        sctype = type = CV_MAT_TYPE(iterator.hdr[0]->type);
        if( CV_MAT_DEPTH(sctype) < CV_32S )
            sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;
        iterator.size.width *= CV_MAT_CN(type);

        func = (CvFunc2D_2A1P)(add_tab.fn_2d[CV_MAT_DEPTH(type)]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
       
        CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

        do
        {
            IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                             iterator.ptr[1], CV_STUB_STEP,
                             iterator.size, buf ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    sctype = type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( depth < CV_32S )
        sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;

    size = cvGetMatSize( src );

    if( !maskarr )
    {
        if( CV_IS_MAT_CONT( src->type & dst->type ))
        {
            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
            {
                int len = size.width * size.height;

                if( type == CV_32FC1 )
                {
                    const float* srcdata = (const float*)(src->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = (float)(scalar.val[0] + srcdata[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( type == CV_64FC1 )
                {
                    const double* srcdata = (const double*)(src->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = scalar.val[0] + srcdata[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }
        
        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    func = (CvFunc2D_2A1P)(add_tab.fn_2d[depth]);
    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    src_step = src->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( func( src->data.ptr + y*src->step, src_step,
                         tdst->data.ptr, tdst_step,
                         cvSize( tsize.width*cn, tsize.height ), buf ));
        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


/***************************************** M U L ****************************************/

#define ICV_DEF_MUL_OP_CASE( flavor, arrtype, worktype, _cast_macro1_,                  \
                             _cast_macro2_, _cvt_macro_ )                               \
static CvStatus CV_STDCALL                                                              \
    icvMul_##flavor##_C1R( const arrtype* src1, int step1,                              \
                           const arrtype* src2, int step2,                              \
                           arrtype* dst, int step,                                      \
                           CvSize size, double scale )                                  \
{                                                                                       \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]); step /= sizeof(dst[0]);         \
                                                                                        \
    if( fabs(scale - 1.) < DBL_EPSILON )                                                \
    {                                                                                   \
        for( ; size.height--; src1+=step1, src2+=step2, dst+=step )                     \
        {                                                                               \
            int i;                                                                      \
            for( i = 0; i <= size.width - 4; i += 4 )                                   \
            {                                                                           \
                worktype t0 = src1[i] * src2[i];                                        \
                worktype t1 = src1[i+1] * src2[i+1];                                    \
                                                                                        \
                dst[i] = _cast_macro2_(t0);                                             \
                dst[i+1] = _cast_macro2_(t1);                                           \
                                                                                        \
                t0 = src1[i+2] * src2[i+2];                                             \
                t1 = src1[i+3] * src2[i+3];                                             \
                                                                                        \
                dst[i+2] = _cast_macro2_(t0);                                           \
                dst[i+3] = _cast_macro2_(t1);                                           \
            }                                                                           \
                                                                                        \
            for( ; i < size.width; i++ )                                                \
            {                                                                           \
                worktype t0 = src1[i] * src2[i];                                        \
                dst[i] = _cast_macro2_(t0);                                             \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        for( ; size.height--; src1+=step1, src2+=step2, dst+=step )                     \
        {                                                                               \
            int i;                                                                      \
            for( i = 0; i <= size.width - 4; i += 4 )                                   \
            {                                                                           \
                double ft0 = scale*_cvt_macro_(src1[i])*_cvt_macro_(src2[i]);           \
                double ft1 = scale*_cvt_macro_(src1[i+1])*_cvt_macro_(src2[i+1]);       \
                worktype t0 = _cast_macro1_(ft0);                                       \
                worktype t1 = _cast_macro1_(ft1);                                       \
                                                                                        \
                dst[i] = _cast_macro2_(t0);                                             \
                dst[i+1] = _cast_macro2_(t1);                                           \
                                                                                        \
                ft0 = scale*_cvt_macro_(src1[i+2])*_cvt_macro_(src2[i+2]);              \
                ft1 = scale*_cvt_macro_(src1[i+3])*_cvt_macro_(src2[i+3]);              \
                t0 = _cast_macro1_(ft0);                                                \
                t1 = _cast_macro1_(ft1);                                                \
                                                                                        \
                dst[i+2] = _cast_macro2_(t0);                                           \
                dst[i+3] = _cast_macro2_(t1);                                           \
            }                                                                           \
                                                                                        \
            for( ; i < size.width; i++ )                                                \
            {                                                                           \
                worktype t0;                                                            \
                t0 = _cast_macro1_(scale*_cvt_macro_(src1[i])*_cvt_macro_(src2[i]));    \
                dst[i] = _cast_macro2_(t0);                                             \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}


ICV_DEF_MUL_OP_CASE( 8u, uchar, int, cvRound, CV_CAST_8U, CV_8TO32F )
ICV_DEF_MUL_OP_CASE( 16u, ushort, int, cvRound, CV_CAST_16U, CV_NOP )
ICV_DEF_MUL_OP_CASE( 16s, short, int, cvRound, CV_CAST_16S, CV_NOP )
ICV_DEF_MUL_OP_CASE( 32s, int, int, cvRound, CV_CAST_32S, CV_NOP )
ICV_DEF_MUL_OP_CASE( 32f, float, double, CV_NOP, CV_CAST_32F, CV_NOP )
ICV_DEF_MUL_OP_CASE( 64f, double, double, CV_NOP, CV_CAST_64F, CV_NOP )


ICV_DEF_INIT_ARITHM_FUNC_TAB( Mul, C1R )


typedef CvStatus (CV_STDCALL * CvScaledElWiseFunc)( const void* src1, int step1,
                                                    const void* src2, int step2,
                                                    void* dst, int step,
                                                    CvSize size, double scale );

CV_IMPL void
cvMul( const void* srcarr1, const void* srcarr2, void* dstarr, double scale )
{
    static CvFuncTable mul_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvMul" );

    __BEGIN__;

    int type, depth, coi = 0;
    int src1_step, src2_step, dst_step;
    int is_nd = 0;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    CvScaledElWiseFunc func;

    if( !inittab )
    {
        icvInitMulC1RTable( &mul_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        if( CV_IS_MATND(src1) )
            is_nd = 1;
        else
        {
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(src2) )
    {
        if( CV_IS_MATND(src2) )
            is_nd = 1;
        else
        {
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
        {
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src1, src2, dst };
        CvMatND stubs[3];
        CvNArrayIterator iterator;

        CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

        type = iterator.hdr[0]->type;
        iterator.size.width *= CV_MAT_CN(type);

        func = (CvScaledElWiseFunc)(mul_tab.fn_2d[CV_MAT_DEPTH(type)]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        do
        {
            IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                             iterator.ptr[1], CV_STUB_STEP,
                             iterator.ptr[2], CV_STUB_STEP,
                             iterator.size, scale ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );

    depth = CV_MAT_DEPTH(type);
    size.width *= CV_MAT_CN( type );

    if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
    {
        size.width *= size.height;

        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE && scale == 1 )
        {
            if( depth == CV_32F )
            {
                const float* src1data = (const float*)(src1->data.ptr);
                const float* src2data = (const float*)(src2->data.ptr);
                float* dstdata = (float*)(dst->data.ptr);
            
                do
                {
                    dstdata[size.width-1] = (float)
                        (src1data[size.width-1] * src2data[size.width-1]);
                }
                while( --size.width );

                EXIT;
            }

            if( depth == CV_64F )
            {
                const double* src1data = (const double*)(src1->data.ptr);
                const double* src2data = (const double*)(src2->data.ptr);
                double* dstdata = (double*)(dst->data.ptr);
            
                do
                {
                    dstdata[size.width-1] =
                        src1data[size.width-1] * src2data[size.width-1];
                }
                while( --size.width );

                EXIT;
            }
        }

        src1_step = src2_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        src2_step = src2->step;
        dst_step = dst->step;
    }

    func = (CvScaledElWiseFunc)(mul_tab.fn_2d[CV_MAT_DEPTH(type)]);

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                     dst->data.ptr, dst_step, size, scale ));

    __END__;
}


/***************************************** D I V ****************************************/

#define ICV_DEF_DIV_OP_CASE( flavor, arrtype, worktype, checktype, _start_row_macro_,   \
    _cast_macro1_, _cast_macro2_, _cvt_macro_, _check_macro_, isrc )                    \
                                                                                        \
static CvStatus CV_STDCALL                                                              \
icvDiv_##flavor##_C1R( const arrtype* src1, int step1,                                  \
                       const arrtype* src2, int step2,                                  \
                       arrtype* dst, int step,                                          \
                       CvSize size, double scale )                                      \
{                                                                                       \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]); step /= sizeof(dst[0]);         \
                                                                                        \
    for( ; size.height--; src1+=step1, src2+=step2, dst+=step )                         \
    {                                                                                   \
        _start_row_macro_(checktype, src2);                                             \
        for( i = 0; i <= size.width - 4; i += 4 )                                       \
        {                                                                               \
            if( _check_macro_(isrc[i]) && _check_macro_(isrc[i+1]) &&                   \
                _check_macro_(isrc[i+2]) && _check_macro_(isrc[i+3]))                   \
            {                                                                           \
                double a = (double)_cvt_macro_(src2[i]) * _cvt_macro_(src2[i+1]);       \
                double b = (double)_cvt_macro_(src2[i+2]) * _cvt_macro_(src2[i+3]);     \
                double d = scale/(a * b);                                               \
                                                                                        \
                b *= d;                                                                 \
                a *= d;                                                                 \
                                                                                        \
                worktype z0 = _cast_macro1_(src2[i+1] * _cvt_macro_(src1[i]) * b);      \
                worktype z1 = _cast_macro1_(src2[i] * _cvt_macro_(src1[i+1]) * b);      \
                worktype z2 = _cast_macro1_(src2[i+3] * _cvt_macro_(src1[i+2]) * a);    \
                worktype z3 = _cast_macro1_(src2[i+2] * _cvt_macro_(src1[i+3]) * a);    \
                                                                                        \
                dst[i] = _cast_macro2_(z0);                                             \
                dst[i+1] = _cast_macro2_(z1);                                           \
                dst[i+2] = _cast_macro2_(z2);                                           \
                dst[i+3] = _cast_macro2_(z3);                                           \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                worktype z0 = _check_macro_(isrc[i]) ?                                  \
                   _cast_macro1_(_cvt_macro_(src1[i])*scale/_cvt_macro_(src2[i])) : 0;  \
                worktype z1 = _check_macro_(isrc[i+1]) ?                                \
                   _cast_macro1_(_cvt_macro_(src1[i+1])*scale/_cvt_macro_(src2[i+1])):0;\
                worktype z2 = _check_macro_(isrc[i+2]) ?                                \
                   _cast_macro1_(_cvt_macro_(src1[i+2])*scale/_cvt_macro_(src2[i+2])):0;\
                worktype z3 = _check_macro_(isrc[i+3]) ?                                \
                   _cast_macro1_(_cvt_macro_(src1[i+3])*scale/_cvt_macro_(src2[i+3])):0;\
                                                                                        \
                dst[i] = _cast_macro2_(z0);                                             \
                dst[i+1] = _cast_macro2_(z1);                                           \
                dst[i+2] = _cast_macro2_(z2);                                           \
                dst[i+3] = _cast_macro2_(z3);                                           \
            }                                                                           \
        }                                                                               \
                                                                                        \
        for( ; i < size.width; i++ )                                                    \
        {                                                                               \
            worktype z0 = _check_macro_(isrc[i]) ?                                      \
                _cast_macro1_(_cvt_macro_(src1[i])*scale/_cvt_macro_(src2[i])) : 0;     \
            dst[i] = _cast_macro2_(z0);                                                 \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}


#define ICV_DEF_RECIP_OP_CASE( flavor, arrtype, worktype, checktype,            \
    _start_row_macro_, _cast_macro1_, _cast_macro2_,                            \
    _cvt_macro_, _check_macro_, isrc )                                          \
                                                                                \
static CvStatus CV_STDCALL                                                      \
icvRecip_##flavor##_C1R( const arrtype* src, int step1,                         \
                         arrtype* dst, int step,                                \
                         CvSize size, double scale )                            \
{                                                                               \
    step1 /= sizeof(src[0]); step /= sizeof(dst[0]);                            \
                                                                                \
    for( ; size.height--; src+=step1, dst+=step )                               \
    {                                                                           \
        _start_row_macro_(checktype, src);                                      \
        for( i = 0; i <= size.width - 4; i += 4 )                               \
        {                                                                       \
            if( _check_macro_(isrc[i]) && _check_macro_(isrc[i+1]) &&           \
                _check_macro_(isrc[i+2]) && _check_macro_(isrc[i+3]))           \
            {                                                                   \
                double a = (double)_cvt_macro_(src[i]) * _cvt_macro_(src[i+1]); \
                double b = (double)_cvt_macro_(src[i+2]) * _cvt_macro_(src[i+3]);\
                double d = scale/(a * b);                                       \
                                                                                \
                b *= d;                                                         \
                a *= d;                                                         \
                                                                                \
                worktype z0 = _cast_macro1_(src[i+1] * b);                      \
                worktype z1 = _cast_macro1_(src[i] * b);                        \
                worktype z2 = _cast_macro1_(src[i+3] * a);                      \
                worktype z3 = _cast_macro1_(src[i+2] * a);                      \
                                                                                \
                dst[i] = _cast_macro2_(z0);                                     \
                dst[i+1] = _cast_macro2_(z1);                                   \
                dst[i+2] = _cast_macro2_(z2);                                   \
                dst[i+3] = _cast_macro2_(z3);                                   \
            }                                                                   \
            else                                                                \
            {                                                                   \
                worktype z0 = _check_macro_(isrc[i]) ?                          \
                   _cast_macro1_(scale/_cvt_macro_(src[i])) : 0;                \
                worktype z1 = _check_macro_(isrc[i+1]) ?                        \
                   _cast_macro1_(scale/_cvt_macro_(src[i+1])):0;                \
                worktype z2 = _check_macro_(isrc[i+2]) ?                        \
                   _cast_macro1_(scale/_cvt_macro_(src[i+2])):0;                \
                worktype z3 = _check_macro_(isrc[i+3]) ?                        \
                   _cast_macro1_(scale/_cvt_macro_(src[i+3])):0;                \
                                                                                \
                dst[i] = _cast_macro2_(z0);                                     \
                dst[i+1] = _cast_macro2_(z1);                                   \
                dst[i+2] = _cast_macro2_(z2);                                   \
                dst[i+3] = _cast_macro2_(z3);                                   \
            }                                                                   \
        }                                                                       \
                                                                                \
        for( ; i < size.width; i++ )                                            \
        {                                                                       \
            worktype z0 = _check_macro_(isrc[i]) ?                              \
                _cast_macro1_(scale/_cvt_macro_(src[i])) : 0;                   \
            dst[i] = _cast_macro2_(z0);                                         \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


#define div_start_row_int(checktype, divisor) \
    int i

#define div_start_row_flt(checktype, divisor) \
    const checktype* isrc = (const checktype*)divisor; int i

#define div_check_zero_flt(x)  (((x) & 0x7fffffff) != 0)
#define div_check_zero_dbl(x)  (((x) & CV_BIG_INT(0x7fffffffffffffff)) != 0)

#if defined WIN64 && defined EM64T && defined _MSC_VER && !defined CV_ICC
#pragma optimize("",off)
#endif

ICV_DEF_DIV_OP_CASE( 8u, uchar, int, uchar, div_start_row_int,
                     cvRound, CV_CAST_8U, CV_8TO32F, CV_NONZERO, src2 )

#if defined WIN64 && defined EM64T && defined _MSC_VER && !defined CV_ICC
#pragma optimize("",on)
#endif


ICV_DEF_DIV_OP_CASE( 16u, ushort, int, ushort, div_start_row_int,
                     cvRound, CV_CAST_16U, CV_CAST_64F, CV_NONZERO, src2 )
ICV_DEF_DIV_OP_CASE( 16s, short, int, short, div_start_row_int,
                     cvRound, CV_CAST_16S, CV_NOP, CV_NONZERO, src2 )
ICV_DEF_DIV_OP_CASE( 32s, int, int, int, div_start_row_int,
                     cvRound, CV_CAST_32S, CV_CAST_64F, CV_NONZERO, src2 )
ICV_DEF_DIV_OP_CASE( 32f, float, double, int, div_start_row_flt,
                     CV_NOP, CV_CAST_32F, CV_NOP, div_check_zero_flt, isrc )
ICV_DEF_DIV_OP_CASE( 64f, double, double, int64, div_start_row_flt,
                     CV_NOP, CV_CAST_64F, CV_NOP, div_check_zero_dbl, isrc )

ICV_DEF_RECIP_OP_CASE( 8u, uchar, int, uchar, div_start_row_int,
                       cvRound, CV_CAST_8U, CV_8TO32F, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 16u, ushort, int, ushort, div_start_row_int,
                       cvRound, CV_CAST_16U, CV_CAST_64F, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 16s, short, int, short, div_start_row_int,
                       cvRound, CV_CAST_16S, CV_NOP, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 32s, int, int, int, div_start_row_int,
                       cvRound, CV_CAST_32S, CV_CAST_64F, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 32f, float, double, int, div_start_row_flt,
                       CV_NOP, CV_CAST_32F, CV_NOP, div_check_zero_flt, isrc  )
ICV_DEF_RECIP_OP_CASE( 64f, double, double, int64, div_start_row_flt,
                       CV_NOP, CV_CAST_64F, CV_NOP, div_check_zero_dbl, isrc )

ICV_DEF_INIT_ARITHM_FUNC_TAB( Div, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( Recip, C1R )

typedef CvStatus (CV_STDCALL * CvRecipFunc)( const void* src, int step1,
                                             void* dst, int step,
                                             CvSize size, double scale );

CV_IMPL void
cvDiv( const void* srcarr1, const void* srcarr2, void* dstarr, double scale )
{
    static CvFuncTable div_tab;
    static CvFuncTable recip_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvDiv" );

    __BEGIN__;

    int type, coi = 0;
    int is_nd = 0;
    int src1_step, src2_step, dst_step;
    int src1_cont_flag = CV_MAT_CONT_FLAG;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;

    if( !inittab )
    {
        icvInitDivC1RTable( &div_tab );
        icvInitRecipC1RTable( &recip_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src2) )
    {
        if( CV_IS_MATND(src2))
            is_nd = 1;
        else
        {
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( src1 )
    {
        if( CV_IS_MATND(src1))
            is_nd = 1;
        else
        {
            if( !CV_IS_MAT(src1) )
            {
                CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
                if( coi != 0 )
                    CV_ERROR( CV_BadCOI, "" );
            }

            if( !CV_ARE_TYPES_EQ( src1, src2 ))
                CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

            if( !CV_ARE_SIZES_EQ( src1, src2 ))
                CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );
            src1_cont_flag = src1->type;
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst))
            is_nd = 1;
        else
        {
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { dst, src2, src1 };
        CvMatND stubs[3];
        CvNArrayIterator iterator;

        CV_CALL( cvInitNArrayIterator( 2 + (src1 != 0), arrs, 0, stubs, &iterator ));

        type = iterator.hdr[0]->type;
        iterator.size.width *= CV_MAT_CN(type);

        if( src1 )
        {
            CvScaledElWiseFunc func =
                (CvScaledElWiseFunc)(div_tab.fn_2d[CV_MAT_DEPTH(type)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[2], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.ptr[0], CV_STUB_STEP,
                                 iterator.size, scale ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        else
        {
            CvRecipFunc func = (CvRecipFunc)(recip_tab.fn_2d[CV_MAT_DEPTH(type)]);

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[1], CV_STUB_STEP,
                                 iterator.ptr[0], CV_STUB_STEP,
                                 iterator.size, scale ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src2, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src2, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src2->type);
    size = cvGetMatSize( src2 );
    size.width *= CV_MAT_CN( type );

    if( CV_IS_MAT_CONT( src1_cont_flag & src2->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = src2_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1 ? src1->step : 0;
        src2_step = src2->step;
        dst_step = dst->step;
    }

    if( src1 )
    {
        CvScaledElWiseFunc func = (CvScaledElWiseFunc)(div_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                         dst->data.ptr, dst_step, size, scale ));
    }
    else
    {
        CvRecipFunc func = (CvRecipFunc)(recip_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src2->data.ptr, src2_step,
                         dst->data.ptr, dst_step, size, scale ));
    }

    __END__;
}

/******************************* A D D   W E I G T E D ******************************/

#define ICV_DEF_ADD_WEIGHTED_OP(flavor, arrtype, worktype, load_macro,          \
                                     cast_macro1, cast_macro2)                  \
static CvStatus CV_STDCALL                                                      \
icvAddWeighted_##flavor##_C1R( const arrtype* src1, int step1, double alpha,    \
                               const arrtype* src2, int step2, double beta,     \
                               double gamma, arrtype* dst, int step, CvSize size )\
{                                                                               \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]); step /= sizeof(dst[0]); \
                                                                                \
    for( ; size.height--; src1 += step1, src2 += step2, dst += step )           \
    {                                                                           \
        int i;                                                                  \
                                                                                \
        for( i = 0; i <= size.width - 4; i += 4 )                               \
        {                                                                       \
            worktype t0 = cast_macro1(load_macro((src1)[i])*alpha +             \
                                      load_macro((src2)[i])*beta + gamma);      \
            worktype t1 = cast_macro1(load_macro((src1)[i+1])*alpha +           \
                                      load_macro((src2)[i+1])*beta + gamma);    \
                                                                                \
            (dst)[i] = cast_macro2( t0 );                                       \
            (dst)[i+1] = cast_macro2( t1 );                                     \
                                                                                \
            t0 = cast_macro1(load_macro((src1)[i+2])*alpha +                    \
                             load_macro((src2)[i+2])*beta + gamma);             \
            t1 = cast_macro1(load_macro((src1)[i+3])*alpha +                    \
                             load_macro((src2)[i+3])*beta + gamma);             \
                                                                                \
            (dst)[i+2] = cast_macro2( t0 );                                     \
            (dst)[i+3] = cast_macro2( t1 );                                     \
        }                                                                       \
                                                                                \
        for( ; i < size.width; i++ )                                            \
        {                                                                       \
            worktype t0 = cast_macro1(load_macro((src1)[i])*alpha +             \
                                      load_macro((src2)[i])*beta + gamma);      \
            (dst)[i] = cast_macro2( t0 );                                       \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


#undef shift
#define shift 14

static  CvStatus CV_STDCALL
icvAddWeighted_8u_fast_C1R( const uchar* src1, int step1, double alpha,
                            const uchar* src2, int step2, double beta,
                            double gamma, uchar* dst, int step, CvSize size )
{
    int tab1[256], tab2[256];
    double t = 0;
    int j, t0, t1, t2, t3;

    alpha *= 1 << shift;
    gamma = gamma*(1 << shift) + (1 << (shift - 1));
    beta *= 1 << shift;

    for( j = 0; j < 256; j++ )
    {
        tab1[j] = cvRound(t);
        tab2[j] = cvRound(gamma);
        t += alpha;
        gamma += beta;
    }

    t0 = (tab1[0] + tab2[0]) >> shift;
    t1 = (tab1[0] + tab2[255]) >> shift;
    t2 = (tab1[255] + tab2[0]) >> shift;
    t3 = (tab1[255] + tab2[255]) >> shift;

    if( (unsigned)(t0+256) < 768 && (unsigned)(t1+256) < 768 &&
        (unsigned)(t2+256) < 768 && (unsigned)(t3+256) < 768 )
    {
        // use faster table-based convertion back to 8u
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int i;

            for( i = 0; i <= size.width - 4; i += 4 )
            {
                t0 = CV_FAST_CAST_8U((tab1[src1[i]] + tab2[src2[i]]) >> shift);
                t1 = CV_FAST_CAST_8U((tab1[src1[i+1]] + tab2[src2[i+1]]) >> shift);

                dst[i] = (uchar)t0;
                dst[i+1] = (uchar)t1;

                t0 = CV_FAST_CAST_8U((tab1[src1[i+2]] + tab2[src2[i+2]]) >> shift);
                t1 = CV_FAST_CAST_8U((tab1[src1[i+3]] + tab2[src2[i+3]]) >> shift);

                dst[i+2] = (uchar)t0;
                dst[i+3] = (uchar)t1;
            }

            for( ; i < size.width; i++ )
            {
                t0 = CV_FAST_CAST_8U((tab1[src1[i]] + tab2[src2[i]]) >> shift);
                dst[i] = (uchar)t0;
            }
        }
    }
    else
    {
        // use universal macro for convertion back to 8u
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int i;
            
            for( i = 0; i <= size.width - 4; i += 4 )
            {
                t0 = (tab1[src1[i]] + tab2[src2[i]]) >> shift;
                t1 = (tab1[src1[i+1]] + tab2[src2[i+1]]) >> shift;

                dst[i] = CV_CAST_8U( t0 );
                dst[i+1] = CV_CAST_8U( t1 );

                t0 = (tab1[src1[i+2]] + tab2[src2[i+2]]) >> shift;
                t1 = (tab1[src1[i+3]] + tab2[src2[i+3]]) >> shift;

                dst[i+2] = CV_CAST_8U( t0 );
                dst[i+3] = CV_CAST_8U( t1 );
            }

            for( ; i < size.width; i++ )
            {
                t0 = (tab1[src1[i]] + tab2[src2[i]]) >> shift;
                dst[i] = CV_CAST_8U( t0 );
            }
        }
    }

    return CV_OK;
}


ICV_DEF_ADD_WEIGHTED_OP( 8u, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U )
ICV_DEF_ADD_WEIGHTED_OP( 16u, ushort, int, CV_NOP, cvRound, CV_CAST_16U )
ICV_DEF_ADD_WEIGHTED_OP( 16s, short, int, CV_NOP, cvRound, CV_CAST_16S )
ICV_DEF_ADD_WEIGHTED_OP( 32s, int, int, CV_NOP, cvRound, CV_CAST_32S )
ICV_DEF_ADD_WEIGHTED_OP( 32f, float, double, CV_NOP, CV_NOP, CV_CAST_32F )
ICV_DEF_ADD_WEIGHTED_OP( 64f, double, double, CV_NOP, CV_NOP, CV_CAST_64F )


ICV_DEF_INIT_ARITHM_FUNC_TAB( AddWeighted, C1R )

typedef CvStatus (CV_STDCALL *CvAddWeightedFunc)( const void* src1, int step1, double alpha,
                                                  const void* src2, int step2, double beta,
                                                  double gamma, void* dst,
                                                  int step, CvSize size );

CV_IMPL void
cvAddWeighted( const CvArr* srcAarr, double alpha,
               const CvArr* srcBarr, double beta,
               double gamma, CvArr* dstarr )
{
    static CvFuncTable addw_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvAddWeighted" );

    __BEGIN__;

    CvMat  srcA_stub, *srcA = (CvMat*)srcAarr;
    CvMat  srcB_stub, *srcB = (CvMat*)srcBarr;
    CvMat  dst_stub, *dst = (CvMat*)dstarr;
    int  coi1, coi2, coi;
    int  srcA_step, srcB_step, dst_step;
    int  type;
    CvAddWeightedFunc func;
    CvSize size;

    if( !inittab )
    {
        icvInitAddWeightedC1RTable( &addw_tab );
        inittab = 1;
    }

    CV_CALL( srcA = cvGetMat( srcA, &srcA_stub, &coi1 ));
    CV_CALL( srcB = cvGetMat( srcB, &srcB_stub, &coi2 ));
    CV_CALL( dst = cvGetMat( dst, &dst_stub, &coi ));

    if( coi1 || coi2 || coi )
        CV_ERROR( CV_BadCOI, "COI must not be set" );

    if( !CV_ARE_TYPES_EQ( srcA, srcB ) ||
        !CV_ARE_TYPES_EQ( srcA, dst ))
        CV_ERROR( CV_StsUnmatchedFormats,
        "All input/output arrays should have the same type");

    if( !CV_ARE_SIZES_EQ( srcA, srcB ) ||
        !CV_ARE_SIZES_EQ( srcA, dst ))
        CV_ERROR( CV_StsUnmatchedSizes,
        "All input/output arrays should have the same sizes");

    size = cvGetMatSize( srcA );
    type = CV_MAT_TYPE( srcA->type );
    size.width *= CV_MAT_CN( type );
    srcA_step = srcA->step;
    srcB_step = srcB->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( type & srcB->type & dst->type ))
    {
        size.width *= size.height;
        size.height = 1;
        srcA_step = srcB_step = dst_step = CV_AUTOSTEP;
    }

    if( type == CV_8UC1 && size.width * size.height >= 1024 &&
        fabs(alpha) < 256 && fabs(beta) < 256 && fabs(gamma) < 256*256 )
    {
        func = (CvAddWeightedFunc)icvAddWeighted_8u_fast_C1R;
    }
    else
    {
        func = (CvAddWeightedFunc)addw_tab.fn_2d[CV_MAT_DEPTH(type)];
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "This array type is not supported" );
    }

    IPPI_CALL( func( srcA->data.ptr, srcA_step, alpha, srcB->data.ptr, srcB_step,
                     beta, gamma, dst->data.ptr, dst_step, size ));

    __END__;
}


/* End of file. */
