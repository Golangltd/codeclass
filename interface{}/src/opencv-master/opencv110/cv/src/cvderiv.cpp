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

/****************************************************************************************/

/* lightweight convolution with 3x3 kernel */
void icvSepConvSmall3_32f( float* src, int src_step, float* dst, int dst_step,
            CvSize src_size, const float* kx, const float* ky, float* buffer )
{
    int  dst_width, buffer_step = 0;
    int  x, y;

    assert( src && dst && src_size.width > 2 && src_size.height > 2 &&
            (src_step & 3) == 0 && (dst_step & 3) == 0 &&
            (kx || ky) && (buffer || !kx || !ky));

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    dst_width = src_size.width - 2;

    if( !kx )
    {
        /* set vars, so that vertical convolution
           will write results into destination ROI and
           horizontal convolution won't run */
        src_size.width = dst_width;
        buffer_step = dst_step;
        buffer = dst;
        dst_width = 0;
    }

    assert( src_step >= src_size.width && dst_step >= dst_width );

    src_size.height -= 3;
    if( !ky )
    {
        /* set vars, so that vertical convolution won't run and
           horizontal convolution will write results into destination ROI */
        src_size.height += 3;
        buffer_step = src_step;
        buffer = src;
        src_size.width = 0;
    }

    for( y = 0; y <= src_size.height; y++, src += src_step,
                                           dst += dst_step,
                                           buffer += buffer_step )
    {
        float* src2 = src + src_step;
        float* src3 = src + src_step*2;
        for( x = 0; x < src_size.width; x++ )
        {
            buffer[x] = (float)(ky[0]*src[x] + ky[1]*src2[x] + ky[2]*src3[x]);
        }

        for( x = 0; x < dst_width; x++ )
        {
            dst[x] = (float)(kx[0]*buffer[x] + kx[1]*buffer[x+1] + kx[2]*buffer[x+2]);
        }
    }
}


/****************************************************************************************\
                             Sobel & Scharr Derivative Filters
\****************************************************************************************/

/////////////////////////////// Old IPP derivative filters ///////////////////////////////
// still used in corner detectors (see cvcorner.cpp)

icvFilterSobelVert_8u16s_C1R_t icvFilterSobelVert_8u16s_C1R_p = 0;
icvFilterSobelHoriz_8u16s_C1R_t icvFilterSobelHoriz_8u16s_C1R_p = 0;
icvFilterSobelVertSecond_8u16s_C1R_t icvFilterSobelVertSecond_8u16s_C1R_p = 0;
icvFilterSobelHorizSecond_8u16s_C1R_t icvFilterSobelHorizSecond_8u16s_C1R_p = 0;
icvFilterSobelCross_8u16s_C1R_t icvFilterSobelCross_8u16s_C1R_p = 0;

icvFilterSobelVert_32f_C1R_t icvFilterSobelVert_32f_C1R_p = 0;
icvFilterSobelHoriz_32f_C1R_t icvFilterSobelHoriz_32f_C1R_p = 0;
icvFilterSobelVertSecond_32f_C1R_t icvFilterSobelVertSecond_32f_C1R_p = 0;
icvFilterSobelHorizSecond_32f_C1R_t icvFilterSobelHorizSecond_32f_C1R_p = 0;
icvFilterSobelCross_32f_C1R_t icvFilterSobelCross_32f_C1R_p = 0;

icvFilterScharrVert_8u16s_C1R_t icvFilterScharrVert_8u16s_C1R_p = 0;
icvFilterScharrHoriz_8u16s_C1R_t icvFilterScharrHoriz_8u16s_C1R_p = 0;
icvFilterScharrVert_32f_C1R_t icvFilterScharrVert_32f_C1R_p = 0;
icvFilterScharrHoriz_32f_C1R_t icvFilterScharrHoriz_32f_C1R_p = 0;

///////////////////////////////// New IPP derivative filters /////////////////////////////

#define IPCV_FILTER_PTRS( name )                    \
icvFilter##name##GetBufSize_8u16s_C1R_t             \
    icvFilter##name##GetBufSize_8u16s_C1R_p = 0;    \
icvFilter##name##Border_8u16s_C1R_t                 \
    icvFilter##name##Border_8u16s_C1R_p = 0;        \
icvFilter##name##GetBufSize_32f_C1R_t               \
    icvFilter##name##GetBufSize_32f_C1R_p = 0;      \
icvFilter##name##Border_32f_C1R_t                   \
    icvFilter##name##Border_32f_C1R_p = 0;

IPCV_FILTER_PTRS( ScharrHoriz )
IPCV_FILTER_PTRS( ScharrVert )
IPCV_FILTER_PTRS( SobelHoriz )
IPCV_FILTER_PTRS( SobelNegVert )
IPCV_FILTER_PTRS( SobelHorizSecond )
IPCV_FILTER_PTRS( SobelVertSecond )
IPCV_FILTER_PTRS( SobelCross )
IPCV_FILTER_PTRS( Laplacian )

typedef CvStatus (CV_STDCALL * CvDeriv3x3GetBufSizeIPPFunc)
    ( CvSize roi, int* bufsize );

typedef CvStatus (CV_STDCALL * CvDerivGetBufSizeIPPFunc)
    ( CvSize roi, int masksize, int* bufsize );

typedef CvStatus (CV_STDCALL * CvDeriv3x3IPPFunc_8u )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int bordertype, uchar bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDeriv3x3IPPFunc_32f )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int bordertype, float bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDerivIPPFunc_8u )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int masksize, int bordertype,
      uchar bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDerivIPPFunc_32f )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int masksize, int bordertype,
      float bordervalue, void* buffer );

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvSobel( const void* srcarr, void* dstarr, int dx, int dy, int aperture_size )
{
    CvSepFilter filter;
    void* buffer = 0;
    int local_alloc = 0;

    CV_FUNCNAME( "cvSobel" );

    __BEGIN__;

    int origin = 0;
    int src_type, dst_type;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub ));
    if( !CV_IS_MAT(dst) )
        CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( CV_IS_IMAGE_HDR( srcarr ))
        origin = ((IplImage*)srcarr)->origin;

    src_type = CV_MAT_TYPE( src->type );
    dst_type = CV_MAT_TYPE( dst->type );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsBadArg, "src and dst have different sizes" );

    if( ((aperture_size == CV_SCHARR || aperture_size == 3 || aperture_size == 5) &&
        dx <= 2 && dy <= 2 && dx + dy <= 2 && icvFilterSobelNegVertBorder_8u16s_C1R_p) &&
        (src_type == CV_8UC1 && dst_type == CV_16SC1/* ||
        src_type == CV_32FC1 && dst_type == CV_32FC1*/) )
    {
        CvDerivGetBufSizeIPPFunc ipp_sobel_getbufsize_func = 0;
        CvDerivIPPFunc_8u ipp_sobel_func_8u = 0;
        CvDerivIPPFunc_32f ipp_sobel_func_32f = 0;

        CvDeriv3x3GetBufSizeIPPFunc ipp_scharr_getbufsize_func = 0;
        CvDeriv3x3IPPFunc_8u ipp_scharr_func_8u = 0;
        CvDeriv3x3IPPFunc_32f ipp_scharr_func_32f = 0;

        if( aperture_size == CV_SCHARR )
        {
            if( dx == 1 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_scharr_func_8u = icvFilterScharrVertBorder_8u16s_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrVertGetBufSize_8u16s_C1R_p;
                else
                    ipp_scharr_func_32f = icvFilterScharrVertBorder_32f_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrVertGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_scharr_func_8u = icvFilterScharrHorizBorder_8u16s_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrHorizGetBufSize_8u16s_C1R_p;
                else
                    ipp_scharr_func_32f = icvFilterScharrHorizBorder_32f_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrHorizGetBufSize_32f_C1R_p;
            }
            else
                CV_ERROR( CV_StsBadArg, "Scharr filter can only be used to compute 1st image derivatives" );
        }
        else
        {
            if( dx == 1 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelNegVertBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelNegVertGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelNegVertBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelNegVertGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelHorizBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelHorizBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizGetBufSize_32f_C1R_p;
            }
            else if( dx == 2 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelVertSecondBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelVertSecondGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelVertSecondBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelVertSecondGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 2 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelHorizSecondBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizSecondGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelHorizSecondBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizSecondGetBufSize_32f_C1R_p;
            }
            else if( dx == 1 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelCrossBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelCrossGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelCrossBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelCrossGetBufSize_32f_C1R_p;
            }
        }

        if( ((ipp_sobel_func_8u || ipp_sobel_func_32f) && ipp_sobel_getbufsize_func) ||
            ((ipp_scharr_func_8u || ipp_scharr_func_32f) && ipp_scharr_getbufsize_func) )
        {
            int bufsize = 0, masksize = aperture_size == 3 ? 33 : 55;
            CvSize size = cvGetMatSize( src );
            uchar* src_ptr = src->data.ptr;
            uchar* dst_ptr = dst->data.ptr;
            int src_step = src->step ? src->step : CV_STUB_STEP;
            int dst_step = dst->step ? dst->step : CV_STUB_STEP;
            const int bordertype = 1; // replication border
            CvStatus status;

            status = ipp_sobel_getbufsize_func ?
                ipp_sobel_getbufsize_func( size, masksize, &bufsize ) :
                ipp_scharr_getbufsize_func( size, &bufsize );

            if( status >= 0 )
            {
                if( bufsize <= CV_MAX_LOCAL_SIZE )
                {
                    buffer = cvStackAlloc( bufsize );
                    local_alloc = 1;
                }
                else
                    CV_CALL( buffer = cvAlloc( bufsize ));
            
                status =
                    ipp_sobel_func_8u ? ipp_sobel_func_8u( src_ptr, src_step, dst_ptr, dst_step,
                                                           size, masksize, bordertype, 0, buffer ) :
                    ipp_sobel_func_32f ? ipp_sobel_func_32f( src_ptr, src_step, dst_ptr, dst_step,
                                                             size, masksize, bordertype, 0, buffer ) :
                    ipp_scharr_func_8u ? ipp_scharr_func_8u( src_ptr, src_step, dst_ptr, dst_step,
                                                             size, bordertype, 0, buffer ) :
                    ipp_scharr_func_32f ? ipp_scharr_func_32f( src_ptr, src_step, dst_ptr, dst_step,
                                                               size, bordertype, 0, buffer ) : 
                        CV_NOTDEFINED_ERR;
            }

            if( status >= 0 &&
                ((dx == 0 && dy == 1 && origin) || (dx == 1 && dy == 1 && !origin))) // negate the output
                cvSubRS( dst, cvScalarAll(0), dst );

            if( status >= 0 )
                EXIT;
        }
    }

    CV_CALL( filter.init_deriv( src->cols, src_type, dst_type, dx, dy,
                aperture_size, origin ? CvSepFilter::FLIP_KERNEL : 0));
    CV_CALL( filter.process( src, dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}


/****************************************************************************************\
                                     Laplacian Filter
\****************************************************************************************/

static void icvLaplaceRow_8u32s( const uchar* src, int* dst, void* params );
static void icvLaplaceRow_8u32f( const uchar* src, float* dst, void* params );
static void icvLaplaceRow_32f( const float* src, float* dst, void* params );
static void icvLaplaceCol_32s16s( const int** src, short* dst, int dst_step,
                                  int count, void* params );
static void icvLaplaceCol_32f( const float** src, float* dst, int dst_step,
                               int count, void* params );

CvLaplaceFilter::CvLaplaceFilter()
{
    normalized = basic_laplacian = false;
}


CvLaplaceFilter::CvLaplaceFilter( int _max_width, int _src_type, int _dst_type, bool _normalized,
                                  int _ksize, int _border_mode, CvScalar _border_value )
{
    normalized = basic_laplacian = false;
    init( _max_width, _src_type, _dst_type, _normalized, _ksize, _border_mode, _border_value );
}


CvLaplaceFilter::~CvLaplaceFilter()
{
    clear();
}


void CvLaplaceFilter::get_work_params()
{
    int min_rows = max_ky*2 + 3, rows = MAX(min_rows,10), row_sz;
    int width = max_width, trow_sz = 0;
    int dst_depth = CV_MAT_DEPTH(dst_type);
    int work_depth = dst_depth < CV_32F ? CV_32S : CV_32F;
    work_type = CV_MAKETYPE( work_depth, CV_MAT_CN(dst_type)*2 );
    trow_sz = cvAlign( (max_width + ksize.width - 1)*CV_ELEM_SIZE(src_type), ALIGN );
    row_sz = cvAlign( width*CV_ELEM_SIZE(work_type), ALIGN );
    buf_size = rows*row_sz;
    buf_size = MIN( buf_size, 1 << 16 );
    buf_size = MAX( buf_size, min_rows*row_sz );
    max_rows = (buf_size/row_sz)*3 + max_ky*2 + 8;
    buf_size += trow_sz;
}


void CvLaplaceFilter::init( int _max_width, int _src_type, int _dst_type, bool _normalized,
                            int _ksize0, int _border_mode, CvScalar _border_value )
{
    CvMat *kx = 0, *ky = 0;

    CV_FUNCNAME( "CvLaplaceFilter::init" );

    __BEGIN__;

    int src_depth = CV_MAT_DEPTH(_src_type), dst_depth = CV_MAT_DEPTH(_dst_type);
    int _ksize = MAX( _ksize0, 3 );

    normalized = _normalized;
    basic_laplacian = _ksize0 == 1;

    if( ((src_depth != CV_8U || (dst_depth != CV_16S && dst_depth != CV_32F)) &&
        (src_depth != CV_32F || dst_depth != CV_32F)) ||
        CV_MAT_CN(_src_type) != CV_MAT_CN(_dst_type) )
        CV_ERROR( CV_StsUnmatchedFormats,
        "Laplacian can either transform 8u->16s, or 8u->32f, or 32f->32f.\n"
        "The number of channels must be the same." );

    if( _ksize < 1 || _ksize > CV_MAX_SOBEL_KSIZE || _ksize % 2 == 0 )
        CV_ERROR( CV_StsOutOfRange, "kernel size must be within 1..7 and odd" );

    CV_CALL( kx = cvCreateMat( 1, _ksize, CV_32F ));
    CV_CALL( ky = cvCreateMat( 1, _ksize, CV_32F ));

    CvSepFilter::init_sobel_kernel( kx, ky, 2, 0, 0 );
    CvSepFilter::init( _max_width, _src_type, _dst_type, kx, ky,
                       cvPoint(-1,-1), _border_mode, _border_value );

    x_func = 0;
    y_func = 0;

    if( src_depth == CV_8U )
    {
        if( dst_depth == CV_16S )
        {
            x_func = (CvRowFilterFunc)icvLaplaceRow_8u32s;
            y_func = (CvColumnFilterFunc)icvLaplaceCol_32s16s;
        }
        else if( dst_depth == CV_32F )
        {
            x_func = (CvRowFilterFunc)icvLaplaceRow_8u32f;
            y_func = (CvColumnFilterFunc)icvLaplaceCol_32f;
        }
    }
    else if( src_depth == CV_32F )
    {
        if( dst_depth == CV_32F )
        {
            x_func = (CvRowFilterFunc)icvLaplaceRow_32f;
            y_func = (CvColumnFilterFunc)icvLaplaceCol_32f;
        }
    }

    if( !x_func || !y_func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    __END__;

    cvReleaseMat( &kx );
    cvReleaseMat( &ky );
}


void CvLaplaceFilter::init( int _max_width, int _src_type, int _dst_type,
                            bool _is_separable, CvSize _ksize,
                            CvPoint _anchor, int _border_mode,
                            CvScalar _border_value )
{
    CvSepFilter::init( _max_width, _src_type, _dst_type, _is_separable,
                       _ksize, _anchor, _border_mode, _border_value );
}


void CvLaplaceFilter::init( int _max_width, int _src_type, int _dst_type,
                            const CvMat* _kx, const CvMat* _ky,
                            CvPoint _anchor, int _border_mode,
                            CvScalar _border_value )
{
    CvSepFilter::init( _max_width, _src_type, _dst_type, _kx, _ky,
                       _anchor, _border_mode, _border_value );
}


#define ICV_LAPLACE_ROW( flavor, srctype, dsttype, load_macro )         \
static void                                                             \
icvLaplaceRow_##flavor( const srctype* src, dsttype* dst, void* params )\
{                                                                       \
    const CvLaplaceFilter* state = (const CvLaplaceFilter*)params;      \
    const CvMat* _kx = state->get_x_kernel();                           \
    const CvMat* _ky = state->get_y_kernel();                           \
    const dsttype* kx = (dsttype*)_kx->data.ptr;                        \
    const dsttype* ky = (dsttype*)_ky->data.ptr;                        \
    int ksize = _kx->cols + _kx->rows - 1;                              \
    int i = 0, j, k, width = state->get_width();                        \
    int cn = CV_MAT_CN(state->get_src_type());                          \
    int ksize2 = ksize/2, ksize2n = ksize2*cn;                          \
    const srctype* s = src + ksize2n;                                   \
    bool basic_laplacian = state->is_basic_laplacian();                 \
                                                                        \
    kx += ksize2;                                                       \
    ky += ksize2;                                                       \
    width *= cn;                                                        \
                                                                        \
    if( basic_laplacian )                                               \
        for( i = 0; i < width; i++ )                                    \
        {                                                               \
            dsttype s0 = load_macro(s[i]);                              \
            dsttype s1 = (dsttype)(s[i-cn] - s0*2 + s[i+cn]);           \
            dst[i] = s0; dst[i+width] = s1;                             \
        }                                                               \
    else if( ksize == 3 )                                               \
        for( i = 0; i < width; i++ )                                    \
        {                                                               \
            dsttype s0 = (dsttype)(s[i-cn] + s[i]*2 + s[i+cn]);         \
            dsttype s1 = (dsttype)(s[i-cn] - s[i]*2 + s[i+cn]);         \
            dst[i] = s0; dst[i+width] = s1;                             \
        }                                                               \
    else if( ksize == 5 )                                               \
        for( i = 0; i < width; i++ )                                    \
        {                                                               \
            dsttype s0 = (dsttype)(s[i-2*cn]+(s[i-cn]+s[i+cn])*4+s[i]*6+s[i+2*cn]);\
            dsttype s1 = (dsttype)(s[i-2*cn]-s[i]*2+s[i+2*cn]);         \
            dst[i] = s0; dst[i+width] = s1;                             \
        }                                                               \
    else                                                                \
        for( i = 0; i < width; i++, s++ )                               \
        {                                                               \
            dsttype s0 = ky[0]*load_macro(s[0]), s1 = kx[0]*load_macro(s[0]);\
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )             \
            {                                                           \
                dsttype t = load_macro(s[j] + s[-j]);                   \
                s0 += ky[k]*t; s1 += kx[k]*t;                           \
            }                                                           \
            dst[i] = s0; dst[i+width] = s1;                             \
        }                                                               \
}

ICV_LAPLACE_ROW( 8u32s, uchar, int, CV_NOP )
ICV_LAPLACE_ROW( 8u32f, uchar, float, CV_8TO32F )
ICV_LAPLACE_ROW( 32f, float, float, CV_NOP )

static void
icvLaplaceCol_32s16s( const int** src, short* dst,
                      int dst_step, int count, void* params )
{
    const CvLaplaceFilter* state = (const CvLaplaceFilter*)params;
    const CvMat* _kx = state->get_x_kernel();
    const CvMat* _ky = state->get_y_kernel();
    const int* kx = (const int*)_kx->data.ptr;
    const int* ky = (const int*)_ky->data.ptr;
    int ksize = _kx->cols + _kx->rows - 1, ksize2 = ksize/2;
    int i = 0, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    bool basic_laplacian = state->is_basic_laplacian();
    bool normalized = state->is_normalized();
    int shift = ksize - 1, delta = (1 << shift) >> 1;
    
    width *= cn;
    src += ksize2;
    kx += ksize2;
    ky += ksize2;
    dst_step /= sizeof(dst[0]);

    if( basic_laplacian || !normalized )
    {
        normalized = false;
        shift = delta = 0;
    }

    for( ; count--; dst += dst_step, src++ )
    {
        if( ksize == 3 )
        {
            const int *src0 = src[-1], *src1 = src[0], *src2 = src[1];
            if( basic_laplacian )
            {
                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src0[i] - src1[i]*2 + src2[i] + src1[i+width];
                    int s1 = src0[i+1] - src1[i+1]*2 + src2[i+1] + src1[i+width+1];
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }

                for( ; i < width; i++ )
                    dst[i] = (short)(src0[i] - src1[i]*2 + src2[i] + src1[i+width]);
            }
            else if( !normalized )
                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src0[i] - src1[i]*2 + src2[i] +
                             src0[i+width] + src1[i+width]*2 + src2[i+width];
                    int s1 = src0[i+1] - src1[i+1]*2 + src2[i+1] +
                             src0[i+width+1] + src1[i+width+1]*2 + src2[i+width+1];
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            else
                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = CV_DESCALE(src0[i] - src1[i]*2 + src2[i] +
                             src0[i+width] + src1[i+width]*2 + src2[i+width], 2);
                    int s1 = CV_DESCALE(src0[i+1] - src1[i+1]*2 + src2[i+1] +
                             src0[i+width+1] + src1[i+width+1]*2 + src2[i+width+1],2);
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
        }
        else if( ksize == 5 )
        {
            const int *src0 = src[-2], *src1 = src[-1], *src2 = src[0], *src3 = src[1], *src4 = src[2];

            if( !normalized )
                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src0[i] - src2[i]*2 + src4[i] + src0[i+width] + src4[i+width] +
                             (src1[i+width] + src3[i+width])*4 + src2[i+width]*6;
                    int s1 = src0[i+1] - src2[i+1]*2 + src4[i+1] + src0[i+width+1] +
                             src4[i+width+1] + (src1[i+width+1] + src3[i+width+1])*4 +
                             src2[i+width+1]*6;
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            else
                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = CV_DESCALE(src0[i] - src2[i]*2 + src4[i] +
                             src0[i+width] + src4[i+width] +
                             (src1[i+width] + src3[i+width])*4 + src2[i+width]*6, 4);
                    int s1 = CV_DESCALE(src0[i+1] - src2[i+1]*2 + src4[i+1] +
                             src0[i+width+1] + src4[i+width+1] +
                             (src1[i+width+1] + src3[i+width+1])*4 + src2[i+width+1]*6, 4);
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
        }
        else
        {
            if( !normalized )
                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = kx[0]*src[0][i] + ky[0]*src[0][i+width];
                    int s1 = kx[0]*src[0][i+1] + ky[0]*src[0][i+width+1];

                    for( k = 1; k <= ksize2; k++ )
                    {
                        const int* src1 = src[k] + i, *src2 = src[-k] + i;
                        int fx = kx[k], fy = ky[k];
                        s0 += fx*(src1[0] + src2[0]) + fy*(src1[width] + src2[width]);
                        s1 += fx*(src1[1] + src2[1]) + fy*(src1[width+1] + src2[width+1]);
                    }

                    dst[i] = CV_CAST_16S(s0); dst[i+1] = CV_CAST_16S(s1);
                }
            else
                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = kx[0]*src[0][i] + ky[0]*src[0][i+width];
                    int s1 = kx[0]*src[0][i+1] + ky[0]*src[0][i+width+1];

                    for( k = 1; k <= ksize2; k++ )
                    {
                        const int* src1 = src[k] + i, *src2 = src[-k] + i;
                        int fx = kx[k], fy = ky[k];
                        s0 += fx*(src1[0] + src2[0]) + fy*(src1[width] + src2[width]);
                        s1 += fx*(src1[1] + src2[1]) + fy*(src1[width+1] + src2[width+1]);
                    }

                    s0 = CV_DESCALE( s0, shift ); s1 = CV_DESCALE( s1, shift );
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
        }

        for( ; i < width; i++ )
        {
            int s0 = kx[0]*src[0][i] + ky[0]*src[0][i+width];
            for( k = 1; k <= ksize2; k++ )
            {
                const int* src1 = src[k] + i, *src2 = src[-k] + i;
                s0 += kx[k]*(src1[0] + src2[0]) + ky[k]*(src1[width] + src2[width]);
            }
            s0 = (s0 + delta) >> shift;
            dst[i] = CV_CAST_16S(s0);
        }
    }
}


static void
icvLaplaceCol_32f( const float** src, float* dst,
                   int dst_step, int count, void* params )
{
    const CvLaplaceFilter* state = (const CvLaplaceFilter*)params;
    const CvMat* _kx = state->get_x_kernel();
    const CvMat* _ky = state->get_y_kernel();
    const float* kx = (const float*)_kx->data.ptr;
    const float* ky = (const float*)_ky->data.ptr;
    int ksize = _kx->cols + _kx->rows - 1, ksize2 = ksize/2;
    int i = 0, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    bool basic_laplacian = state->is_basic_laplacian();
    bool normalized = state->is_normalized();
    float scale = 1.f/(1 << (ksize - 1));
    
    width *= cn;
    src += ksize2;
    kx += ksize2;
    ky += ksize2;
    dst_step /= sizeof(dst[0]);

    if( basic_laplacian || !normalized )
    {
        normalized = false;
        scale = 1.f;
    }

    for( ; count--; dst += dst_step, src++ )
    {
        if( ksize == 3 )
        {
            const float *src0 = src[-1], *src1 = src[0], *src2 = src[1];
            if( basic_laplacian )
            {
                for( i = 0; i <= width - 2; i += 2 )
                {
                    float s0 = src0[i] - src1[i]*2 + src2[i] + src1[i+width];
                    float s1 = src0[i+1] - src1[i+1]*2 + src2[i+1] + src1[i+width+1];
                    dst[i] = s0; dst[i+1] = s1;
                }

                for( ; i < width; i++ )
                    dst[i] = src0[i] - src1[i]*2 + src2[i] + src1[i+width];
            }
            else if( !normalized )
                for( i = 0; i <= width - 2; i += 2 )
                {
                    float s0 = src0[i] - src1[i]*2 + src2[i] +
                             src0[i+width] + src1[i+width]*2 + src2[i+width];
                    float s1 = src0[i+1] - src1[i+1]*2 + src2[i+1] +
                             src0[i+width+1] + src1[i+width+1]*2 + src2[i+width+1];
                    dst[i] = s0; dst[i+1] = s1;
                }
            else
                for( i = 0; i <= width - 2; i += 2 )
                {
                    float s0 = (src0[i] - src1[i]*2 + src2[i] +
                             src0[i+width] + src1[i+width]*2 + src2[i+width])*scale;
                    float s1 = (src0[i+1] - src1[i+1]*2 + src2[i+1] +
                             src0[i+width+1] + src1[i+width+1]*2 + src2[i+width+1])*scale;
                    dst[i] = s0; dst[i+1] = s1;
                }
        }
        else if( ksize == 5 )
        {
            const float *src0 = src[-2], *src1 = src[-1], *src2 = src[0], *src3 = src[1], *src4 = src[2];
            for( i = 0; i <= width - 2; i += 2 )
            {
                float s0 = (src0[i] - src2[i]*2 + src4[i] +
                         src0[i+width] + src4[i+width] +
                         (src1[i+width] + src3[i+width])*4 + src2[i+width]*6)*scale;
                float s1 = (src0[i+1] - src2[i+1]*2 + src4[i+1] +
                         src0[i+width+1] + src4[i+width+1] +
                         (src1[i+width+1] + src3[i+width+1])*4 + src2[i+width+1]*6)*scale;
                dst[i] = s0; dst[i+1] = s1;
            }
        }
        else
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                float s0 = kx[0]*src[0][i] + ky[0]*src[0][i+width];
                float s1 = kx[0]*src[0][i+1] + ky[0]*src[0][i+width+1];

                for( k = 1; k <= ksize2; k++ )
                {
                    const float* src1 = src[k] + i, *src2 = src[-k] + i;
                    float fx = kx[k], fy = ky[k];
                    s0 += fx*(src1[0] + src2[0]) + fy*(src1[width] + src2[width]);
                    s1 += fx*(src1[1] + src2[1]) + fy*(src1[width+1] + src2[width+1]);
                }

                s0 *= scale; s1 *= scale;
                dst[i] = s0; dst[i+1] = s1;
            }
        }

        for( ; i < width; i++ )
        {
            float s0 = kx[0]*src[0][i] + ky[0]*src[0][i+width];
            for( k = 1; k <= ksize2; k++ )
            {
                const float* src1 = src[k] + i, *src2 = src[-k] + i;
                s0 += kx[k]*(src1[0] + src2[0]) + ky[k]*(src1[width] + src2[width]);
            }
            dst[i] = s0*scale;
        }
    }
}


CV_IMPL void
cvLaplace( const void* srcarr, void* dstarr, int aperture_size )
{
    CvLaplaceFilter laplacian;
    void* buffer = 0;
    int local_alloc = 0;
    
    CV_FUNCNAME( "cvLaplace" );

    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    int src_type, dst_type;

    CV_CALL( src = cvGetMat( src, &srcstub ));
    CV_CALL( dst = cvGetMat( dst, &dststub ));

    src_type = CV_MAT_TYPE(src->type);
    dst_type = CV_MAT_TYPE(dst->type);

    if( (aperture_size == 3 || aperture_size == 5) &&
        (src_type == CV_8UC1 && dst_type == CV_16SC1/* ||
        src_type == CV_32FC1 && dst_type == CV_32FC1*/) )
    {
        CvDerivGetBufSizeIPPFunc ipp_laplace_getbufsize_func = 0;
        CvDerivIPPFunc_8u ipp_laplace_func_8u = 0;
        CvDerivIPPFunc_32f ipp_laplace_func_32f = 0;

        if( src_type == CV_8U )
            ipp_laplace_func_8u = icvFilterLaplacianBorder_8u16s_C1R_p,
            ipp_laplace_getbufsize_func = icvFilterLaplacianGetBufSize_8u16s_C1R_p;
        else
            ipp_laplace_func_32f = icvFilterLaplacianBorder_32f_C1R_p,
            ipp_laplace_getbufsize_func = icvFilterLaplacianGetBufSize_32f_C1R_p;

        if( (ipp_laplace_func_8u || ipp_laplace_func_32f) && ipp_laplace_getbufsize_func )
        {
            int bufsize = 0, masksize = aperture_size == 3 ? 33 : 55;
            CvSize size = cvGetMatSize( src );
            uchar* src_ptr = src->data.ptr;
            uchar* dst_ptr = dst->data.ptr;
            int src_step = src->step ? src->step : CV_STUB_STEP;
            int dst_step = dst->step ? dst->step : CV_STUB_STEP;
            const int bordertype = 1; // replication border
            CvStatus status;

            status = ipp_laplace_getbufsize_func( size, masksize, &bufsize );

            if( status >= 0 )
            {
                if( bufsize <= CV_MAX_LOCAL_SIZE )
                {
                    buffer = cvStackAlloc( bufsize );
                    local_alloc = 1;
                }
                else
                    CV_CALL( buffer = cvAlloc( bufsize ));
            
                status =
                    ipp_laplace_func_8u ? ipp_laplace_func_8u( src_ptr, src_step, dst_ptr, dst_step,
                                                               size, masksize, bordertype, 0, buffer ) :
                    ipp_laplace_func_32f ? ipp_laplace_func_32f( src_ptr, src_step, dst_ptr, dst_step,
                                                                 size, masksize, bordertype, 0, buffer ) :
                        CV_NOTDEFINED_ERR;
            }

            if( status >= 0 )
                EXIT;
        }
    }

    CV_CALL( laplacian.init( src->cols, src_type, dst_type,
                             false, aperture_size ));
    CV_CALL( laplacian.process( src, dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}

/* End of file. */
