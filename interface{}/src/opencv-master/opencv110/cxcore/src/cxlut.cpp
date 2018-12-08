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
*                                    LUT Transform                                       *
\****************************************************************************************/

#define ICV_LUT_CASE_C1( type )             \
    for( i = 0; i <= size.width-4; i += 4 ) \
    {                                       \
        type t0 = lut[src[i]];              \
        type t1 = lut[src[i+1]];            \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
                                            \
        t0 = lut[src[i+2]];                 \
        t1 = lut[src[i+3]];                 \
        dst[i+2] = t0;                      \
        dst[i+3] = t1;                      \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        type t0 = lut[src[i]];              \
        dst[i] = t0;                        \
    }


#define ICV_LUT_CASE_C2( type )             \
    for( i = 0; i < size.width; i += 2 )    \
    {                                       \
        type t0 = lut[src[i]*2];            \
        type t1 = lut[src[i+1]*2 + 1];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
    }

#define ICV_LUT_CASE_C3( type )             \
    for( i = 0; i < size.width; i += 3 )    \
    {                                       \
        type t0 = lut[src[i]*3];            \
        type t1 = lut[src[i+1]*3 + 1];      \
        type t2 = lut[src[i+2]*3 + 2];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
        dst[i+2] = t2;                      \
    }

#define ICV_LUT_CASE_C4( type )             \
    for( i = 0; i < size.width; i += 4 )    \
    {                                       \
        type t0 = lut[src[i]*4];            \
        type t1 = lut[src[i+1]*4 + 1];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
        t0 = lut[src[i+2]*4 + 2];           \
        t1 = lut[src[i+3]*4 + 3];           \
        dst[i+2] = t0;                      \
        dst[i+3] = t1;                      \
    }


#define  ICV_DEF_LUT_FUNC_8U_CN( flavor, dsttype, cn )      \
CvStatus CV_STDCALL icvLUT_Transform8u_##flavor##_C##cn##R( \
    const uchar* src, int srcstep,                          \
    dsttype* dst, int dststep, CvSize size,                 \
    const dsttype* lut )                                    \
{                                                           \
    size.width *= cn;                                       \
    dststep /= sizeof(dst[0]);                              \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        int i;                                              \
        ICV_LUT_CASE_C##cn( dsttype )                       \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 16u, ushort, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 32s, int, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 64f, double, 1 )

ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 2 )
ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 3 )
ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 4 )


#define  ICV_DEF_LUT_FUNC_8U( flavor, dsttype )             \
static CvStatus CV_STDCALL                                  \
icvLUT_Transform8u_##flavor##_CnR(                          \
    const uchar* src, int srcstep,                          \
    dsttype* dst, int dststep, CvSize size,                 \
    const dsttype* _lut, int cn )                           \
{                                                           \
    int max_block_size = (1 << 10)*cn;                      \
    dsttype lutp[1024];                                     \
    int i, k;                                               \
                                                            \
    size.width *= cn;                                       \
    dststep /= sizeof(dst[0]);                              \
                                                            \
    if( size.width*size.height < 256 )                      \
    {                                                       \
        for( ; size.height--; src+=srcstep, dst+=dststep )  \
            for( k = 0; k < cn; k++ )                       \
                for( i = 0; i < size.width; i += cn )       \
                    dst[i+k] = _lut[src[i+k]*cn+k];         \
        return CV_OK;                                       \
    }                                                       \
                                                            \
    /* repack the lut to planar layout */                   \
    for( k = 0; k < cn; k++ )                               \
        for( i = 0; i < 256; i++ )                          \
            lutp[i+k*256] = _lut[i*cn+k];                   \
                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        for( i = 0; i < size.width; )                       \
        {                                                   \
            int j, limit = MIN(size.width,i+max_block_size);\
            for( k=0; k<cn; k++, src++, dst++ )             \
            {                                               \
                const dsttype* lut = lutp + k*256;          \
                for( j = i; j <= limit - cn*2; j += cn*2 )  \
                {                                           \
                    dsttype t0 = lut[src[j]];               \
                    dsttype t1 = lut[src[j+cn]];            \
                    dst[j] = t0; dst[j+cn] = t1;            \
                }                                           \
                                                            \
                for( ; j < limit; j += cn )                 \
                    dst[j] = lut[src[j]];                   \
            }                                               \
            src -= cn;                                      \
            dst -= cn;                                      \
            i += limit;                                     \
        }                                                   \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}

ICV_DEF_LUT_FUNC_8U( 8u, uchar )
ICV_DEF_LUT_FUNC_8U( 16u, ushort )
ICV_DEF_LUT_FUNC_8U( 32s, int )
ICV_DEF_LUT_FUNC_8U( 64f, double )

#undef   icvLUT_Transform8u_8s_C1R
#undef   icvLUT_Transform8u_16s_C1R
#undef   icvLUT_Transform8u_32f_C1R

#define  icvLUT_Transform8u_8s_C1R    icvLUT_Transform8u_8u_C1R
#define  icvLUT_Transform8u_16s_C1R   icvLUT_Transform8u_16u_C1R
#define  icvLUT_Transform8u_32f_C1R   icvLUT_Transform8u_32s_C1R

#define  icvLUT_Transform8u_8s_CnR    icvLUT_Transform8u_8u_CnR
#define  icvLUT_Transform8u_16s_CnR   icvLUT_Transform8u_16u_CnR
#define  icvLUT_Transform8u_32f_CnR   icvLUT_Transform8u_32s_CnR

CV_DEF_INIT_FUNC_TAB_2D( LUT_Transform8u, C1R )
CV_DEF_INIT_FUNC_TAB_2D( LUT_Transform8u, CnR )

typedef CvStatus (CV_STDCALL * CvLUT_TransformCnFunc)(
    const void* src, int srcstep, void* dst,
    int dststep, CvSize size, const void* lut, int cn );

CV_IMPL  void
cvLUT( const void* srcarr, void* dstarr, const void* lutarr )
{
    static CvFuncTable lut_c1_tab, lut_cn_tab;
    static CvLUT_TransformFunc lut_8u_tab[4];
    static int inittab = 0;

    CV_FUNCNAME( "cvLUT" );

    __BEGIN__;

    int  coi1 = 0, coi2 = 0;
    int  depth, cn, lut_cn;
    CvMat  srcstub, *src = (CvMat*)srcarr;
    CvMat  dststub, *dst = (CvMat*)dstarr;
    CvMat  lutstub, *lut = (CvMat*)lutarr;
    uchar* lut_data;
    uchar* shuffled_lut = 0;
    CvSize size;

    if( !inittab )
    {
        icvInitLUT_Transform8uC1RTable( &lut_c1_tab );
        icvInitLUT_Transform8uCnRTable( &lut_cn_tab );
        lut_8u_tab[0] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C1R;
        lut_8u_tab[1] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C2R;
        lut_8u_tab[2] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C3R;
        lut_8u_tab[3] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C4R;
        inittab = 1;
    }

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));

    if( !CV_IS_MAT(dst) )
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

    if( !CV_IS_MAT(lut) )
        CV_CALL( lut = cvGetMat( lut, &lutstub ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_DEPTH( src->type ) > CV_8S )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    depth = CV_MAT_DEPTH( dst->type );
    cn = CV_MAT_CN( dst->type );
    lut_cn = CV_MAT_CN( lut->type );

    if( !CV_IS_MAT_CONT(lut->type) || (lut_cn != 1 && lut_cn != cn) ||
        !CV_ARE_DEPTHS_EQ( dst, lut ) || lut->width*lut->height != 256 )
        CV_ERROR( CV_StsBadArg, "The LUT must be continuous array \n"
                                "with 256 elements of the same type as destination" );

    size = cvGetMatSize( src );
    if( lut_cn == 1 )
    {
        size.width *= cn;
        cn = 1;
    }

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        size.height = 1;
    }

    lut_data = lut->data.ptr;

    if( CV_MAT_DEPTH( src->type ) == CV_8S )
    {
        int half_size = CV_ELEM_SIZE1(depth)*cn*128;
        shuffled_lut = (uchar*)cvStackAlloc(half_size*2);

        // shuffle lut
        memcpy( shuffled_lut, lut_data + half_size, half_size );
        memcpy( shuffled_lut + half_size, lut_data, half_size );

        lut_data = shuffled_lut;
    }

    if( lut_cn == 1 || (lut_cn <= 4 && depth == CV_8U) )
    {
        CvLUT_TransformFunc func = depth == CV_8U ? lut_8u_tab[cn-1] :
            (CvLUT_TransformFunc)(lut_c1_tab.fn_2d[depth]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, dst->data.ptr,
                         dst->step, size, lut_data ));
    }
    else
    {
        CvLUT_TransformCnFunc func =
            (CvLUT_TransformCnFunc)(lut_cn_tab.fn_2d[depth]);
    
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, dst->data.ptr,
                         dst->step, size, lut_data, cn ));
    }

    __END__;
}

/* End of file. */
