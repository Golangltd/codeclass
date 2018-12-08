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
//  CvMat basic operations: cvCopy, cvSet
//
// */

#include "_cxcore.h"

/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                  L/L COPY & SET FUNCTIONS                           //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


IPCVAPI_IMPL( CvStatus, icvCopy_8u_C1R, ( const uchar* src, int srcstep,
                                          uchar* dst, int dststep, CvSize size ),
                                          (src, srcstep, dst, dststep, size) )
{
    for( ; size.height--; src += srcstep, dst += dststep )
        memcpy( dst, src, size.width );

    return  CV_OK;
}


static CvStatus CV_STDCALL
icvSet_8u_C1R( uchar* dst, int dst_step, CvSize size,
               const void* scalar, int pix_size )
{
    int copy_len = 12*pix_size;
    uchar* dst_limit = dst + size.width;
    
    if( size.height-- )
    {
        while( dst + copy_len <= dst_limit )
        {
            memcpy( dst, scalar, copy_len );
            dst += copy_len;
        }

        memcpy( dst, scalar, dst_limit - dst );
    }

    if( size.height )
    {
        dst = dst_limit - size.width + dst_step;

        for( ; size.height--; dst += dst_step )
            memcpy( dst, dst - dst_step, size.width );
    }

    return CV_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                L/L COPY WITH MASK FUNCTIONS                         //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


#define ICV_DEF_COPY_MASK_C1_CASE( type )   \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
        if( mask[i+1] )                     \
            dst[i+1] = src[i+1];            \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
    }

#define ICV_DEF_COPY_MASK_C3_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*3];             \
            type t1 = src[i*3+1];           \
            type t2 = src[i*3+2];           \
                                            \
            dst[i*3] = t0;                  \
            dst[i*3+1] = t1;                \
            dst[i*3+2] = t2;                \
        }



#define ICV_DEF_COPY_MASK_C4_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*4];             \
            type t1 = src[i*4+1];           \
            dst[i*4] = t0;                  \
            dst[i*4+1] = t1;                \
                                            \
            t0 = src[i*4+2];                \
            t1 = src[i*4+3];                \
            dst[i*4+2] = t0;                \
            dst[i*4+3] = t1;                \
        }


#define ICV_DEF_COPY_MASK_2D( name, type, cn )              \
IPCVAPI_IMPL( CvStatus,                                     \
name,( const type* src, int srcstep, type* dst, int dststep,\
       CvSize size, const uchar* mask, int maskstep ),      \
       (src, srcstep, dst, dststep, size, mask, maskstep))  \
{                                                           \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);   \
    for( ; size.height--; src += srcstep,                   \
            dst += dststep, mask += maskstep )              \
    {                                                       \
        int i;                                              \
        ICV_DEF_COPY_MASK_C##cn##_CASE( type )              \
    }                                                       \
                                                            \
    return  CV_OK;                                          \
}


#define ICV_DEF_SET_MASK_C1_CASE( type )    \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
        if( mask[i+1] )                     \
            dst[i+1] = s0;                  \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
    }


#define ICV_DEF_SET_MASK_C3_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*3] = s0;                  \
            dst[i*3+1] = s1;                \
            dst[i*3+2] = s2;                \
        }

#define ICV_DEF_SET_MASK_C4_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*4] = s0;                  \
            dst[i*4+1] = s1;                \
            dst[i*4+2] = s2;                \
            dst[i*4+3] = s3;                \
        }

#define ICV_DEF_SET_MASK_2D( name, type, cn )       \
IPCVAPI_IMPL( CvStatus,                             \
name,( type* dst, int dststep,                      \
       const uchar* mask, int maskstep,             \
       CvSize size, const type* scalar ),           \
       (dst, dststep, mask, maskstep, size, scalar))\
{                                                   \
    CV_UN_ENTRY_C##cn( type );                      \
    dststep /= sizeof(dst[0]);                      \
                                                    \
    for( ; size.height--; mask += maskstep,         \
                          dst += dststep )          \
    {                                               \
        int i;                                      \
        ICV_DEF_SET_MASK_C##cn##_CASE( type )       \
    }                                               \
                                                    \
    return CV_OK;                                   \
}


ICV_DEF_SET_MASK_2D( icvSet_8u_C1MR, uchar, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C1MR, ushort, 1 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C3MR, uchar, 3 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C4MR, int, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C3MR, ushort, 3 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C4MR, int64, 1 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C3MR, int, 3 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C4MR, int, 4 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C3MR, int64, 3 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C4MR, int64, 4 )

ICV_DEF_COPY_MASK_2D( icvCopy_8u_C1MR, uchar, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C1MR, ushort, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C3MR, uchar, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C4MR, int, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C3MR, ushort, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C4MR, int64, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C3MR, int, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C4MR, int, 4 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C3MR, int64, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C4MR, int64, 4 )

#define CV_DEF_INIT_COPYSET_TAB_2D( FUNCNAME, FLAG )                \
static void icvInit##FUNCNAME##FLAG##Table( CvBtFuncTable* table )  \
{                                                                   \
    table->fn_2d[1]  = (void*)icv##FUNCNAME##_8u_C1##FLAG;          \
    table->fn_2d[2]  = (void*)icv##FUNCNAME##_16s_C1##FLAG;         \
    table->fn_2d[3]  = (void*)icv##FUNCNAME##_8u_C3##FLAG;          \
    table->fn_2d[4]  = (void*)icv##FUNCNAME##_8u_C4##FLAG;          \
    table->fn_2d[6]  = (void*)icv##FUNCNAME##_16s_C3##FLAG;         \
    table->fn_2d[8]  = (void*)icv##FUNCNAME##_16s_C4##FLAG;         \
    table->fn_2d[12] = (void*)icv##FUNCNAME##_32f_C3##FLAG;         \
    table->fn_2d[16] = (void*)icv##FUNCNAME##_32f_C4##FLAG;         \
    table->fn_2d[24] = (void*)icv##FUNCNAME##_64s_C3##FLAG;         \
    table->fn_2d[32] = (void*)icv##FUNCNAME##_64s_C4##FLAG;         \
}

CV_DEF_INIT_COPYSET_TAB_2D( Set, MR )
CV_DEF_INIT_COPYSET_TAB_2D( Copy, MR )

/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                H/L COPY & SET FUNCTIONS                             //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


CvCopyMaskFunc
icvGetCopyMaskFunc( int elem_size )
{
    static CvBtFuncTable copym_tab;
    static int inittab = 0;

    if( !inittab )
    {
        icvInitCopyMRTable( &copym_tab );
        inittab = 1;
    }
    return (CvCopyMaskFunc)copym_tab.fn_2d[elem_size];
}


/* dst = src */
CV_IMPL void
cvCopy( const void* srcarr, void* dstarr, const void* maskarr )
{
    CV_FUNCNAME( "cvCopy" );
    
    __BEGIN__;

    int pix_size;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;

    if( !CV_IS_MAT(src) || !CV_IS_MAT(dst) )
    {
        if( CV_IS_SPARSE_MAT(src) && CV_IS_SPARSE_MAT(dst))
        {
            CvSparseMat* src1 = (CvSparseMat*)src;
            CvSparseMat* dst1 = (CvSparseMat*)dst;
            CvSparseMatIterator iterator;
            CvSparseNode* node;

            dst1->dims = src1->dims;
            memcpy( dst1->size, src1->size, src1->dims*sizeof(src1->size[0]));
            dst1->valoffset = src1->valoffset;
            dst1->idxoffset = src1->idxoffset;
            cvClearSet( dst1->heap );

            if( src1->heap->active_count >= dst1->hashsize*CV_SPARSE_HASH_RATIO )
            {
                CV_CALL( cvFree( &dst1->hashtable ));
                dst1->hashsize = src1->hashsize;
                CV_CALL( dst1->hashtable =
                    (void**)cvAlloc( dst1->hashsize*sizeof(dst1->hashtable[0])));
            }

            memset( dst1->hashtable, 0, dst1->hashsize*sizeof(dst1->hashtable[0]));

            for( node = cvInitSparseMatIterator( src1, &iterator );
                 node != 0; node = cvGetNextSparseNode( &iterator ))
            {
                CvSparseNode* node_copy = (CvSparseNode*)cvSetNew( dst1->heap );
                int tabidx = node->hashval & (dst1->hashsize - 1);
                CV_MEMCPY_AUTO( node_copy, node, dst1->heap->elem_size );
                node_copy->next = (CvSparseNode*)dst1->hashtable[tabidx];
                dst1->hashtable[tabidx] = node_copy;
            }
            EXIT;
        }
        else if( CV_IS_MATND(src) || CV_IS_MATND(dst) )
        {
            CvArr* arrs[] = { src, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            CV_CALL( cvInitNArrayIterator( 2, arrs, maskarr, stubs, &iterator ));
            pix_size = CV_ELEM_SIZE(iterator.hdr[0]->type);

            if( !maskarr )
            {
                iterator.size.width *= pix_size;
                if( iterator.size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double))
                {
                    do
                    {
                        memcpy( iterator.ptr[1], iterator.ptr[0], iterator.size.width );
                    }
                    while( cvNextNArraySlice( &iterator ));
                }
                else
                {
                    do
                    {
                        icvCopy_8u_C1R( iterator.ptr[0], CV_STUB_STEP,
                                        iterator.ptr[1], CV_STUB_STEP, iterator.size );
                    }
                    while( cvNextNArraySlice( &iterator ));
                }
            }
            else
            {
                CvCopyMaskFunc func = icvGetCopyMaskFunc( pix_size );
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    func( iterator.ptr[0], CV_STUB_STEP,
                          iterator.ptr[1], CV_STUB_STEP,
                          iterator.size,
                          iterator.ptr[2], CV_STUB_STEP );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0;
            CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

            if( coi1 )
            {
                CvArr* planes[] = { 0, 0, 0, 0 };

                if( maskarr )
                    CV_ERROR( CV_StsBadArg, "COI + mask are not supported" );

                planes[coi1-1] = dst;
                CV_CALL( cvSplit( src, planes[0], planes[1], planes[2], planes[3] ));
                EXIT;
            }
            else if( coi2 )
            {
                CvArr* planes[] = { 0, 0, 0, 0 };
            
                if( maskarr )
                    CV_ERROR( CV_StsBadArg, "COI + mask are not supported" );

                planes[coi2-1] = src;
                CV_CALL( cvMerge( planes[0], planes[1], planes[2], planes[3], dst ));
                EXIT;
            }
        }
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src );
    pix_size = CV_ELEM_SIZE(src->type);

    if( !maskarr )
    {
        int src_step = src->step, dst_step = dst->step;
        size.width *= pix_size;
        if( CV_IS_MAT_CONT( src->type & dst->type ) && (src_step == dst_step) && (src_step == src->width * pix_size))
        {
            size.width *= size.height;

            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE*
                              CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double))
            {
                memcpy( dst->data.ptr, src->data.ptr, size.width );
                EXIT;
            }

            size.height = 1;
            src_step = dst_step = CV_STUB_STEP;
        }

        if( src->data.ptr != dst->data.ptr )
            icvCopy_8u_C1R( src->data.ptr, src_step,
                            dst->data.ptr, dst_step, size );
    }
    else
    {
        CvCopyMaskFunc func = icvGetCopyMaskFunc(pix_size);
        CvMat maskstub, *mask = (CvMat*)maskarr;
        int src_step = src->step;
        int dst_step = dst->step;
        int mask_step;

        if( !CV_IS_MAT( mask ))
            CV_CALL( mask = cvGetMat( mask, &maskstub ));
        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( src, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;
        
        if( CV_IS_MAT_CONT( src->type & dst->type & mask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            src_step = dst_step = mask_step = CV_STUB_STEP;
        }

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step, dst->data.ptr, dst_step,
                         size, mask->data.ptr, mask_step ));
    }

    __END__;
}


/* dst(idx) = value */
CV_IMPL void
cvSet( void* arr, CvScalar value, const void* maskarr )
{
    static CvBtFuncTable setm_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvSet" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    int pix_size, type;
    double buf[12];
    int mat_step;
    CvSize size;

    if( !value.val[0] && !value.val[1] &&
        !value.val[2] && !value.val[3] && !maskarr )
    {
        cvZero( arr );
        EXIT;
    }

    if( !CV_IS_MAT(mat))
    {
        if( CV_IS_MATND(mat))
        {
            CvMatND nstub;
            CvNArrayIterator iterator;
            int pix_size1;
            
            CV_CALL( cvInitNArrayIterator( 1, &arr, maskarr, &nstub, &iterator ));

            type = CV_MAT_TYPE(iterator.hdr[0]->type);
            pix_size1 = CV_ELEM_SIZE1(type);
            pix_size = pix_size1*CV_MAT_CN(type);

            CV_CALL( cvScalarToRawData( &value, buf, type, maskarr == 0 ));

            if( !maskarr )
            {
                iterator.size.width *= pix_size;
                do
                {
                    icvSet_8u_C1R( iterator.ptr[0], CV_STUB_STEP,
                                   iterator.size, buf, pix_size1 );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_2A1P func = (CvFunc2D_2A1P)(setm_tab.fn_2d[pix_size]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    func( iterator.ptr[0], CV_STUB_STEP,
                          iterator.ptr[1], CV_STUB_STEP,
                          iterator.size, buf );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }    
        else
        {
            int coi = 0;
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    type = CV_MAT_TYPE( mat->type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( mat );
    mat_step = mat->step;

    if( !maskarr )
    {
        if( CV_IS_MAT_CONT( mat->type ))
        {
            size.width *= size.height;
        
            if( size.width <= (int)(CV_MAX_INLINE_MAT_OP_SIZE*sizeof(double)))
            {
                if( type == CV_32FC1 )
                {
                    float* dstdata = (float*)(mat->data.ptr);
                    float val = (float)value.val[0];

                    do
                    {
                        dstdata[size.width-1] = val;
                    }
                    while( --size.width );

                    EXIT;
                }

                if( type == CV_64FC1 )
                {
                    double* dstdata = (double*)(mat->data.ptr);
                    double val = value.val[0];

                    do
                    {
                        dstdata[size.width-1] = val;
                    }
                    while( --size.width );

                    EXIT;
                }
            }

            mat_step = CV_STUB_STEP;
            size.height = 1;
        }
        
        size.width *= pix_size;
        CV_CALL( cvScalarToRawData( &value, buf, type, 1 ));

        IPPI_CALL( icvSet_8u_C1R( mat->data.ptr, mat_step, size, buf,
                                  CV_ELEM_SIZE1(type)));
    }
    else
    {
        CvFunc2D_2A1P func;
        CvMat maskstub, *mask = (CvMat*)maskarr;
        int mask_step;

        CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !inittab )
        {
            icvInitSetMRTable( &setm_tab );
            inittab = 1;
        }

        if( !CV_ARE_SIZES_EQ( mat, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & mask->type ))
        {
            size.width *= size.height;
            mat_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        func = (CvFunc2D_2A1P)(setm_tab.fn_2d[pix_size]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( cvScalarToRawData( &value, buf, type, 0 ));

        IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr,
                         mask_step, size, buf ));
    }

    __END__;
}


/****************************************************************************************\
*                                          Clearing                                      *
\****************************************************************************************/

icvSetByte_8u_C1R_t icvSetByte_8u_C1R_p = 0;

CvStatus CV_STDCALL
icvSetZero_8u_C1R( uchar* dst, int dststep, CvSize size )
{
    if( size.width + size.height > 256 && icvSetByte_8u_C1R_p )
        return icvSetByte_8u_C1R_p( 0, dst, dststep, size );

    for( ; size.height--; dst += dststep )
        memset( dst, 0, size.width );

    return CV_OK;
}

CV_IMPL void
cvSetZero( CvArr* arr )
{
    CV_FUNCNAME( "cvSetZero" );
    
    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    CvSize size;
    int mat_step;

    if( !CV_IS_MAT( mat ))
    {
        if( CV_IS_MATND(mat))
        {
            CvMatND nstub;
            CvNArrayIterator iterator;
            
            CV_CALL( cvInitNArrayIterator( 1, &arr, 0, &nstub, &iterator ));
            iterator.size.width *= CV_ELEM_SIZE(iterator.hdr[0]->type);

            if( iterator.size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double) )
            {
                do
                {
                    memset( iterator.ptr[0], 0, iterator.size.width );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                do
                {
                    icvSetZero_8u_C1R( iterator.ptr[0], CV_STUB_STEP, iterator.size );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }    
        else if( CV_IS_SPARSE_MAT(mat))
        {
            CvSparseMat* mat1 = (CvSparseMat*)mat;
            cvClearSet( mat1->heap );
            if( mat1->hashtable )
                memset( mat1->hashtable, 0, mat1->hashsize*sizeof(mat1->hashtable[0]));
            EXIT;
        }
        else
        {
            int coi = 0;
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "coi is not supported" );
        }
    }

    size = cvGetMatSize( mat );
    size.width *= CV_ELEM_SIZE(mat->type);
    mat_step = mat->step;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;

        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double) )
        {
            memset( mat->data.ptr, 0, size.width );
            EXIT;
        }

        mat_step = CV_STUB_STEP;
        size.height = 1;
    }

    IPPI_CALL( icvSetZero_8u_C1R( mat->data.ptr, mat_step, size ));

    __END__;
}


/****************************************************************************************\
*                                          Flipping                                      *
\****************************************************************************************/

#define ICV_DEF_FLIP_HZ_CASE_C1( type ) \
    for( i = 0; i < (len+1)/2; i++ )    \
    {                                   \
        type t0 = src[i];               \
        type t1 = src[len - i - 1];     \
        dst[i] = t1;                    \
        dst[len - i - 1] = t0;          \
    }


#define ICV_DEF_FLIP_HZ_CASE_C3( type ) \
    for( i = 0; i < (len+1)/2; i++ )    \
    {                                   \
        type t0 = src[i*3];             \
        type t1 = src[(len - i)*3 - 3]; \
        dst[i*3] = t1;                  \
        dst[(len - i)*3 - 3] = t0;      \
        t0 = src[i*3 + 1];              \
        t1 = src[(len - i)*3 - 2];      \
        dst[i*3 + 1] = t1;              \
        dst[(len - i)*3 - 2] = t0;      \
        t0 = src[i*3 + 2];              \
        t1 = src[(len - i)*3 - 1];      \
        dst[i*3 + 2] = t1;              \
        dst[(len - i)*3 - 1] = t0;      \
    }


#define ICV_DEF_FLIP_HZ_CASE_C4( type ) \
    for( i = 0; i < (len+1)/2; i++ )    \
    {                                   \
        type t0 = src[i*4];             \
        type t1 = src[(len - i)*4 - 4]; \
        dst[i*4] = t1;                  \
        dst[(len - i)*4 - 4] = t0;      \
        t0 = src[i*4 + 1];              \
        t1 = src[(len - i)*4 - 3];      \
        dst[i*4 + 1] = t1;              \
        dst[(len - i)*4 - 3] = t0;      \
        t0 = src[i*4 + 2];              \
        t1 = src[(len - i)*4 - 2];      \
        dst[i*4 + 2] = t1;              \
        dst[(len - i)*4 - 2] = t0;      \
        t0 = src[i*4 + 3];              \
        t1 = src[(len - i)*4 - 1];      \
        dst[i*4 + 3] = t1;              \
        dst[(len - i)*4 - 1] = t0;      \
    }


#define ICV_DEF_FLIP_HZ_FUNC( flavor, arrtype, cn )                 \
static CvStatus CV_STDCALL                                          \
icvFlipHorz_##flavor( const arrtype* src, int srcstep,              \
                      arrtype* dst, int dststep, CvSize size )      \
{                                                                   \
    int i, len = size.width;                                        \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);           \
                                                                    \
    for( ; size.height--; src += srcstep, dst += dststep )          \
    {                                                               \
        ICV_DEF_FLIP_HZ_CASE_C##cn( arrtype )                       \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}


ICV_DEF_FLIP_HZ_FUNC( 8u_C1R, uchar, 1 )
ICV_DEF_FLIP_HZ_FUNC( 8u_C2R, ushort, 1 )
ICV_DEF_FLIP_HZ_FUNC( 8u_C3R, uchar, 3 )
ICV_DEF_FLIP_HZ_FUNC( 16u_C2R, int, 1 )
ICV_DEF_FLIP_HZ_FUNC( 16u_C3R, ushort, 3 )
ICV_DEF_FLIP_HZ_FUNC( 32s_C2R, int64, 1 )
ICV_DEF_FLIP_HZ_FUNC( 32s_C3R, int, 3 )
ICV_DEF_FLIP_HZ_FUNC( 64s_C2R, int, 4 )
ICV_DEF_FLIP_HZ_FUNC( 64s_C3R, int64, 3 )
ICV_DEF_FLIP_HZ_FUNC( 64s_C4R, int64, 4 )

CV_DEF_INIT_PIXSIZE_TAB_2D( FlipHorz, R )


static CvStatus
icvFlipVert_8u_C1R( const uchar* src, int srcstep,
                    uchar* dst, int dststep, CvSize size )
{
    int y, i;
    const uchar* src1 = src + (size.height - 1)*srcstep;
    uchar* dst1 = dst + (size.height - 1)*dststep;

    for( y = 0; y < (size.height + 1)/2; y++, src += srcstep, src1 -= srcstep,
                                              dst += dststep, dst1 -= dststep )
    {
        i = 0;
        if( ((size_t)(src)|(size_t)(dst)|(size_t)src1|(size_t)dst1) % sizeof(int) == 0 )
        {
            for( ; i <= size.width - 16; i += 16 )
            {
                int t0 = ((int*)(src + i))[0];
                int t1 = ((int*)(src1 + i))[0];

                ((int*)(dst + i))[0] = t1;
                ((int*)(dst1 + i))[0] = t0;

                t0 = ((int*)(src + i))[1];
                t1 = ((int*)(src1 + i))[1];

                ((int*)(dst + i))[1] = t1;
                ((int*)(dst1 + i))[1] = t0;

                t0 = ((int*)(src + i))[2];
                t1 = ((int*)(src1 + i))[2];

                ((int*)(dst + i))[2] = t1;
                ((int*)(dst1 + i))[2] = t0;

                t0 = ((int*)(src + i))[3];
                t1 = ((int*)(src1 + i))[3];

                ((int*)(dst + i))[3] = t1;
                ((int*)(dst1 + i))[3] = t0;
            }

            for( ; i <= size.width - 4; i += 4 )
            {
                int t0 = ((int*)(src + i))[0];
                int t1 = ((int*)(src1 + i))[0];

                ((int*)(dst + i))[0] = t1;
                ((int*)(dst1 + i))[0] = t0;
            }
        }

        for( ; i < size.width; i++ )
        {
            uchar t0 = src[i];
            uchar t1 = src1[i];

            dst[i] = t1;
            dst1[i] = t0;
        }
    }

    return CV_OK;
}


CV_IMPL void
cvFlip( const CvArr* srcarr, CvArr* dstarr, int flip_mode )
{
    static CvBtFuncTable tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvFlip" );
    
    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize size;
    CvFunc2D_2A func = 0;
    int pix_size;

    if( !inittab )
    {
        icvInitFlipHorzRTable( &tab );
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !dst )
        dst = src;
    else if( !CV_IS_MAT( dst ))
    {
        int coi = 0;
        CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( src );
    pix_size = CV_ELEM_SIZE( src->type );

    if( flip_mode == 0 )
    {
        size.width *= pix_size;
        
        IPPI_CALL( icvFlipVert_8u_C1R( src->data.ptr, src->step,
                                       dst->data.ptr, dst->step, size ));
    }
    else
    {
        int inplace = src->data.ptr == dst->data.ptr;
        uchar* dst_data = dst->data.ptr;
        int dst_step = dst->step;

        func = (CvFunc2D_2A)(tab.fn_2d[pix_size]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( flip_mode < 0 && !inplace )
        {
            dst_data += dst_step * (dst->height - 1);
            dst_step = -dst_step;
        }

        IPPI_CALL( func( src->data.ptr, src->step, dst_data, dst_step, size ));
        
        if( flip_mode < 0 && inplace )
        {
            size.width *= pix_size;
            IPPI_CALL( icvFlipVert_8u_C1R( dst->data.ptr, dst->step,
                                           dst->data.ptr, dst->step, size ));
        }
    }

    __END__;
}


CV_IMPL void
cvRepeat( const CvArr* srcarr, CvArr* dstarr )
{
    CV_FUNCNAME( "cvRepeat" );
    
    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize srcsize, dstsize;
    int pix_size;
    int x, y, k, l;

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !CV_IS_MAT( dst ))
    {
        int coi = 0;
        CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    srcsize = cvGetMatSize( src );
    dstsize = cvGetMatSize( dst );
    pix_size = CV_ELEM_SIZE( src->type );

    for( y = 0, k = 0; y < dstsize.height; y++ )
    {
        for( x = 0; x < dstsize.width; x += srcsize.width )
        {
            l = srcsize.width;
            if( l > dstsize.width - x )
                l = dstsize.width - x;
            memcpy( dst->data.ptr + y*dst->step + x*pix_size,
                    src->data.ptr + k*src->step, l*pix_size );
        }
        if( ++k == srcsize.height )
            k = 0;
    }

    __END__;
}

/* End of file. */

