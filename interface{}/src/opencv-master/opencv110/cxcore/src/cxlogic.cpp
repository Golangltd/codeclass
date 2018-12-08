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
//  CvMat logical operations: &, |, ^ ...
//
// */

#include "_cxcore.h"

/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                             Macros for logic operations                             //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////

/* //////////////////////////////////////////////////////////////////////////////////////
                                Mat op Mat
////////////////////////////////////////////////////////////////////////////////////// */


#define ICV_DEF_BIN_LOG_OP_2D( __op__, name )                                       \
IPCVAPI_IMPL( CvStatus, icv##name##_8u_C1R,                                         \
( const uchar* src1, int step1, const uchar* src2, int step2,                       \
  uchar* dst, int step, CvSize size ), (src1, step1, src2, step2, dst, step, size) )\
{                                                                                   \
    for( ; size.height--; src1 += step1, src2 += step2, dst += step )               \
    {                                                                               \
        int i = 0;                                                                  \
                                                                                    \
        if( (((size_t)src1 | (size_t)src2 | (size_t)dst) & 3) == 0 )                \
        {                                                                           \
            for( ; i <= size.width - 16; i += 16 )                                  \
            {                                                                       \
                int t0 = __op__(((const int*)(src1+i))[0], ((const int*)(src2+i))[0]);\
                int t1 = __op__(((const int*)(src1+i))[1], ((const int*)(src2+i))[1]);\
                                                                                    \
                ((int*)(dst+i))[0] = t0;                                            \
                ((int*)(dst+i))[1] = t1;                                            \
                                                                                    \
                t0 = __op__(((const int*)(src1+i))[2], ((const int*)(src2+i))[2]);  \
                t1 = __op__(((const int*)(src1+i))[3], ((const int*)(src2+i))[3]);  \
                                                                                    \
                ((int*)(dst+i))[2] = t0;                                            \
                ((int*)(dst+i))[3] = t1;                                            \
            }                                                                       \
                                                                                    \
            for( ; i <= size.width - 4; i += 4 )                                    \
            {                                                                       \
                int t = __op__(*(const int*)(src1+i), *(const int*)(src2+i));       \
                *(int*)(dst+i) = t;                                                 \
            }                                                                       \
        }                                                                           \
                                                                                    \
        for( ; i < size.width; i++ )                                                \
        {                                                                           \
            int t = __op__(((const uchar*)src1)[i],((const uchar*)src2)[i]);        \
            dst[i] = (uchar)t;                                                      \
        }                                                                           \
    }                                                                               \
                                                                                    \
    return  CV_OK;                                                                  \
}


/* //////////////////////////////////////////////////////////////////////////////////////
                                     Mat op Scalar
////////////////////////////////////////////////////////////////////////////////////// */


#define ICV_DEF_UN_LOG_OP_2D( __op__, name )                                            \
static CvStatus CV_STDCALL icv##name##_8u_CnR                                           \
( const uchar* src0, int step1, uchar* dst0, int step, CvSize size,                     \
  const uchar* scalar, int pix_size )                                                   \
{                                                                                       \
    int delta = 12*pix_size;                                                            \
                                                                                        \
    for( ; size.height--; src0 += step1, dst0 += step )                                 \
    {                                                                                   \
        const uchar* src = (const uchar*)src0;                                          \
        uchar* dst = dst0;                                                              \
        int i, len = size.width;                                                        \
                                                                                        \
        if( (((size_t)src|(size_t)dst) & 3) == 0 )                                      \
        {                                                                               \
            while( (len -= delta) >= 0 )                                                \
            {                                                                           \
                for( i = 0; i < (delta); i += 12 )                                      \
                {                                                                       \
                    int t0 = __op__(((const int*)(src+i))[0], ((const int*)(scalar+i))[0]); \
                    int t1 = __op__(((const int*)(src+i))[1], ((const int*)(scalar+i))[1]); \
                    ((int*)(dst+i))[0] = t0;                                            \
                    ((int*)(dst+i))[1] = t1;                                            \
                                                                                        \
                    t0 = __op__(((const int*)(src+i))[2], ((const int*)(scalar+i))[2]); \
                    ((int*)(dst+i))[2] = t0;                                            \
                }                                                                       \
                src += delta;                                                           \
                dst += delta;                                                           \
            }                                                                           \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            while( (len -= delta) >= 0 )                                                \
            {                                                                           \
                for( i = 0; i < (delta); i += 4 )                                       \
                {                                                                       \
                    int t0 = __op__(src[i], scalar[i]);                                 \
                    int t1 = __op__(src[i+1], scalar[i+1]);                             \
                    dst[i] = (uchar)t0;                                                 \
                    dst[i+1] = (uchar)t1;                                               \
                                                                                        \
                    t0 = __op__(src[i+2], scalar[i+2]);                                 \
                    t1 = __op__(src[i+3], scalar[i+3]);                                 \
                    dst[i+2] = (uchar)t0;                                               \
                    dst[i+3] = (uchar)t1;                                               \
                }                                                                       \
                src += delta;                                                           \
                dst += delta;                                                           \
            }                                                                           \
        }                                                                               \
                                                                                        \
        for( len += delta, i = 0; i < len; i++ )                                        \
        {                                                                               \
            int t = __op__(src[i],scalar[i]);                                           \
            dst[i] = (uchar)t;                                                          \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}

/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                LOGIC OPERATIONS                                     //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////

static void
icvLogicS( const void* srcarr, CvScalar* scalar, void* dstarr,
           const void* maskarr, CvFunc2D_2A1P1I fn_2d )
{
    uchar* buffer = 0;
    int local_alloc = 1;
    
    CV_FUNCNAME( "icvLogicS" );
    
    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvCopyMaskFunc copym_func = 0;
    
    int y, dy;
    int coi1 = 0, coi2 = 0;
    int is_nd = 0, cont_flag = 0;
    int elem_size, elem_size1, type, depth;
    double buf[12];
    CvSize size, tsize;
    int src_step, dst_step, tdst_step, mask_step;

    if( !CV_IS_MAT(src))
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
            CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    }

    if( !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));
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

        type = CV_MAT_TYPE(iterator.hdr[0]->type);
        depth = CV_MAT_DEPTH(type);
        iterator.size.width *= CV_ELEM_SIZE(type);
        elem_size1 = CV_ELEM_SIZE1(depth);

        CV_CALL( cvScalarToRawData( scalar, buf, type, 1 ));

        do
        {
            IPPI_CALL( fn_2d( iterator.ptr[0], CV_STUB_STEP,
                              iterator.ptr[1], CV_STUB_STEP,
                              iterator.size, buf, elem_size1 ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_TYPES_EQ( src, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src );
    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    elem_size = CV_ELEM_SIZE(type);
    elem_size1 = CV_ELEM_SIZE1(depth);

    if( !mask )
    {
        cont_flag = CV_IS_MAT_CONT( src->type & dst->type );
        dy = size.height;
        tdst = dst;
    }
    else
    {
        int buf_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src->type & dst->type & mask->type );
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

    src_step = src->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;
    CV_CALL( cvScalarToRawData( scalar, buf, type, 1 ));

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
        IPPI_CALL( fn_2d( src->data.ptr + y*src->step, src_step, tdst->data.ptr, tdst_step,
                          cvSize(tsize.width*elem_size, tsize.height), buf, elem_size1 ));
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


static void
icvLogic( const void* srcarr1, const void* srcarr2, void* dstarr,
          const void* maskarr, CvFunc2D_3A fn_2d )
{
    uchar* buffer = 0;
    int local_alloc = 1;

    CV_FUNCNAME( "icvLogic" );
    
    __BEGIN__;

    int y, dy;
    int coi1 = 0, coi2 = 0, coi3 = 0;
    int type, elem_size;
    int is_nd = 0, cont_flag = 0;
    CvMat  srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat  srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat  dststub,  *dst = (CvMat*)dstarr;
    CvMat  maskstub, *mask = (CvMat*)maskarr;
    CvMat  dstbuf, *tdst;
    int src1_step, src2_step, tdst_step, dst_step, mask_step;
    CvSize size, tsize;
    CvCopyMaskFunc copym_func = 0;

    if( !CV_IS_MAT(src1))
    {
        if( CV_IS_MATND(src1) )
            is_nd = 1;
        else
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1 ));
    }

    if( !CV_IS_MAT(src2))
    {
        if( CV_IS_MATND(src2) )
            is_nd = 1;
        else
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2 ));
    }

    if( !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3 ));
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src1, src2, dst };
        CvMatND stubs[3];
        CvNArrayIterator iterator;

        if( maskarr )
            CV_ERROR( CV_StsBadMask,
            "This operation on multi-dimensional arrays does not support mask" );

        CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

        type = CV_MAT_TYPE(iterator.hdr[0]->type);
        iterator.size.width *= CV_ELEM_SIZE(type);

        do
        {
            IPPI_CALL( fn_2d( iterator.ptr[0], CV_STUB_STEP,
                              iterator.ptr[1], CV_STUB_STEP,
                              iterator.ptr[2], CV_STUB_STEP,
                              iterator.size ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( coi1 != 0 || coi2 != 0 || coi3 != 0 )
        CV_ERROR_FROM_CODE( CV_BadCOI );

    if( !CV_ARE_TYPES_EQ( src1, src2 ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    if( !CV_ARE_TYPES_EQ( src1, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );
    
    if( !CV_ARE_SIZES_EQ( src1, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src1 );
    type = CV_MAT_TYPE( src1->type );
    elem_size = CV_ELEM_SIZE(type);

    if( !mask )
    {
        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type );
        dy = size.height;
        tdst = dst;
    }
    else
    {
        int buf_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
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
        IPPI_CALL( fn_2d( src1->data.ptr + y*src1->step, src1_step,
                          src2->data.ptr + y*src2->step, src2_step,
                          tdst->data.ptr, tdst_step,
                          cvSize(tsize.width*elem_size, tsize.height) ));
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

ICV_DEF_BIN_LOG_OP_2D( CV_XOR, Xor )
ICV_DEF_UN_LOG_OP_2D( CV_XOR, XorC )

ICV_DEF_BIN_LOG_OP_2D( CV_AND, And )
ICV_DEF_UN_LOG_OP_2D( CV_AND, AndC )

ICV_DEF_BIN_LOG_OP_2D( CV_OR, Or )
ICV_DEF_UN_LOG_OP_2D( CV_OR, OrC )


/////////////////////////////////////////////////////////////////////////////////////////
//                                    X O R                                            //
/////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvXorS( const void* src, CvScalar scalar, void* dst, const void* mask )
{
    icvLogicS( src, &scalar, dst, mask, (CvFunc2D_2A1P1I)icvXorC_8u_CnR );
}


CV_IMPL void
cvXor( const void* src1, const void* src2, void* dst, const void* mask )
{
    icvLogic( src1, src2, dst, mask, (CvFunc2D_3A)icvXor_8u_C1R );
}

/////////////////////////////////////////////////////////////////////////////////////////
//                                    A N D                                            //
/////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvAndS( const void* src, CvScalar scalar, void* dst, const void* mask )
{
    icvLogicS( src, &scalar, dst, mask, (CvFunc2D_2A1P1I)icvAndC_8u_CnR );
}


CV_IMPL void
cvAnd( const void* src1, const void* src2, void* dst, const void* mask )
{
    icvLogic( src1, src2, dst, mask, (CvFunc2D_3A)icvAnd_8u_C1R );
}


/////////////////////////////////////////////////////////////////////////////////////////
//                                      O R                                            //
/////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvOrS( const void* src, CvScalar scalar, void* dst, const void* mask )
{
    icvLogicS( src, &scalar, dst, mask, (CvFunc2D_2A1P1I)icvOrC_8u_CnR );
}


CV_IMPL void
cvOr( const void* src1, const void* src2, void* dst, const void* mask )
{
    icvLogic( src1, src2, dst, mask, (CvFunc2D_3A)icvOr_8u_C1R );
}


/////////////////////////////////////////////////////////////////////////////////////////
//                                      N O T                                          //
/////////////////////////////////////////////////////////////////////////////////////////


IPCVAPI_IMPL( CvStatus, icvNot_8u_C1R,
( const uchar* src1, int step1, uchar* dst, int step, CvSize size ),
  (src1, step1, dst, step, size) )
{
    for( ; size.height--; src1 += step1, dst += step )
    {
        int i = 0;

        if( (((size_t)src1 | (size_t)dst) & 3) == 0 )
        {
            for( ; i <= size.width - 16; i += 16 )
            {
                int t0 = ~((const int*)(src1+i))[0];
                int t1 = ~((const int*)(src1+i))[1];

                ((int*)(dst+i))[0] = t0;
                ((int*)(dst+i))[1] = t1;

                t0 = ~((const int*)(src1+i))[2];
                t1 = ~((const int*)(src1+i))[3];

                ((int*)(dst+i))[2] = t0;
                ((int*)(dst+i))[3] = t1;
            }

            for( ; i <= size.width - 4; i += 4 )
            {
                int t = ~*(const int*)(src1+i);
                *(int*)(dst+i) = t;
            }
        }

        for( ; i < size.width; i++ )
        {
            int t = ~((const uchar*)src1)[i];
            dst[i] = (uchar)t;
        }
    }

    return  CV_OK;
}


CV_IMPL void
cvNot( const void* srcarr, void* dstarr )
{
    CV_FUNCNAME( "cvNot" );
    
    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    
    int coi1 = 0, coi2 = 0;
    int type, is_nd = 0;
    CvSize size;
    int src_step, dst_step;

    if( !CV_IS_MAT(src))
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
            CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    }

    if( !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src, dst };
        CvMatND stubs[2];
        CvNArrayIterator iterator;

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator ));

        type = CV_MAT_TYPE(iterator.hdr[0]->type);
        iterator.size.width *= CV_ELEM_SIZE(type);

        do
        {
            IPPI_CALL( icvNot_8u_C1R( iterator.ptr[0], CV_STUB_STEP,
                                      iterator.ptr[1], CV_STUB_STEP,
                                      iterator.size ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_TYPES_EQ( src, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src );
    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }

    type = CV_MAT_TYPE( src->type );
    size.width *= CV_ELEM_SIZE(type);

    IPPI_CALL( icvNot_8u_C1R( src->data.ptr, src_step, dst->data.ptr, dst_step, size ));

    __END__;
}

/* End of file. */
