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
#include <limits.h>
#include <stdio.h>

#define IPCV_MORPHOLOGY_PTRS( morphtype, flavor )               \
    icv##morphtype##Rect_##flavor##_C1R_t                       \
        icv##morphtype##Rect_##flavor##_C1R_p = 0;              \
    icv##morphtype##Rect_GetBufSize_##flavor##_C1R_t            \
        icv##morphtype##Rect_GetBufSize_##flavor##_C1R_p = 0;   \
    icv##morphtype##Rect_##flavor##_C3R_t                       \
        icv##morphtype##Rect_##flavor##_C3R_p = 0;              \
    icv##morphtype##Rect_GetBufSize_##flavor##_C3R_t            \
        icv##morphtype##Rect_GetBufSize_##flavor##_C3R_p = 0;   \
    icv##morphtype##Rect_##flavor##_C4R_t                       \
        icv##morphtype##Rect_##flavor##_C4R_p = 0;              \
    icv##morphtype##Rect_GetBufSize_##flavor##_C4R_t            \
        icv##morphtype##Rect_GetBufSize_##flavor##_C4R_p = 0;   \
                                                                \
    icv##morphtype##_##flavor##_C1R_t                           \
        icv##morphtype##_##flavor##_C1R_p = 0;                  \
    icv##morphtype##_##flavor##_C3R_t                           \
        icv##morphtype##_##flavor##_C3R_p = 0;                  \
    icv##morphtype##_##flavor##_C4R_t                           \
        icv##morphtype##_##flavor##_C4R_p = 0;

#define IPCV_MORPHOLOGY_INITALLOC_PTRS( flavor )                \
    icvMorphInitAlloc_##flavor##_C1R_t                          \
        icvMorphInitAlloc_##flavor##_C1R_p = 0;                 \
    icvMorphInitAlloc_##flavor##_C3R_t                          \
        icvMorphInitAlloc_##flavor##_C3R_p = 0;                 \
    icvMorphInitAlloc_##flavor##_C4R_t                          \
        icvMorphInitAlloc_##flavor##_C4R_p = 0;

IPCV_MORPHOLOGY_PTRS( Erode, 8u )
IPCV_MORPHOLOGY_PTRS( Erode, 16u )
IPCV_MORPHOLOGY_PTRS( Erode, 32f )
IPCV_MORPHOLOGY_PTRS( Dilate, 8u )
IPCV_MORPHOLOGY_PTRS( Dilate, 16u )
IPCV_MORPHOLOGY_PTRS( Dilate, 32f )
IPCV_MORPHOLOGY_INITALLOC_PTRS( 8u )
IPCV_MORPHOLOGY_INITALLOC_PTRS( 16u )
IPCV_MORPHOLOGY_INITALLOC_PTRS( 32f )

icvMorphFree_t icvMorphFree_p = 0;

/****************************************************************************************\
                     Basic Morphological Operations: Erosion & Dilation
\****************************************************************************************/

static void icvErodeRectRow_8u( const uchar* src, uchar* dst, void* params );
static void icvErodeRectRow_16u( const ushort* src, ushort* dst, void* params );
static void icvErodeRectRow_32f( const int* src, int* dst, void* params );
static void icvDilateRectRow_8u( const uchar* src, uchar* dst, void* params );
static void icvDilateRectRow_16u( const ushort* src, ushort* dst, void* params );
static void icvDilateRectRow_32f( const int* src, int* dst, void* params );

static void icvErodeRectCol_8u( const uchar** src, uchar* dst, int dst_step,
                                int count, void* params );
static void icvErodeRectCol_16u( const ushort** src, ushort* dst, int dst_step,
                                 int count, void* params );
static void icvErodeRectCol_32f( const int** src, int* dst, int dst_step,
                                 int count, void* params );
static void icvDilateRectCol_8u( const uchar** src, uchar* dst, int dst_step,
                                 int count, void* params );
static void icvDilateRectCol_16u( const ushort** src, ushort* dst, int dst_step,
                                  int count, void* params );
static void icvDilateRectCol_32f( const int** src, int* dst, int dst_step,
                                  int count, void* params );

static void icvErodeAny_8u( const uchar** src, uchar* dst, int dst_step,
                            int count, void* params );
static void icvErodeAny_16u( const ushort** src, ushort* dst, int dst_step,
                             int count, void* params );
static void icvErodeAny_32f( const int** src, int* dst, int dst_step,
                             int count, void* params );
static void icvDilateAny_8u( const uchar** src, uchar* dst, int dst_step,
                             int count, void* params );
static void icvDilateAny_16u( const ushort** src, ushort* dst, int dst_step,
                              int count, void* params );
static void icvDilateAny_32f( const int** src, int* dst, int dst_step,
                              int count, void* params );

CvMorphology::CvMorphology()
{
    element = 0;
    el_sparse = 0;
}

CvMorphology::CvMorphology( int _operation, int _max_width, int _src_dst_type,
                            int _element_shape, CvMat* _element,
                            CvSize _ksize, CvPoint _anchor,
                            int _border_mode, CvScalar _border_value )
{
    element = 0;
    el_sparse = 0;
    init( _operation, _max_width, _src_dst_type,
          _element_shape, _element, _ksize, _anchor,
          _border_mode, _border_value );
}


void CvMorphology::clear()
{
    cvReleaseMat( &element );
    cvFree( &el_sparse );
    CvBaseImageFilter::clear();
}


CvMorphology::~CvMorphology()
{
    clear();
}


void CvMorphology::init( int _operation, int _max_width, int _src_dst_type,
                         int _element_shape, CvMat* _element,
                         CvSize _ksize, CvPoint _anchor,
                         int _border_mode, CvScalar _border_value )
{
    CV_FUNCNAME( "CvMorphology::init" );

    __BEGIN__;

    int depth = CV_MAT_DEPTH(_src_dst_type);
    int el_type = 0, nz = -1;
    
    if( _operation != ERODE && _operation != DILATE )
        CV_ERROR( CV_StsBadArg, "Unknown/unsupported morphological operation" );

    if( _element_shape == CUSTOM )
    {
        if( !CV_IS_MAT(_element) )
            CV_ERROR( CV_StsBadArg,
            "structuring element should be valid matrix if CUSTOM element shape is specified" );

        el_type = CV_MAT_TYPE(_element->type);
        if( el_type != CV_8UC1 && el_type != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat, "the structuring element must have 8uC1 or 32sC1 type" );

        _ksize = cvGetMatSize(_element);
        CV_CALL( nz = cvCountNonZero(_element));
        if( nz == _ksize.width*_ksize.height )
            _element_shape = RECT;
    }

    operation = _operation;
    el_shape = _element_shape;

    CV_CALL( CvBaseImageFilter::init( _max_width, _src_dst_type, _src_dst_type,
        _element_shape == RECT, _ksize, _anchor, _border_mode, _border_value ));

    if( el_shape == RECT )
    {
        if( operation == ERODE )
        {
            if( depth == CV_8U )
                x_func = (CvRowFilterFunc)icvErodeRectRow_8u,
                y_func = (CvColumnFilterFunc)icvErodeRectCol_8u;
            else if( depth == CV_16U )
                x_func = (CvRowFilterFunc)icvErodeRectRow_16u,
                y_func = (CvColumnFilterFunc)icvErodeRectCol_16u;
            else if( depth == CV_32F )
                x_func = (CvRowFilterFunc)icvErodeRectRow_32f,
                y_func = (CvColumnFilterFunc)icvErodeRectCol_32f;
        }
        else
        {
            assert( operation == DILATE );
            if( depth == CV_8U )
                x_func = (CvRowFilterFunc)icvDilateRectRow_8u,
                y_func = (CvColumnFilterFunc)icvDilateRectCol_8u;
            else if( depth == CV_16U )
                x_func = (CvRowFilterFunc)icvDilateRectRow_16u,
                y_func = (CvColumnFilterFunc)icvDilateRectCol_16u;
            else if( depth == CV_32F )
                x_func = (CvRowFilterFunc)icvDilateRectRow_32f,
                y_func = (CvColumnFilterFunc)icvDilateRectCol_32f;
        }
    }
    else
    {
        int i, j, k = 0;
        int cn = CV_MAT_CN(src_type);
        CvPoint* nz_loc;

        if( !(element && el_sparse &&
            _ksize.width == element->cols && _ksize.height == element->rows) )
        {
            cvReleaseMat( &element );
            cvFree( &el_sparse );
            CV_CALL( element = cvCreateMat( _ksize.height, _ksize.width, CV_8UC1 ));
            CV_CALL( el_sparse = (uchar*)cvAlloc(
                ksize.width*ksize.height*(2*sizeof(int) + sizeof(uchar*))));
        }

        if( el_shape == CUSTOM )
        {
            CV_CALL( cvConvert( _element, element ));
        }
        else
        {
            CV_CALL( init_binary_element( element, el_shape, anchor ));
        }

        if( operation == ERODE )
        {
            if( depth == CV_8U )
                y_func = (CvColumnFilterFunc)icvErodeAny_8u;
            else if( depth == CV_16U )
                y_func = (CvColumnFilterFunc)icvErodeAny_16u;
            else if( depth == CV_32F )
                y_func = (CvColumnFilterFunc)icvErodeAny_32f;
        }
        else
        {
            assert( operation == DILATE );
            if( depth == CV_8U )
                y_func = (CvColumnFilterFunc)icvDilateAny_8u;
            else if( depth == CV_16U )
                y_func = (CvColumnFilterFunc)icvDilateAny_16u;
            else if( depth == CV_32F )
                y_func = (CvColumnFilterFunc)icvDilateAny_32f;
        }
        
        nz_loc = (CvPoint*)el_sparse;

        for( i = 0; i < ksize.height; i++ )
            for( j = 0; j < ksize.width; j++ )
            {
                if( element->data.ptr[i*element->step+j] )
                    nz_loc[k++] = cvPoint(j*cn,i);
            }
        if( k == 0 )
            nz_loc[k++] = cvPoint(anchor.x*cn,anchor.y);
        el_sparse_count = k;
    }

    if( depth == CV_32F && border_mode == IPL_BORDER_CONSTANT )
    {
        int i, cn = CV_MAT_CN(src_type);
        int* bt = (int*)border_tab;
        for( i = 0; i < cn; i++ )
            bt[i] = CV_TOGGLE_FLT(bt[i]);
    }

    __END__;
}


void CvMorphology::init( int _max_width, int _src_type, int _dst_type,
                         bool _is_separable, CvSize _ksize,
                         CvPoint _anchor, int _border_mode,
                         CvScalar _border_value )
{
    CvBaseImageFilter::init( _max_width, _src_type, _dst_type, _is_separable,
                             _ksize, _anchor, _border_mode, _border_value );
}


void CvMorphology::start_process( CvSlice x_range, int width )
{
    CvBaseImageFilter::start_process( x_range, width );
    if( el_shape == RECT )
    {
        // cut the cyclic buffer off by 1 line if need, to make
        // the vertical part of separable morphological filter
        // always process 2 rows at once (except, may be,
        // for the last one in a stripe).
        int t = buf_max_count - max_ky*2;
        if( t > 1 && t % 2 != 0 )
        {
            buf_max_count--;
            buf_end -= buf_step;
        }
    }
}


int CvMorphology::fill_cyclic_buffer( const uchar* src, int src_step,
                                      int y0, int y1, int y2 )
{
    int i, y = y0, bsz1 = border_tab_sz1, bsz = border_tab_sz;
    int pix_size = CV_ELEM_SIZE(src_type);
    int width_n = (prev_x_range.end_index - prev_x_range.start_index)*pix_size;

    if( CV_MAT_DEPTH(src_type) != CV_32F )
        return CvBaseImageFilter::fill_cyclic_buffer( src, src_step, y0, y1, y2 );

    // fill the cyclic buffer
    for( ; buf_count < buf_max_count && y < y2; buf_count++, y++, src += src_step )
    {
        uchar* trow = is_separable ? buf_end : buf_tail;

        for( i = 0; i < width_n; i += sizeof(int) )
        {
            int t = *(int*)(src + i);
            *(int*)(trow + i + bsz1) = CV_TOGGLE_FLT(t);
        }

        if( border_mode != IPL_BORDER_CONSTANT )
        {
            for( i = 0; i < bsz1; i++ )
            {
                int j = border_tab[i];
                trow[i] = trow[j];
            }
            for( ; i < bsz; i++ )
            {
                int j = border_tab[i];
                trow[i + width_n] = trow[j];
            }
        }
        else
        {
            const uchar *bt = (uchar*)border_tab; 
            for( i = 0; i < bsz1; i++ )
                trow[i] = bt[i];

            for( ; i < bsz; i++ )
                trow[i + width_n] = bt[i];
        }

        if( is_separable )
            x_func( trow, buf_tail, this );

        buf_tail += buf_step;
        if( buf_tail >= buf_end )
            buf_tail = buf_start;
    }

    return y - y0;
}


void CvMorphology::init_binary_element( CvMat* element, int element_shape, CvPoint anchor )
{
    CV_FUNCNAME( "CvMorphology::init_binary_element" );

    __BEGIN__;

    int type;
    int i, j, cols, rows;
    int r = 0, c = 0;
    double inv_r2 = 0;

    if( !CV_IS_MAT(element) )
        CV_ERROR( CV_StsBadArg, "element must be valid matrix" );

    type = CV_MAT_TYPE(element->type);
    if( type != CV_8UC1 && type != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "element must have 8uC1 or 32sC1 type" );

    if( anchor.x == -1 )
        anchor.x = element->cols/2;

    if( anchor.y == -1 )
        anchor.y = element->rows/2;

    if( (unsigned)anchor.x >= (unsigned)element->cols ||
        (unsigned)anchor.y >= (unsigned)element->rows )
        CV_ERROR( CV_StsOutOfRange, "anchor is outside of element" );

    if( element_shape != RECT && element_shape != CROSS && element_shape != ELLIPSE )
        CV_ERROR( CV_StsBadArg, "Unknown/unsupported element shape" );

    rows = element->rows;
    cols = element->cols;

    if( rows == 1 || cols == 1 )
        element_shape = RECT;

    if( element_shape == ELLIPSE )
    {
        r = rows/2;
        c = cols/2;
        inv_r2 = r ? 1./((double)r*r) : 0;
    }

    for( i = 0; i < rows; i++ )
    {
        uchar* ptr = element->data.ptr + i*element->step;
        int j1 = 0, j2 = 0, jx, t = 0;

        if( element_shape == RECT || (element_shape == CROSS && i == anchor.y) )
            j2 = cols;
        else if( element_shape == CROSS )
            j1 = anchor.x, j2 = j1 + 1;
        else
        {
            int dy = i - r;
            if( abs(dy) <= r )
            {
                int dx = cvRound(c*sqrt(((double)r*r - dy*dy)*inv_r2));
                j1 = MAX( c - dx, 0 );
                j2 = MIN( c + dx + 1, cols );
            }
        }

        for( j = 0, jx = j1; j < cols; )
        {
            for( ; j < jx; j++ )
            {
                if( type == CV_8UC1 )
                    ptr[j] = (uchar)t;
                else
                    ((int*)ptr)[j] = t;
            }
            if( jx == j2 )
                jx = cols, t = 0;
            else
                jx = j2, t = 1;
        }
    }

    __END__;
}


#define ICV_MORPH_RECT_ROW( name, flavor, arrtype,          \
                            worktype, update_extr_macro )   \
static void                                                 \
icv##name##RectRow_##flavor( const arrtype* src,            \
                             arrtype* dst, void* params )   \
{                                                           \
    const CvMorphology* state = (const CvMorphology*)params;\
    int ksize = state->get_kernel_size().width;             \
    int width = state->get_width();                         \
    int cn = CV_MAT_CN(state->get_src_type());              \
    int i, j, k;                                            \
                                                            \
    width *= cn; ksize *= cn;                               \
                                                            \
    if( ksize == cn )                                       \
    {                                                       \
        for( i = 0; i < width; i++ )                        \
            dst[i] = src[i];                                \
        return;                                             \
    }                                                       \
                                                            \
    for( k = 0; k < cn; k++, src++, dst++ )                 \
    {                                                       \
        for( i = 0; i <= width - cn*2; i += cn*2 )          \
        {                                                   \
            const arrtype* s = src + i;                     \
            worktype m = s[cn], t;                          \
            for( j = cn*2; j < ksize; j += cn )             \
            {                                               \
                t = s[j]; update_extr_macro(m,t);           \
            }                                               \
            t = s[0]; update_extr_macro(t,m);               \
            dst[i] = (arrtype)t;                            \
            t = s[j]; update_extr_macro(t,m);               \
            dst[i+cn] = (arrtype)t;                         \
        }                                                   \
                                                            \
        for( ; i < width; i += cn )                         \
        {                                                   \
            const arrtype* s = src + i;                     \
            worktype m = s[0], t;                           \
            for( j = cn; j < ksize; j += cn )               \
            {                                               \
                t = s[j]; update_extr_macro(m,t);           \
            }                                               \
            dst[i] = (arrtype)m;                            \
        }                                                   \
    }                                                       \
}


ICV_MORPH_RECT_ROW( Erode, 8u, uchar, int, CV_CALC_MIN_8U )
ICV_MORPH_RECT_ROW( Dilate, 8u, uchar, int, CV_CALC_MAX_8U )
ICV_MORPH_RECT_ROW( Erode, 16u, ushort, int, CV_CALC_MIN )
ICV_MORPH_RECT_ROW( Dilate, 16u, ushort, int, CV_CALC_MAX )
ICV_MORPH_RECT_ROW( Erode, 32f, int, int, CV_CALC_MIN )
ICV_MORPH_RECT_ROW( Dilate, 32f, int, int, CV_CALC_MAX )


#define ICV_MORPH_RECT_COL( name, flavor, arrtype,          \
        worktype, update_extr_macro, toggle_macro )         \
static void                                                 \
icv##name##RectCol_##flavor( const arrtype** src,           \
    arrtype* dst, int dst_step, int count, void* params )   \
{                                                           \
    const CvMorphology* state = (const CvMorphology*)params;\
    int ksize = state->get_kernel_size().height;            \
    int width = state->get_width();                         \
    int cn = CV_MAT_CN(state->get_src_type());              \
    int i, k;                                               \
                                                            \
    width *= cn;                                            \
    dst_step /= sizeof(dst[0]);                             \
                                                            \
    for( ; ksize > 1 && count > 1; count -= 2,              \
                        dst += dst_step*2, src += 2 )       \
    {                                                       \
        for( i = 0; i <= width - 4; i += 4 )                \
        {                                                   \
            const arrtype* sptr = src[1] + i;               \
            worktype s0 = sptr[0], s1 = sptr[1],            \
                s2 = sptr[2], s3 = sptr[3], t0, t1;         \
                                                            \
            for( k = 2; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i;                          \
                t0 = sptr[0]; t1 = sptr[1];                 \
                update_extr_macro(s0,t0);                   \
                update_extr_macro(s1,t1);                   \
                t0 = sptr[2]; t1 = sptr[3];                 \
                update_extr_macro(s2,t0);                   \
                update_extr_macro(s3,t1);                   \
            }                                               \
                                                            \
            sptr = src[0] + i;                              \
            t0 = sptr[0]; t1 = sptr[1];                     \
            update_extr_macro(t0,s0);                       \
            update_extr_macro(t1,s1);                       \
            dst[i] = (arrtype)toggle_macro(t0);             \
            dst[i+1] = (arrtype)toggle_macro(t1);           \
            t0 = sptr[2]; t1 = sptr[3];                     \
            update_extr_macro(t0,s2);                       \
            update_extr_macro(t1,s3);                       \
            dst[i+2] = (arrtype)toggle_macro(t0);           \
            dst[i+3] = (arrtype)toggle_macro(t1);           \
                                                            \
            sptr = src[k] + i;                              \
            t0 = sptr[0]; t1 = sptr[1];                     \
            update_extr_macro(t0,s0);                       \
            update_extr_macro(t1,s1);                       \
            dst[i+dst_step] = (arrtype)toggle_macro(t0);    \
            dst[i+dst_step+1] = (arrtype)toggle_macro(t1);  \
            t0 = sptr[2]; t1 = sptr[3];                     \
            update_extr_macro(t0,s2);                       \
            update_extr_macro(t1,s3);                       \
            dst[i+dst_step+2] = (arrtype)toggle_macro(t0);  \
            dst[i+dst_step+3] = (arrtype)toggle_macro(t1);  \
        }                                                   \
                                                            \
        for( ; i < width; i++ )                             \
        {                                                   \
            const arrtype* sptr = src[1] + i;               \
            worktype s0 = sptr[0], t0;                      \
                                                            \
            for( k = 2; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i; t0 = sptr[0];            \
                update_extr_macro(s0,t0);                   \
            }                                               \
                                                            \
            sptr = src[0] + i; t0 = sptr[0];                \
            update_extr_macro(t0,s0);                       \
            dst[i] = (arrtype)toggle_macro(t0);             \
                                                            \
            sptr = src[k] + i; t0 = sptr[0];                \
            update_extr_macro(t0,s0);                       \
            dst[i+dst_step] = (arrtype)toggle_macro(t0);    \
        }                                                   \
    }                                                       \
                                                            \
    for( ; count > 0; count--, dst += dst_step, src++ )     \
    {                                                       \
        for( i = 0; i <= width - 4; i += 4 )                \
        {                                                   \
            const arrtype* sptr = src[0] + i;               \
            worktype s0 = sptr[0], s1 = sptr[1],            \
                s2 = sptr[2], s3 = sptr[3], t0, t1;         \
                                                            \
            for( k = 1; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i;                          \
                t0 = sptr[0]; t1 = sptr[1];                 \
                update_extr_macro(s0,t0);                   \
                update_extr_macro(s1,t1);                   \
                t0 = sptr[2]; t1 = sptr[3];                 \
                update_extr_macro(s2,t0);                   \
                update_extr_macro(s3,t1);                   \
            }                                               \
            dst[i] = (arrtype)toggle_macro(s0);             \
            dst[i+1] = (arrtype)toggle_macro(s1);           \
            dst[i+2] = (arrtype)toggle_macro(s2);           \
            dst[i+3] = (arrtype)toggle_macro(s3);           \
        }                                                   \
                                                            \
        for( ; i < width; i++ )                             \
        {                                                   \
            const arrtype* sptr = src[0] + i;               \
            worktype s0 = sptr[0], t0;                      \
                                                            \
            for( k = 1; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i; t0 = sptr[0];            \
                update_extr_macro(s0,t0);                   \
            }                                               \
            dst[i] = (arrtype)toggle_macro(s0);             \
        }                                                   \
    }                                                       \
}


ICV_MORPH_RECT_COL( Erode, 8u, uchar, int, CV_CALC_MIN_8U, CV_NOP )
ICV_MORPH_RECT_COL( Dilate, 8u, uchar, int, CV_CALC_MAX_8U, CV_NOP )
ICV_MORPH_RECT_COL( Erode, 16u, ushort, int, CV_CALC_MIN, CV_NOP )
ICV_MORPH_RECT_COL( Dilate, 16u, ushort, int, CV_CALC_MAX, CV_NOP )
ICV_MORPH_RECT_COL( Erode, 32f, int, int, CV_CALC_MIN, CV_TOGGLE_FLT )
ICV_MORPH_RECT_COL( Dilate, 32f, int, int, CV_CALC_MAX, CV_TOGGLE_FLT )


#define ICV_MORPH_ANY( name, flavor, arrtype, worktype,     \
                       update_extr_macro, toggle_macro )    \
static void                                                 \
icv##name##Any_##flavor( const arrtype** src, arrtype* dst, \
                    int dst_step, int count, void* params ) \
{                                                           \
    CvMorphology* state = (CvMorphology*)params;            \
    int width = state->get_width();                         \
    int cn = CV_MAT_CN(state->get_src_type());              \
    int i, k;                                               \
    CvPoint* el_sparse = (CvPoint*)state->get_element_sparse_buf();\
    int el_count = state->get_element_sparse_count();       \
    const arrtype** el_ptr = (const arrtype**)(el_sparse + el_count);\
    const arrtype** el_end = el_ptr + el_count;             \
                                                            \
    width *= cn;                                            \
    dst_step /= sizeof(dst[0]);                             \
                                                            \
    for( ; count > 0; count--, dst += dst_step, src++ )     \
    {                                                       \
        for( k = 0; k < el_count; k++ )                     \
            el_ptr[k] = src[el_sparse[k].y]+el_sparse[k].x; \
                                                            \
        for( i = 0; i <= width - 4; i += 4 )                \
        {                                                   \
            const arrtype** psptr = el_ptr;                 \
            const arrtype* sptr = *psptr++;                 \
            worktype s0 = sptr[i], s1 = sptr[i+1],          \
                     s2 = sptr[i+2], s3 = sptr[i+3], t;     \
                                                            \
            while( psptr != el_end )                        \
            {                                               \
                sptr = *psptr++;                            \
                t = sptr[i];                                \
                update_extr_macro(s0,t);                    \
                t = sptr[i+1];                              \
                update_extr_macro(s1,t);                    \
                t = sptr[i+2];                              \
                update_extr_macro(s2,t);                    \
                t = sptr[i+3];                              \
                update_extr_macro(s3,t);                    \
            }                                               \
                                                            \
            dst[i] = (arrtype)toggle_macro(s0);             \
            dst[i+1] = (arrtype)toggle_macro(s1);           \
            dst[i+2] = (arrtype)toggle_macro(s2);           \
            dst[i+3] = (arrtype)toggle_macro(s3);           \
        }                                                   \
                                                            \
        for( ; i < width; i++ )                             \
        {                                                   \
            const arrtype* sptr = el_ptr[0] + i;            \
            worktype s0 = sptr[0], t0;                      \
                                                            \
            for( k = 1; k < el_count; k++ )                 \
            {                                               \
                sptr = el_ptr[k] + i;                       \
                t0 = sptr[0];                               \
                update_extr_macro(s0,t0);                   \
            }                                               \
                                                            \
            dst[i] = (arrtype)toggle_macro(s0);             \
        }                                                   \
    }                                                       \
}

ICV_MORPH_ANY( Erode, 8u, uchar, int, CV_CALC_MIN, CV_NOP )
ICV_MORPH_ANY( Dilate, 8u, uchar, int, CV_CALC_MAX, CV_NOP )
ICV_MORPH_ANY( Erode, 16u, ushort, int, CV_CALC_MIN, CV_NOP )
ICV_MORPH_ANY( Dilate, 16u, ushort, int, CV_CALC_MAX, CV_NOP )
ICV_MORPH_ANY( Erode, 32f, int, int, CV_CALC_MIN, CV_TOGGLE_FLT )
ICV_MORPH_ANY( Dilate, 32f, int, int, CV_CALC_MAX, CV_TOGGLE_FLT )

/////////////////////////////////// External Interface /////////////////////////////////////


CV_IMPL IplConvKernel *
cvCreateStructuringElementEx( int cols, int rows,
                              int anchorX, int anchorY,
                              int shape, int *values )
{
    IplConvKernel *element = 0;
    int i, size = rows * cols;
    int element_size = sizeof(*element) + size*sizeof(element->values[0]);

    CV_FUNCNAME( "cvCreateStructuringElementEx" );

    __BEGIN__;

    if( !values && shape == CV_SHAPE_CUSTOM )
        CV_ERROR_FROM_STATUS( CV_NULLPTR_ERR );

    if( cols <= 0 || rows <= 0 ||
        (unsigned) anchorX >= (unsigned) cols ||
        (unsigned) anchorY >= (unsigned) rows )
        CV_ERROR_FROM_STATUS( CV_BADSIZE_ERR );

    CV_CALL( element = (IplConvKernel *)cvAlloc(element_size + 32));
    if( !element )
        CV_ERROR_FROM_STATUS( CV_OUTOFMEM_ERR );

    element->nCols = cols;
    element->nRows = rows;
    element->anchorX = anchorX;
    element->anchorY = anchorY;
    element->nShiftR = shape < CV_SHAPE_ELLIPSE ? shape : CV_SHAPE_CUSTOM;
    element->values = (int*)(element + 1);

    if( shape == CV_SHAPE_CUSTOM )
    {
        if( !values )
            CV_ERROR( CV_StsNullPtr, "Null pointer to the custom element mask" );
        for( i = 0; i < size; i++ )
            element->values[i] = values[i];
    }
    else
    {
        CvMat el_hdr = cvMat( rows, cols, CV_32SC1, element->values );
        CV_CALL( CvMorphology::init_binary_element(&el_hdr,
                        shape, cvPoint(anchorX,anchorY)));
    }

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseStructuringElement( &element );

    return element;
}


CV_IMPL void
cvReleaseStructuringElement( IplConvKernel ** element )
{
    CV_FUNCNAME( "cvReleaseStructuringElement" );

    __BEGIN__;

    if( !element )
        CV_ERROR( CV_StsNullPtr, "" );
    cvFree( element );

    __END__;
}


typedef CvStatus (CV_STDCALL * CvMorphRectGetBufSizeFunc_IPP)
    ( int width, CvSize el_size, int* bufsize );

typedef CvStatus (CV_STDCALL * CvMorphRectFunc_IPP)
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize roi, CvSize el_size, CvPoint el_anchor, void* buffer );

typedef CvStatus (CV_STDCALL * CvMorphCustomInitAllocFunc_IPP)
    ( int width, const uchar* element, CvSize el_size,
      CvPoint el_anchor, void** morphstate );

typedef CvStatus (CV_STDCALL * CvMorphCustomFunc_IPP)
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize roi, int bordertype, void* morphstate );

static void
icvMorphOp( const void* srcarr, void* dstarr, IplConvKernel* element,
            int iterations, int mop )
{
    CvMorphology morphology;
    void* buffer = 0;
    int local_alloc = 0;
    void* morphstate = 0;
    CvMat* temp = 0;

    CV_FUNCNAME( "icvMorphOp" );

    __BEGIN__;

    int i, coi1 = 0, coi2 = 0;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat el_hdr, *el = 0;
    CvSize size, el_size;
    CvPoint el_anchor;
    int el_shape;
    int type;
    bool inplace;

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    
    if( src != &srcstub )
    {
        srcstub = *src;
        src = &srcstub;
    }

    if( dstarr == srcarr )
        dst = src;
    else
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

        if( !CV_ARE_TYPES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( !CV_ARE_SIZES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }

    if( dst != &dststub )
    {
        dststub = *dst;
        dst = &dststub;
    }

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    type = CV_MAT_TYPE( src->type );
    size = cvGetMatSize( src );
    inplace = src->data.ptr == dst->data.ptr;

    if( iterations == 0 || (element && element->nCols == 1 && element->nRows == 1))
    {
        if( src->data.ptr != dst->data.ptr )
            cvCopy( src, dst );
        EXIT;
    }

    if( element )
    {
        el_size = cvSize( element->nCols, element->nRows );
        el_anchor = cvPoint( element->anchorX, element->anchorY );
        el_shape = (int)(element->nShiftR);
        el_shape = el_shape < CV_SHAPE_CUSTOM ? el_shape : CV_SHAPE_CUSTOM;
    }
    else
    {
        el_size = cvSize(3,3);
        el_anchor = cvPoint(1,1);
        el_shape = CV_SHAPE_RECT;
    }

    if( el_shape == CV_SHAPE_RECT && iterations > 1 )
    {
        el_size.width = 1 + (el_size.width-1)*iterations;
        el_size.height = 1 + (el_size.height-1)*iterations;
        el_anchor.x *= iterations;
        el_anchor.y *= iterations;
        iterations = 1;
    }

    if( el_shape == CV_SHAPE_RECT && icvErodeRect_GetBufSize_8u_C1R_p )
    {
        CvMorphRectFunc_IPP rect_func = 0;
        CvMorphRectGetBufSizeFunc_IPP rect_getbufsize_func = 0;

        if( mop == 0 )
        {
            if( type == CV_8UC1 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_8u_C1R_p,
                rect_func = icvErodeRect_8u_C1R_p;
            else if( type == CV_8UC3 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_8u_C3R_p,
                rect_func = icvErodeRect_8u_C3R_p;
            else if( type == CV_8UC4 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_8u_C4R_p,
                rect_func = icvErodeRect_8u_C4R_p;
            else if( type == CV_16UC1 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_16u_C1R_p,
                rect_func = icvErodeRect_16u_C1R_p;
            else if( type == CV_16UC3 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_16u_C3R_p,
                rect_func = icvErodeRect_16u_C3R_p;
            else if( type == CV_16UC4 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_16u_C4R_p,
                rect_func = icvErodeRect_16u_C4R_p;
            else if( type == CV_32FC1 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_32f_C1R_p,
                rect_func = icvErodeRect_32f_C1R_p;
            else if( type == CV_32FC3 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_32f_C3R_p,
                rect_func = icvErodeRect_32f_C3R_p;
            else if( type == CV_32FC4 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_32f_C4R_p,
                rect_func = icvErodeRect_32f_C4R_p;
        }
        else
        {
            if( type == CV_8UC1 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_8u_C1R_p,
                rect_func = icvDilateRect_8u_C1R_p;
            else if( type == CV_8UC3 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_8u_C3R_p,
                rect_func = icvDilateRect_8u_C3R_p;
            else if( type == CV_8UC4 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_8u_C4R_p,
                rect_func = icvDilateRect_8u_C4R_p;
            else if( type == CV_16UC1 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_16u_C1R_p,
                rect_func = icvDilateRect_16u_C1R_p;
            else if( type == CV_16UC3 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_16u_C3R_p,
                rect_func = icvDilateRect_16u_C3R_p;
            else if( type == CV_16UC4 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_16u_C4R_p,
                rect_func = icvDilateRect_16u_C4R_p;
            else if( type == CV_32FC1 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_32f_C1R_p,
                rect_func = icvDilateRect_32f_C1R_p;
            else if( type == CV_32FC3 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_32f_C3R_p,
                rect_func = icvDilateRect_32f_C3R_p;
            else if( type == CV_32FC4 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_32f_C4R_p,
                rect_func = icvDilateRect_32f_C4R_p;
        }

        if( rect_getbufsize_func && rect_func )
        {
            int bufsize = 0;

            CvStatus status = rect_getbufsize_func( size.width, el_size, &bufsize );
            if( status >= 0 && bufsize > 0 )
            {
                if( bufsize < CV_MAX_LOCAL_SIZE )
                {
                    buffer = cvStackAlloc( bufsize );
                    local_alloc = 1;
                }
                else
                    CV_CALL( buffer = cvAlloc( bufsize ));
            }

            if( status >= 0 )
            {
                int src_step, dst_step = dst->step ? dst->step : CV_STUB_STEP;

                if( inplace )
                {
                    CV_CALL( temp = cvCloneMat( dst ));
                    src = temp;
                }
                src_step = src->step ? src->step : CV_STUB_STEP;

                status = rect_func( src->data.ptr, src_step, dst->data.ptr,
                                    dst_step, size, el_size, el_anchor, buffer );
            }
            
            if( status >= 0 )
                EXIT;
        }
    }
    else if( el_shape == CV_SHAPE_CUSTOM && icvMorphInitAlloc_8u_C1R_p && icvMorphFree_p &&
             src->data.ptr != dst->data.ptr )
    {
        CvMorphCustomFunc_IPP custom_func = 0;
        CvMorphCustomInitAllocFunc_IPP custom_initalloc_func = 0;
        const int bordertype = 1; // replication border

        if( type == CV_8UC1 )
            custom_initalloc_func = icvMorphInitAlloc_8u_C1R_p,
            custom_func = mop == 0 ? icvErode_8u_C1R_p : icvDilate_8u_C1R_p;
        else if( type == CV_8UC3 )
            custom_initalloc_func = icvMorphInitAlloc_8u_C3R_p,
            custom_func = mop == 0 ? icvErode_8u_C3R_p : icvDilate_8u_C3R_p;
        else if( type == CV_8UC4 )
            custom_initalloc_func = icvMorphInitAlloc_8u_C4R_p,
            custom_func = mop == 0 ? icvErode_8u_C4R_p : icvDilate_8u_C4R_p;
        else if( type == CV_16UC1 )
            custom_initalloc_func = icvMorphInitAlloc_16u_C1R_p,
            custom_func = mop == 0 ? icvErode_16u_C1R_p : icvDilate_16u_C1R_p;
        else if( type == CV_16UC3 )
            custom_initalloc_func = icvMorphInitAlloc_16u_C3R_p,
            custom_func = mop == 0 ? icvErode_16u_C3R_p : icvDilate_16u_C3R_p;
        else if( type == CV_16UC4 )
            custom_initalloc_func = icvMorphInitAlloc_16u_C4R_p,
            custom_func = mop == 0 ? icvErode_16u_C4R_p : icvDilate_16u_C4R_p;
        else if( type == CV_32FC1 )
            custom_initalloc_func = icvMorphInitAlloc_32f_C1R_p,
            custom_func = mop == 0 ? icvErode_32f_C1R_p : icvDilate_32f_C1R_p;
        else if( type == CV_32FC3 )
            custom_initalloc_func = icvMorphInitAlloc_32f_C3R_p,
            custom_func = mop == 0 ? icvErode_32f_C3R_p : icvDilate_32f_C3R_p;
        else if( type == CV_32FC4 )
            custom_initalloc_func = icvMorphInitAlloc_32f_C4R_p,
            custom_func = mop == 0 ? icvErode_32f_C4R_p : icvDilate_32f_C4R_p;

        if( custom_initalloc_func && custom_func )
        {
            uchar *src_ptr, *dst_ptr = dst->data.ptr;
            int src_step, dst_step = dst->step ? dst->step : CV_STUB_STEP;
            int el_len = el_size.width*el_size.height;
            uchar* el_mask = (uchar*)cvStackAlloc( el_len );
            CvStatus status;

            for( i = 0; i < el_len; i++ )
                el_mask[i] = (uchar)(element->values[i] != 0);

            status = custom_initalloc_func( size.width, el_mask, el_size,
                                            el_anchor, &morphstate );

            if( status >= 0 && (inplace || iterations > 1) )
            {
                CV_CALL( temp = cvCloneMat( src ));
                src = temp;
            }

            src_ptr = src->data.ptr;
            src_step = src->step ? src->step : CV_STUB_STEP;

            for( i = 0; i < iterations && status >= 0 && morphstate; i++ )
            {
                uchar* t_ptr;
                int t_step;
                status = custom_func( src_ptr, src_step, dst_ptr, dst_step,
                                      size, bordertype, morphstate );
                CV_SWAP( src_ptr, dst_ptr, t_ptr );
                CV_SWAP( src_step, dst_step, t_step );
                if( i == 0 && temp )
                {
                    dst_ptr = temp->data.ptr;
                    dst_step = temp->step ? temp->step : CV_STUB_STEP;
                }
            }

            if( status >= 0 )
            {
                if( iterations % 2 == 0 )
                    cvCopy( temp, dst );
                EXIT;
            }
        }
    }

    if( el_shape != CV_SHAPE_RECT )
    {
        el_hdr = cvMat( element->nRows, element->nCols, CV_32SC1, element->values );
        el = &el_hdr;
        el_shape = CV_SHAPE_CUSTOM;
    }

    CV_CALL( morphology.init( mop, src->cols, src->type,
                    el_shape, el, el_size, el_anchor ));

    for( i = 0; i < iterations; i++ )
    {
        CV_CALL( morphology.process( src, dst ));
        src = dst;
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
    if( morphstate )
        icvMorphFree_p( morphstate );
    cvReleaseMat( &temp );
}


CV_IMPL void
cvErode( const void* src, void* dst, IplConvKernel* element, int iterations )
{
    icvMorphOp( src, dst, element, iterations, 0 );
}


CV_IMPL void
cvDilate( const void* src, void* dst, IplConvKernel* element, int iterations )
{
    icvMorphOp( src, dst, element, iterations, 1 );
}


CV_IMPL void
cvMorphologyEx( const void* src, void* dst,
                void* temp, IplConvKernel* element, int op, int iterations )
{
    CV_FUNCNAME( "cvMorhologyEx" );

    __BEGIN__;

    if( (op == CV_MOP_GRADIENT ||
        ((op == CV_MOP_TOPHAT || op == CV_MOP_BLACKHAT) && src == dst)) && temp == 0 )
        CV_ERROR( CV_HeaderIsNull, "temp image required" );

    if( temp == src || temp == dst )
        CV_ERROR( CV_HeaderIsNull, "temp image is equal to src or dst" );

    switch (op)
    {
    case CV_MOP_OPEN:
        CV_CALL( cvErode( src, dst, element, iterations ));
        CV_CALL( cvDilate( dst, dst, element, iterations ));
        break;
    case CV_MOP_CLOSE:
        CV_CALL( cvDilate( src, dst, element, iterations ));
        CV_CALL( cvErode( dst, dst, element, iterations ));
        break;
    case CV_MOP_GRADIENT:
        CV_CALL( cvErode( src, temp, element, iterations ));
        CV_CALL( cvDilate( src, dst, element, iterations ));
        CV_CALL( cvSub( dst, temp, dst ));
        break;
    case CV_MOP_TOPHAT:
        if( src != dst )
            temp = dst;
        CV_CALL( cvErode( src, temp, element, iterations ));
        CV_CALL( cvDilate( temp, temp, element, iterations ));
        CV_CALL( cvSub( src, temp, dst ));
        break;
    case CV_MOP_BLACKHAT:
        if( src != dst )
            temp = dst;
        CV_CALL( cvDilate( src, temp, element, iterations ));
        CV_CALL( cvErode( temp, temp, element, iterations ));
        CV_CALL( cvSub( temp, src, dst ));
        break;
    default:
        CV_ERROR( CV_StsBadArg, "unknown morphological operation" );
    }

    __END__;
}

/* End of file. */
