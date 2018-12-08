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
*                                     MinMaxLoc                                          *
\****************************************************************************************/
                                                                    
#define CV_MINMAXLOC_ENTRY( _toggle_, srctype, temptype, cn ) \
    temptype min_val, max_val;                  \
    int min_loc = 0, max_loc = 0;               \
    int x, loc = 0, width = size.width*(cn);    \
    step /= sizeof(src[0]);                     \
                                                \
    min_val = src[0];                           \
    min_val = max_val = _toggle_( min_val )


#define CV_MINMAXLOC_EXIT( _fin_cast_macro_ )   \
    minLoc->x = min_loc;                        \
    maxLoc->x = max_loc;                        \
    minLoc->y = maxLoc->y = 0;                  \
    *minVal = _fin_cast_macro_(min_val);        \
    *maxVal = _fin_cast_macro_(max_val);        \
    return CV_OK


#define ICV_DEF_MINMAXLOC_1D_CASE_COI( _toggle_, temptype, cn ) \
    for( x = 0; x < width; x += (cn), loc++ )   \
    {                                           \
        temptype val = src[x];                  \
        val = _toggle_(val);                    \
                                                \
        if( val < min_val )                     \
        {                                       \
            min_val = val;                      \
            min_loc = loc;                      \
        }                                       \
        else if( val > max_val )                \
        {                                       \
            max_val = val;                      \
            max_loc = loc;                      \
        }                                       \
    }


#define ICV_DEF_MINMAXLOC_FUNC_2D( _toggle_, _fin_cast_macro_, flavor,      \
                                   srctype, temptype, extrtype )            \
IPCVAPI_IMPL( CvStatus,                                                     \
icvMinMaxIndx_##flavor##_C1R,( const srctype* src, int step, CvSize size,   \
    extrtype* minVal, extrtype* maxVal, CvPoint* minLoc, CvPoint* maxLoc ), \
    (src, step, size, minVal, maxVal, minLoc, maxLoc) )                     \
{                                                                           \
    CV_MINMAXLOC_ENTRY( _toggle_, srctype, temptype, 1 );                   \
                                                                            \
    for( ; size.height--; src += step )                                     \
    {                                                                       \
        ICV_DEF_MINMAXLOC_1D_CASE_COI( _toggle_, temptype, 1 );             \
    }                                                                       \
                                                                            \
    CV_MINMAXLOC_EXIT( _fin_cast_macro_ );                                  \
}


#define ICV_DEF_MINMAXLOC_FUNC_2D_COI( _toggle_, _fin_cast_macro_, flavor,  \
                                       srctype, temptype, extrtype )        \
static CvStatus CV_STDCALL                                                  \
icvMinMaxIndx_##flavor##_CnCR( const srctype* src, int step,                \
                          CvSize size, int cn, int coi,                     \
                          extrtype* minVal, extrtype* maxVal,               \
                          CvPoint* minLoc, CvPoint* maxLoc )                \
{                                                                           \
    (src) += coi - 1;                                                       \
    CV_MINMAXLOC_ENTRY( _toggle_, srctype, temptype, cn );                  \
                                                                            \
    for( ; size.height--; src += step )                                     \
    {                                                                       \
        ICV_DEF_MINMAXLOC_1D_CASE_COI( _toggle_, temptype, cn );            \
    }                                                                       \
                                                                            \
    CV_MINMAXLOC_EXIT( _fin_cast_macro_ );                                  \
}


#define ICV_DEF_MINMAXLOC_ALL_INT( flavor, srctype,             \
                                   _fin_cast_macro_, extrtype ) \
    ICV_DEF_MINMAXLOC_FUNC_2D( CV_NOP, _fin_cast_macro_, flavor,\
                               srctype, int, extrtype )         \
    ICV_DEF_MINMAXLOC_FUNC_2D_COI( CV_NOP, _fin_cast_macro_,    \
                            flavor, srctype, int, extrtype )

CV_INLINE float minmax_to_float( int val )
{
    Cv32suf v;
    v.i = CV_TOGGLE_FLT(val);
    return v.f;
}

CV_INLINE double minmax_to_double( int64 val )
{
    Cv64suf v;
    v.i = CV_TOGGLE_DBL(val);
    return v.f;
}

#define ICV_DEF_MINMAXLOC_ALL_FLT( flavor, srctype, _toggle_,           \
                                   _fin_cast_macro_, extrtype )         \
                                                                        \
    ICV_DEF_MINMAXLOC_FUNC_2D( _toggle_, _fin_cast_macro_, flavor,      \
                                srctype, srctype, extrtype )            \
    ICV_DEF_MINMAXLOC_FUNC_2D_COI( _toggle_, _fin_cast_macro_, flavor,  \
                                srctype, srctype, extrtype )

ICV_DEF_MINMAXLOC_ALL_INT( 8u, uchar, CV_CAST_32F, float )
ICV_DEF_MINMAXLOC_ALL_INT( 16u, ushort, CV_CAST_32F, float )
ICV_DEF_MINMAXLOC_ALL_INT( 16s, short, CV_CAST_32F, float )
ICV_DEF_MINMAXLOC_ALL_INT( 32s, int, CV_CAST_64F, double )
ICV_DEF_MINMAXLOC_ALL_FLT( 32f, int, CV_TOGGLE_FLT, minmax_to_float, float )
ICV_DEF_MINMAXLOC_ALL_FLT( 64f, int64, CV_TOGGLE_DBL, minmax_to_double, double )


/****************************************************************************************\
*                              MinMaxLoc with mask                                       *
\****************************************************************************************/

#define CV_MINMAXLOC_MASK_ENTRY( _toggle_, srctype, temptype, cn )  \
    temptype min_val = 0, max_val = 0;                              \
    int min_loc = -1, max_loc = -1;                                 \
    int x = 0, y, loc = 0, width = size.width;                      \
    step /= sizeof(src[0]);                                         \
                                                                    \
    if( width*(cn) == step && width == maskStep )                   \
    {                                                               \
        width *= size.height;                                       \
        size.height = 1;                                            \
    }                                                               \
                                                                    \
    for( y = 0; y < size.height; y++, src += step,                  \
                                      mask += maskStep )            \
    {                                                               \
        for( x = 0; x < width; x++, loc++ )                         \
            if( mask[x] != 0 )                                      \
            {                                                       \
                min_loc = max_loc = loc;                            \
                min_val = (src)[x*(cn)];                            \
                min_val = max_val = _toggle_( min_val );            \
                goto stop_scan;                                     \
            }                                                       \
    }                                                               \
                                                                    \
    stop_scan:;


#define ICV_DEF_MINMAXLOC_1D_MASK_CASE_COI( _toggle_, temptype, cn ) \
    for( ; x < width; x++, loc++ )      \
    {                                   \
        temptype val = src[x*(cn)];     \
        int m = mask[x] != 0;           \
        val = _toggle_(val);            \
                                        \
        if( val < min_val && m )        \
        {                               \
            min_val = val;              \
            min_loc = loc;              \
        }                               \
        else if( val > max_val && m )   \
        {                               \
            max_val = val;              \
            max_loc = loc;              \
        }                               \
    }


#define ICV_DEF_MINMAXLOC_MASK_FUNC_2D( _toggle_, _fin_cast_macro_, flavor, \
                                        srctype, temptype, extrtype )       \
IPCVAPI_IMPL( CvStatus,                                                     \
icvMinMaxIndx_##flavor##_C1MR,( const srctype* src, int step,               \
    const uchar* mask, int maskStep, CvSize size,                           \
    extrtype* minVal, extrtype* maxVal, CvPoint* minLoc, CvPoint* maxLoc ), \
    ( src, step, mask, maskStep, size, minVal, maxVal, minLoc, maxLoc) )    \
{                                                                           \
    CV_MINMAXLOC_MASK_ENTRY( _toggle_, srctype, temptype, 1 );              \
                                                                            \
    for( ; y < size.height; y++, src += step, mask += maskStep )            \
    {                                                                       \
        ICV_DEF_MINMAXLOC_1D_MASK_CASE_COI( _toggle_, temptype, 1 )         \
        x = 0;                                                              \
    }                                                                       \
                                                                            \
    CV_MINMAXLOC_EXIT( _fin_cast_macro_ );                                  \
}


#define ICV_DEF_MINMAXLOC_MASK_FUNC_2D_COI( _toggle_, _fin_cast_macro_,     \
                                    flavor, srctype, temptype, extrtype )   \
static CvStatus CV_STDCALL                                                  \
icvMinMaxIndx_##flavor##_CnCMR( const srctype* src, int step,               \
    const uchar* mask, int maskStep, CvSize size, int cn, int coi,          \
    extrtype* minVal, extrtype* maxVal, CvPoint* minLoc, CvPoint* maxLoc )  \
{                                                                           \
    (src) += coi - 1;                                                       \
    CV_MINMAXLOC_MASK_ENTRY( _toggle_, srctype, temptype, cn );             \
                                                                            \
    for( ; y < size.height; y++, src += step, mask += maskStep )            \
    {                                                                       \
        ICV_DEF_MINMAXLOC_1D_MASK_CASE_COI( _toggle_, temptype, cn )        \
        x = 0;                                                              \
    }                                                                       \
                                                                            \
    CV_MINMAXLOC_EXIT( _fin_cast_macro_ );                                  \
}



#define ICV_DEF_MINMAXLOC_MASK_ALL_INT( flavor, srctype,                    \
                                        _fin_cast_macro_, extrtype )        \
    ICV_DEF_MINMAXLOC_MASK_FUNC_2D( CV_NOP, _fin_cast_macro_, flavor,       \
                                    srctype, int, extrtype )                \
    ICV_DEF_MINMAXLOC_MASK_FUNC_2D_COI( CV_NOP, _fin_cast_macro_, flavor,   \
                                    srctype, int, extrtype )

#define ICV_DEF_MINMAXLOC_MASK_ALL_FLT( flavor, srctype, _toggle_,          \
                                        _fin_cast_macro_, extrtype )        \
    ICV_DEF_MINMAXLOC_MASK_FUNC_2D( _toggle_, _fin_cast_macro_, flavor,     \
                                    srctype, srctype, extrtype )            \
    ICV_DEF_MINMAXLOC_MASK_FUNC_2D_COI( _toggle_, _fin_cast_macro_, flavor, \
                                    srctype, srctype, extrtype )

ICV_DEF_MINMAXLOC_MASK_ALL_INT( 8u, uchar, CV_CAST_32F, float )
ICV_DEF_MINMAXLOC_MASK_ALL_INT( 16u, ushort, CV_CAST_32F, float )
ICV_DEF_MINMAXLOC_MASK_ALL_INT( 16s, short, CV_CAST_32F, float )
ICV_DEF_MINMAXLOC_MASK_ALL_INT( 32s, int, CV_CAST_64F, double )
ICV_DEF_MINMAXLOC_MASK_ALL_FLT( 32f, int, CV_TOGGLE_FLT, minmax_to_float, float )
ICV_DEF_MINMAXLOC_MASK_ALL_FLT( 64f, int64, CV_TOGGLE_DBL, minmax_to_double, double )

#define icvMinMaxIndx_8s_C1R    0
#define icvMinMaxIndx_8s_CnCR   0
#define icvMinMaxIndx_8s_C1MR   0
#define icvMinMaxIndx_8s_CnCMR  0

CV_DEF_INIT_FUNC_TAB_2D( MinMaxIndx, C1R )
CV_DEF_INIT_FUNC_TAB_2D( MinMaxIndx, CnCR )
CV_DEF_INIT_FUNC_TAB_2D( MinMaxIndx, C1MR )
CV_DEF_INIT_FUNC_TAB_2D( MinMaxIndx, CnCMR )


CV_IMPL  void
cvMinMaxLoc( const void* img, double* _minVal, double* _maxVal,
             CvPoint* _minLoc, CvPoint* _maxLoc, const void* mask )
{
    static CvFuncTable minmax_tab, minmaxcoi_tab;
    static CvFuncTable minmaxmask_tab, minmaxmaskcoi_tab;
    static int inittab = 0;

    CV_FUNCNAME("cvMinMaxLoc");

    __BEGIN__;

    int type, depth, cn, coi = 0;
    int mat_step, mask_step = 0, cont_flag;
    CvSize size;
    CvMat stub, maskstub, *mat = (CvMat*)img, *matmask = (CvMat*)mask;
    CvPoint minloc, maxloc;
    double minv = 0, maxv = 0;
    float minvf = 0.f, maxvf = 0.f;
    void *pmin = &minvf, *pmax = &maxvf;

    if( !inittab )
    {
        icvInitMinMaxIndxC1RTable( &minmax_tab );
        icvInitMinMaxIndxCnCRTable( &minmaxcoi_tab );
        icvInitMinMaxIndxC1MRTable( &minmaxmask_tab );
        icvInitMinMaxIndxCnCMRTable( &minmaxmaskcoi_tab );
        inittab = 1;
    }
    
    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub, &coi ));

    type = CV_MAT_TYPE( mat->type );
    depth = CV_MAT_DEPTH( type );
    cn = CV_MAT_CN( type );
    size = cvGetMatSize( mat );

    if( cn > 1 && coi == 0 )
        CV_ERROR( CV_StsBadArg, "" );

    if( depth == CV_32S || depth == CV_64F )
        pmin = &minv, pmax = &maxv;
    
    mat_step = mat->step;
    cont_flag = mat->type;

    if( mask )
    {
        CV_CALL( matmask = cvGetMat( matmask, &maskstub ));

        if( !CV_IS_MASK_ARR( matmask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, matmask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = matmask->step;
        cont_flag &= matmask->type;
    }

    if( CV_IS_MAT_CONT(cont_flag) )
    {
        size.width *= size.height;
        size.height = 1;
    }

    if( size.height == 1 )
        mat_step = mask_step = CV_STUB_STEP;

    if( !mask )
    {
        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            CvFunc2D_1A4P func = (CvFunc2D_1A4P)(minmax_tab.fn_2d[depth]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size,
                             pmin, pmax, &minloc, &maxloc ));
        }
        else
        {
            CvFunc2DnC_1A4P func = (CvFunc2DnC_1A4P)(minmaxcoi_tab.fn_2d[depth]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size, cn, coi,
                             pmin, pmax, &minloc, &maxloc ));
        }
    }
    else
    {
        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            CvFunc2D_2A4P func = (CvFunc2D_2A4P)(minmaxmask_tab.fn_2d[depth]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, matmask->data.ptr,
                             mask_step, size,
                             pmin, pmax, &minloc, &maxloc ));
        }
        else
        {
            CvFunc2DnC_2A4P func = (CvFunc2DnC_2A4P)(minmaxmaskcoi_tab.fn_2d[depth]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step,
                             matmask->data.ptr, mask_step, size, cn, coi,
                             pmin, pmax, &minloc, &maxloc ));
        }
    }

    if( matmask || _minLoc || _maxLoc )
    {
        if( minloc.x >= mat->cols )
        {
            minloc.y = minloc.x / mat->cols;
            minloc.x -= minloc.y * mat->cols;
        }

        if( maxloc.x >= mat->cols )
        {
            maxloc.y = maxloc.x / mat->cols;
            maxloc.x -= maxloc.y * mat->cols;
        }

        if( matmask && ((unsigned)minloc.x >= (unsigned)mat->cols ||
            (unsigned)minloc.y >= (unsigned)mat->rows ||
            matmask->data.ptr[minloc.y*matmask->step + minloc.x] == 0 ||
            (unsigned)maxloc.x >= (unsigned)mat->cols ||
            (unsigned)maxloc.y >= (unsigned)mat->rows ||
            matmask->data.ptr[maxloc.y*matmask->step + maxloc.x] == 0) )
        {
            minloc.x = minloc.y = maxloc.x = maxloc.y = -1;
            minv = maxv = minvf = maxvf = 0;
        }

        if( _minLoc )
            *_minLoc = minloc;

        if( _maxLoc )
            *_maxLoc = maxloc;
    }

    if( depth != CV_32S && depth != CV_64F )
    {
        minv = minvf;
        maxv = maxvf;
    }

    if( _minVal )
        *_minVal = minv;

    if( _maxVal )
        *_maxVal = maxv;

    __END__;
}

/*  End of file  */
