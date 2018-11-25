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
*                           [scaled] Identity matrix initialization                      *
\****************************************************************************************/

CV_IMPL void
cvSetIdentity( CvArr* array, CvScalar value )
{
    CV_FUNCNAME( "cvSetIdentity" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)array;
    CvSize size;
    int i, k, len, step;
    int type, pix_size;
    uchar* data = 0;
    double buf[4];

    if( !CV_IS_MAT( mat ))
    {
        int coi = 0;
        CV_CALL( mat = cvGetMat( mat, &stub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    size = cvGetMatSize( mat );
    len = CV_IMIN( size.width, size.height );

    type = CV_MAT_TYPE(mat->type);
    pix_size = CV_ELEM_SIZE(type);
    size.width *= pix_size;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;
        size.height = 1;
    }

    data = mat->data.ptr;
    step = mat->step;
    if( step == 0 )
        step = CV_STUB_STEP;
    IPPI_CALL( icvSetZero_8u_C1R( data, step, size ));
    step += pix_size;

    if( type == CV_32FC1 )
    {
        float val = (float)value.val[0];
        float* _data = (float*)data;
        step /= sizeof(_data[0]);
        len *= step;

        for( i = 0; i < len; i += step )
            _data[i] = val;
    }
    else if( type == CV_64FC1 )
    {
        double val = value.val[0];
        double* _data = (double*)data;
        step /= sizeof(_data[0]);
        len *= step;

        for( i = 0; i < len; i += step )
            _data[i] = val;
    }
    else
    {
        uchar* val_ptr = (uchar*)buf;
        cvScalarToRawData( &value, buf, type, 0 );
        len *= step;

        for( i = 0; i < len; i += step )
            for( k = 0; k < pix_size; k++ )
                data[i+k] = val_ptr[k];
    }

    __END__;
}


/****************************************************************************************\
*                                    Trace of the matrix                                 *
\****************************************************************************************/

CV_IMPL CvScalar
cvTrace( const CvArr* array )
{
    CvScalar sum = {{0,0,0,0}};
    
    CV_FUNCNAME( "cvTrace" );

    __BEGIN__;

    CvMat stub, *mat = 0;

    if( CV_IS_MAT( array ))
    {
        mat = (CvMat*)array;
        int type = CV_MAT_TYPE(mat->type);
        int size = MIN(mat->rows,mat->cols);
        uchar* data = mat->data.ptr;

        if( type == CV_32FC1 )
        {
            int step = mat->step + sizeof(float);

            for( ; size--; data += step )
                sum.val[0] += *(float*)data;
            EXIT;
        }
        
        if( type == CV_64FC1 )
        {
            int step = mat->step + sizeof(double);

            for( ; size--; data += step )
                sum.val[0] += *(double*)data;
            EXIT;
        }
    }

    CV_CALL( mat = cvGetDiag( array, &stub ));
    CV_CALL( sum = cvSum( mat ));

    __END__;

    return sum;
}


/****************************************************************************************\
*                                     Matrix transpose                                   *
\****************************************************************************************/

/////////////////// macros for inplace transposition of square matrix ////////////////////

#define ICV_DEF_TRANSP_INP_CASE_C1( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    step /= sizeof(arr[0]);         \
                                    \
    while( --len )                  \
    {                               \
        arr += step, arr1++;        \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        do                          \
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
                                    \
            arr2++;                 \
            arr3 += step;           \
        }                           \
        while( arr2 != arr3  );     \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C3( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 3;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=3, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
        }                           \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C4( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 4;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=4, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
            t0 = arr2[3];           \
            t1 = arr3[3];           \
            arr2[3] = t1;           \
            arr3[3] = t0;           \
        }                           \
    }                               \
}


//////////////// macros for non-inplace transposition of rectangular matrix //////////////

#define ICV_DEF_TRANSP_CASE_C1( arrtype )       \
{                                               \
    int x, y;                                   \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( y = 0; y <= size.height - 2; y += 2,   \
                src += 2*srcstep, dst += 2 )    \
    {                                           \
        const arrtype* src1 = src + srcstep;    \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += dststep )       \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1 += dststep;                    \
                                                \
            t0 = src[x + 1];                    \
            t1 = src1[x + 1];                   \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
    }                                           \
                                                \
    if( y < size.height )                       \
    {                                           \
        arrtype* dst1 = dst;                    \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += 2*dststep )     \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            dst1[0] = t0;                       \
            dst1[dststep] = t1;                 \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            dst1[0] = t0;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C3( arrtype )       \
{                                               \
    size.width *= 3;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=3 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 3,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            arrtype t2 = src[x + 2];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1[2] = t2;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C4( arrtype )       \
{                                               \
    size.width *= 4;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=4 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 4,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
                                                \
            t0 = src[x + 2];                    \
            t1 = src[x + 3];                    \
                                                \
            dst1[2] = t0;                       \
            dst1[3] = t1;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_INP_FUNC( flavor, arrtype, cn )      \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( arrtype* arr, int step, CvSize size )\
{                                                           \
    assert( size.width == size.height );                    \
                                                            \
    ICV_DEF_TRANSP_INP_CASE_C##cn( arrtype, size.width )    \
    return CV_OK;                                           \
}


#define ICV_DEF_TRANSP_FUNC( flavor, arrtype, cn )          \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( const arrtype* src, int srcstep,     \
                    arrtype* dst, int dststep, CvSize size )\
{                                                           \
    ICV_DEF_TRANSP_CASE_C##cn( arrtype )                    \
    return CV_OK;                                           \
}


ICV_DEF_TRANSP_INP_FUNC( 8u_C1IR, uchar, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C2IR, ushort, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C3IR, uchar, 3 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C2IR, int, 1 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C3IR, ushort, 3 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C2IR, int64, 1 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C3IR, int, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C2IR, int, 4 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C3IR, int64, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C4IR, int64, 4 )


ICV_DEF_TRANSP_FUNC( 8u_C1R, uchar, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C2R, ushort, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C3R, uchar, 3 )
ICV_DEF_TRANSP_FUNC( 16u_C2R, int, 1 )
ICV_DEF_TRANSP_FUNC( 16u_C3R, ushort, 3 )
ICV_DEF_TRANSP_FUNC( 32s_C2R, int64, 1 )
ICV_DEF_TRANSP_FUNC( 32s_C3R, int, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C2R, int, 4 )
ICV_DEF_TRANSP_FUNC( 64s_C3R, int64, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C4R, int64, 4 )

CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, R )
CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, IR )

CV_IMPL void
cvTranspose( const CvArr* srcarr, CvArr* dstarr )
{
    static CvBtFuncTable tab, inp_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvTranspose" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize size;
    int type, pix_size;

    if( !inittab )
    {
        icvInitTransposeIRTable( &inp_tab );
        icvInitTransposeRTable( &tab );
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    type = CV_MAT_TYPE( src->type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( src );

    if( dstarr == srcarr )
    {
        dst = src; 
    }
    else
    {
        if( !CV_IS_MAT( dst ))
        {
            int coi = 0;
            CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));

            if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
        }

        if( !CV_ARE_TYPES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( size.width != dst->height || size.height != dst->width )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }

    if( src->data.ptr == dst->data.ptr )
    {
        if( size.width == size.height )
        {
            CvFunc2D_1A func = (CvFunc2D_1A)(inp_tab.fn_2d[pix_size]);

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            IPPI_CALL( func( src->data.ptr, src->step, size ));
        }
        else
        {
            if( size.width != 1 && size.height != 1 )
                CV_ERROR( CV_StsBadSize,
                    "Rectangular matrix can not be transposed inplace" );
            
            if( !CV_IS_MAT_CONT( src->type & dst->type ))
                CV_ERROR( CV_StsBadFlag, "In case of inplace column/row transposition "
                                       "both source and destination must be continuous" );

            if( dst == src )
            {
                int t;
                CV_SWAP( dst->width, dst->height, t );
                dst->step = dst->height == 1 ? 0 : pix_size;
            }
        }
    }
    else
    {
        CvFunc2D_2A func = (CvFunc2D_2A)(tab.fn_2d[pix_size]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step,
                         dst->data.ptr, dst->step, size ));
    }

    __END__;
}


/****************************************************************************************\
*                              LU decomposition/back substitution                        *
\****************************************************************************************/

CV_IMPL void
cvCompleteSymm( CvMat* matrix, int LtoR )
{
    CV_FUNCNAME( "cvCompleteSymm" );

    __BEGIN__;
    
    int i, j, nrows;
    
    CV_ASSERT( CV_IS_MAT(matrix) && matrix->rows == matrix->cols );

    nrows = matrix->rows;

    if( CV_MAT_TYPE(matrix->type) == CV_32FC1 || CV_MAT_TYPE(matrix->type) == CV_32SC1 )
    {
        int* data = matrix->data.i;
        int step = matrix->step/sizeof(data[0]);
        int j0 = 0, j1 = nrows;
        for( i = 0; i < nrows; i++ )
        {
            if( !LtoR ) j1 = i; else j0 = i+1;
            for( j = j0; j < j1; j++ )
                data[i*step + j] = data[j*step + i];
        }
    }
    else if( CV_MAT_TYPE(matrix->type) == CV_64FC1 )
    {
        double* data = matrix->data.db;
        int step = matrix->step/sizeof(data[0]);
        int j0 = 0, j1 = nrows;
        for( i = 0; i < nrows; i++ )
        {
            if( !LtoR ) j1 = i; else j0 = i+1;
            for( j = j0; j < j1; j++ )
                data[i*step + j] = data[j*step + i];
        }
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    __END__;
}


/****************************************************************************************\
*                              LU decomposition/back substitution                        *
\****************************************************************************************/

#define arrtype float
#define temptype double

typedef  CvStatus (CV_STDCALL * CvLUDecompFunc)( double* A, int stepA, CvSize sizeA,
                                                 void* B, int stepB, CvSize sizeB,
                                                 double* det );

typedef  CvStatus (CV_STDCALL * CvLUBackFunc)( double* A, int stepA, CvSize sizeA,
                                               void* B, int stepB, CvSize sizeB );


#define ICV_DEF_LU_DECOMP_FUNC( flavor, arrtype )                               \
static CvStatus CV_STDCALL                                                      \
icvLUDecomp_##flavor( double* A, int stepA, CvSize sizeA,                       \
                      arrtype* B, int stepB, CvSize sizeB, double* _det )       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = 0, i;                                                               \
    double det = 1;                                                             \
                                                                                \
    assert( sizeA.width == sizeA.height );                                      \
                                                                                \
    if( B )                                                                     \
    {                                                                           \
        assert( sizeA.height == sizeB.height );                                 \
        m = sizeB.width;                                                        \
    }                                                                           \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    for( i = 0; i < n; i++, A += stepA, B += stepB )                            \
    {                                                                           \
        int j, k = i;                                                           \
        double* tA = A;                                                         \
        arrtype* tB = 0;                                                        \
        double kval = fabs(A[i]), tval;                                         \
                                                                                \
        /* find the pivot element */                                            \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tval = fabs(tA[i]);                                                 \
                                                                                \
            if( tval > kval )                                                   \
            {                                                                   \
                kval = tval;                                                    \
                k = j;                                                          \
            }                                                                   \
        }                                                                       \
                                                                                \
        if( kval == 0 )                                                         \
        {                                                                       \
            det = 0;                                                            \
            break;                                                              \
        }                                                                       \
                                                                                \
        /* swap rows */                                                         \
        if( k != i )                                                            \
        {                                                                       \
            tA = A + stepA*(k - i);                                             \
            det = -det;                                                         \
                                                                                \
            for( j = i; j < n; j++ )                                            \
            {                                                                   \
                double t;                                                       \
                CV_SWAP( A[j], tA[j], t );                                      \
            }                                                                   \
                                                                                \
            if( m > 0 )                                                         \
            {                                                                   \
                tB = B + stepB*(k - i);                                         \
                                                                                \
                for( j = 0; j < m; j++ )                                        \
                {                                                               \
                    arrtype t = B[j];                                           \
                    CV_SWAP( B[j], tB[j], t );                                  \
                }                                                               \
            }                                                                   \
        }                                                                       \
                                                                                \
        tval = 1./A[i];                                                         \
        det *= A[i];                                                            \
        tA = A;                                                                 \
        tB = B;                                                                 \
        A[i] = tval; /* to replace division with multiplication in LUBack */    \
                                                                                \
        /* update matrix and the right side of the system */                    \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tB += stepB;                                                        \
            double alpha = -tA[i]*tval;                                         \
                                                                                \
            for( k = i + 1; k < n; k++ )                                        \
                tA[k] = tA[k] + alpha*A[k];                                     \
                                                                                \
            if( m > 0 )                                                         \
                for( k = 0; k < m; k++ )                                        \
                    tB[k] = (arrtype)(tB[k] + alpha*B[k]);                      \
        }                                                                       \
    }                                                                           \
                                                                                \
    if( _det )                                                                  \
        *_det = det;                                                            \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_DECOMP_FUNC( 32f, float )
ICV_DEF_LU_DECOMP_FUNC( 64f, double )


#define ICV_DEF_LU_BACK_FUNC( flavor, arrtype )                                 \
static CvStatus CV_STDCALL                                                      \
icvLUBack_##flavor( double* A, int stepA, CvSize sizeA,                         \
                    arrtype* B, int stepB, CvSize sizeB )                       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = sizeB.width, i;                                                     \
                                                                                \
    assert( m > 0 && sizeA.width == sizeA.height &&                             \
            sizeA.height == sizeB.height );                                     \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    A += stepA*(n - 1);                                                         \
    B += stepB*(n - 1);                                                         \
                                                                                \
    for( i = n - 1; i >= 0; i--, A -= stepA )                                   \
    {                                                                           \
        int j, k;                                                               \
        for( j = 0; j < m; j++ )                                                \
        {                                                                       \
            arrtype* tB = B + j;                                                \
            double x = 0;                                                       \
                                                                                \
            for( k = n - 1; k > i; k--, tB -= stepB )                           \
                x += A[k]*tB[0];                                                \
                                                                                \
            tB[0] = (arrtype)((tB[0] - x)*A[i]);                                \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_BACK_FUNC( 32f, float )
ICV_DEF_LU_BACK_FUNC( 64f, double )

static CvFuncTable lu_decomp_tab, lu_back_tab;
static int lu_inittab = 0;

static void icvInitLUTable( CvFuncTable* decomp_tab,
                            CvFuncTable* back_tab )
{
    decomp_tab->fn_2d[0] = (void*)icvLUDecomp_32f;
    decomp_tab->fn_2d[1] = (void*)icvLUDecomp_64f;
    back_tab->fn_2d[0] = (void*)icvLUBack_32f;
    back_tab->fn_2d[1] = (void*)icvLUBack_64f;
}



/****************************************************************************************\
*                                 Determinant of the matrix                              *
\****************************************************************************************/

#define det2(m)   (m(0,0)*m(1,1) - m(0,1)*m(1,0))
#define det3(m)   (m(0,0)*(m(1,1)*m(2,2) - m(1,2)*m(2,1)) -  \
                   m(0,1)*(m(1,0)*m(2,2) - m(1,2)*m(2,0)) +  \
                   m(0,2)*(m(1,0)*m(2,1) - m(1,1)*m(2,0)))

CV_IMPL double
cvDet( const CvArr* arr )
{
    double result = 0;
    uchar* buffer = 0;
    int local_alloc = 0;
    
    CV_FUNCNAME( "cvDet" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    int type;

    if( !CV_IS_MAT( mat ))
    {
        CV_CALL( mat = cvGetMat( mat, &stub ));
    }

    type = CV_MAT_TYPE( mat->type );

    if( mat->width != mat->height )
        CV_ERROR( CV_StsBadSize, "The matrix must be square" );

    #define Mf( y, x ) ((float*)(m + y*step))[x]
    #define Md( y, x ) ((double*)(m + y*step))[x]

    if( mat->width == 2 )
    {
        uchar* m = mat->data.ptr;
        int step = mat->step;

        if( type == CV_32FC1 )
        {
            result = det2(Mf);
        }
        else if( type == CV_64FC1 )
        {
            result = det2(Md);
        }
        else
        {
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        }
    }
    else if( mat->width == 3 )
    {
        uchar* m = mat->data.ptr;
        int step = mat->step;
        
        if( type == CV_32FC1 )
        {
            result = det3(Mf);
        }
        else if( type == CV_64FC1 )
        {
            result = det3(Md);
        }
        else
        {
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        }
    }
    else if( mat->width == 1 )
    {
        if( type == CV_32FC1 )
        {
            result = mat->data.fl[0];
        }
        else if( type == CV_64FC1 )
        {
            result = mat->data.db[0];
        }
        else
        {
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        }
    }
    else
    {
        CvLUDecompFunc decomp_func;
        CvSize size = cvGetMatSize( mat );
        const int worktype = CV_64FC1;
        int buf_size = size.width*size.height*CV_ELEM_SIZE(worktype);
        CvMat tmat;

        if( !lu_inittab )
        {
            icvInitLUTable( &lu_decomp_tab, &lu_back_tab );
            lu_inittab = 1;
        }

        if( CV_MAT_CN( type ) != 1 || CV_MAT_DEPTH( type ) < CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( size.width <= CV_MAX_LOCAL_MAT_SIZE )
        {
            buffer = (uchar*)cvStackAlloc( buf_size );
            local_alloc = 1;
        }
        else
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
        }

        CV_CALL( cvInitMatHeader( &tmat, size.height, size.width, worktype, buffer ));
        if( type == worktype )
        {
        	CV_CALL( cvCopy( mat, &tmat ));
        }
        else
            CV_CALL( cvConvert( mat, &tmat ));

        decomp_func = (CvLUDecompFunc)(lu_decomp_tab.fn_2d[CV_MAT_DEPTH(worktype)-CV_32F]);
        assert( decomp_func );

        IPPI_CALL( decomp_func( tmat.data.db, tmat.step, size, 0, 0, size, &result ));
    }

    #undef Mf
    #undef Md

    /*icvCheckVector_64f( &result, 1 );*/

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    return result;
}



/****************************************************************************************\
*                          Inverse (or pseudo-inverse) of the matrix                     *
\****************************************************************************************/

#define Sf( y, x ) ((float*)(srcdata + y*srcstep))[x]
#define Sd( y, x ) ((double*)(srcdata + y*srcstep))[x]
#define Df( y, x ) ((float*)(dstdata + y*dststep))[x]
#define Dd( y, x ) ((double*)(dstdata + y*dststep))[x]

CV_IMPL double
cvInvert( const CvArr* srcarr, CvArr* dstarr, int method )
{
    CvMat* u = 0;
    CvMat* v = 0;
    CvMat* w = 0;

    uchar* buffer = 0;
    int local_alloc = 0;
    double result = 0;
    
    CV_FUNCNAME( "cvInvert" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    int type;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub ));

    if( !CV_IS_MAT( dst ))
        CV_CALL( dst = cvGetMat( dst, &dstub ));

    type = CV_MAT_TYPE( src->type );

    if( method == CV_SVD || method == CV_SVD_SYM )
    {
        int n = MIN(src->rows,src->cols);
        if( method == CV_SVD_SYM && src->rows != src->cols )
            CV_ERROR( CV_StsBadSize, "CV_SVD_SYM method is used for non-square matrix" );

        CV_CALL( u = cvCreateMat( n, src->rows, src->type ));
        if( method != CV_SVD_SYM )
            CV_CALL( v = cvCreateMat( n, src->cols, src->type ));
        CV_CALL( w = cvCreateMat( n, 1, src->type ));
        CV_CALL( cvSVD( src, w, u, v, CV_SVD_U_T + CV_SVD_V_T ));

        if( type == CV_32FC1 )
            result = w->data.fl[0] >= FLT_EPSILON ?
                     w->data.fl[w->rows-1]/w->data.fl[0] : 0;
        else
            result = w->data.db[0] >= FLT_EPSILON ?
                     w->data.db[w->rows-1]/w->data.db[0] : 0;

        CV_CALL( cvSVBkSb( w, u, v ? v : u, 0, dst, CV_SVD_U_T + CV_SVD_V_T ));
        EXIT;
    }
    else if( method != CV_LU )
        CV_ERROR( CV_StsBadArg, "Unknown inversion method" );

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( src->width != src->height )
        CV_ERROR( CV_StsBadSize, "The matrix must be square" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( type != CV_32FC1 && type != CV_64FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( src->width <= 3 )
    {
        uchar* srcdata = src->data.ptr;
        uchar* dstdata = dst->data.ptr;
        int srcstep = src->step;
        int dststep = dst->step;

        if( src->width == 2 )
        {
            if( type == CV_32FC1 )
            {
                double d = det2(Sf);
                if( d != 0. )
                {
                    double t0, t1;
                    result = d;
                    d = 1./d;
                    t0 = Sf(0,0)*d;
                    t1 = Sf(1,1)*d;
                    Df(1,1) = (float)t0;
                    Df(0,0) = (float)t1;
                    t0 = -Sf(0,1)*d;
                    t1 = -Sf(1,0)*d;
                    Df(0,1) = (float)t0;
                    Df(1,0) = (float)t1;
                }
            }
            else
            {
                double d = det2(Sd);
                if( d != 0. )
                {
                    double t0, t1;
                    result = d;
                    d = 1./d;
                    t0 = Sd(0,0)*d;
                    t1 = Sd(1,1)*d;
                    Dd(1,1) = t0;
                    Dd(0,0) = t1;
                    t0 = -Sd(0,1)*d;
                    t1 = -Sd(1,0)*d;
                    Dd(0,1) = t0;
                    Dd(1,0) = t1;
                }
            }
        }
        else if( src->width == 3 )
        {
            if( type == CV_32FC1 )
            {
                double d = det3(Sf);
                if( d != 0. )
                {
                    float t[9];
                    result = d;
                    d = 1./d;

                    t[0] = (float)((Sf(1,1) * Sf(2,2) - Sf(1,2) * Sf(2,1)) * d);
                    t[1] = (float)((Sf(0,2) * Sf(2,1) - Sf(0,1) * Sf(2,2)) * d);
                    t[2] = (float)((Sf(0,1) * Sf(1,2) - Sf(0,2) * Sf(1,1)) * d);
                                  
                    t[3] = (float)((Sf(1,2) * Sf(2,0) - Sf(1,0) * Sf(2,2)) * d);
                    t[4] = (float)((Sf(0,0) * Sf(2,2) - Sf(0,2) * Sf(2,0)) * d);
                    t[5] = (float)((Sf(0,2) * Sf(1,0) - Sf(0,0) * Sf(1,2)) * d);
                                  
                    t[6] = (float)((Sf(1,0) * Sf(2,1) - Sf(1,1) * Sf(2,0)) * d);
                    t[7] = (float)((Sf(0,1) * Sf(2,0) - Sf(0,0) * Sf(2,1)) * d);
                    t[8] = (float)((Sf(0,0) * Sf(1,1) - Sf(0,1) * Sf(1,0)) * d);

                    Df(0,0) = t[0]; Df(0,1) = t[1]; Df(0,2) = t[2];
                    Df(1,0) = t[3]; Df(1,1) = t[4]; Df(1,2) = t[5];
                    Df(2,0) = t[6]; Df(2,1) = t[7]; Df(2,2) = t[8];
                }
            }
            else
            {
                double d = det3(Sd);
                if( d != 0. )
                {
                    double t[9];
                    result = d;
                    d = 1./d;

                    t[0] = (Sd(1,1) * Sd(2,2) - Sd(1,2) * Sd(2,1)) * d;
                    t[1] = (Sd(0,2) * Sd(2,1) - Sd(0,1) * Sd(2,2)) * d;
                    t[2] = (Sd(0,1) * Sd(1,2) - Sd(0,2) * Sd(1,1)) * d;
                           
                    t[3] = (Sd(1,2) * Sd(2,0) - Sd(1,0) * Sd(2,2)) * d;
                    t[4] = (Sd(0,0) * Sd(2,2) - Sd(0,2) * Sd(2,0)) * d;
                    t[5] = (Sd(0,2) * Sd(1,0) - Sd(0,0) * Sd(1,2)) * d;
                           
                    t[6] = (Sd(1,0) * Sd(2,1) - Sd(1,1) * Sd(2,0)) * d;
                    t[7] = (Sd(0,1) * Sd(2,0) - Sd(0,0) * Sd(2,1)) * d;
                    t[8] = (Sd(0,0) * Sd(1,1) - Sd(0,1) * Sd(1,0)) * d;

                    Dd(0,0) = t[0]; Dd(0,1) = t[1]; Dd(0,2) = t[2];
                    Dd(1,0) = t[3]; Dd(1,1) = t[4]; Dd(1,2) = t[5];
                    Dd(2,0) = t[6]; Dd(2,1) = t[7]; Dd(2,2) = t[8];
                }
            }
        }
        else
        {
            assert( src->width == 1 );

            if( type == CV_32FC1 )
            {
                double d = Sf(0,0);
                if( d != 0. )
                {
                    result = d;
                    Df(0,0) = (float)(1./d);
                }
            }
            else
            {
                double d = Sd(0,0);
                if( d != 0. )
                {
                    result = d;
                    Dd(0,0) = 1./d;
                }
            }
        }
    }
    else
    {
        CvLUDecompFunc decomp_func;
        CvLUBackFunc back_func;
        CvSize size = cvGetMatSize( src );
        const int worktype = CV_64FC1;
        int buf_size = size.width*size.height*CV_ELEM_SIZE(worktype);
        CvMat tmat;

        if( !lu_inittab )
        {
            icvInitLUTable( &lu_decomp_tab, &lu_back_tab );
            lu_inittab = 1;
        }

        if( size.width <= CV_MAX_LOCAL_MAT_SIZE )
        {
            buffer = (uchar*)cvStackAlloc( buf_size );
            local_alloc = 1;
        }
        else
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
        }

        CV_CALL( cvInitMatHeader( &tmat, size.height, size.width, worktype, buffer ));
        if( type == worktype )
        {
            CV_CALL( cvCopy( src, &tmat ));
        }
        else
            CV_CALL( cvConvert( src, &tmat ));
        CV_CALL( cvSetIdentity( dst ));

        decomp_func = (CvLUDecompFunc)(lu_decomp_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        back_func = (CvLUBackFunc)(lu_back_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        assert( decomp_func && back_func );

        IPPI_CALL( decomp_func( tmat.data.db, tmat.step, size,
                                dst->data.ptr, dst->step, size, &result ));

        if( result != 0 )
        {
            IPPI_CALL( back_func( tmat.data.db, tmat.step, size,
                                  dst->data.ptr, dst->step, size ));
        }
    }

    if( !result )
        CV_CALL( cvSetZero( dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    if( u || v || w )
    {
        cvReleaseMat( &u );
        cvReleaseMat( &v );
        cvReleaseMat( &w );
    }

    return result;
}


/****************************************************************************************\
*                               Linear system [least-squares] solution                   *
\****************************************************************************************/

static void
icvLSQ( const CvMat* A, const CvMat* B, CvMat* X )
{
    CvMat* AtA = 0;
    CvMat* AtB = 0;
    CvMat* W = 0;
    CvMat* V = 0;

    CV_FUNCNAME( "icvLSQ" );

    __BEGIN__;

    if( !CV_IS_MAT(A) || !CV_IS_MAT(B) || !CV_IS_MAT(X) )
        CV_ERROR( CV_StsBadArg, "Some of required arguments is not a valid matrix" );

    AtA = cvCreateMat( A->cols, A->cols, A->type );
    AtB = cvCreateMat( A->cols, 1, A->type );
    W = cvCreateMat( A->cols, 1, A->type );
    V = cvCreateMat( A->cols, A->cols, A->type );

    cvMulTransposed( A, AtA, 1 );
    cvGEMM( A, B, 1, 0, 0, AtB, CV_GEMM_A_T );
    cvSVD( AtA, W, 0, V, CV_SVD_MODIFY_A + CV_SVD_V_T );
    cvSVBkSb( W, V, V, AtB, X, CV_SVD_U_T + CV_SVD_V_T );

    __END__;

    cvReleaseMat( &AtA );
    cvReleaseMat( &AtB );
    cvReleaseMat( &W );
    cvReleaseMat( &V );
}

CV_IMPL int
cvSolve( const CvArr* A, const CvArr* b, CvArr* x, int method )
{
    CvMat* u = 0;
    CvMat* v = 0;
    CvMat* w = 0;

    uchar* buffer = 0;
    int local_alloc = 0;
    int result = 1;

    CV_FUNCNAME( "cvSolve" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)A;
    CvMat dstub, *dst = (CvMat*)x;
    CvMat bstub, *src2 = (CvMat*)b;
    int type;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub ));

    if( !CV_IS_MAT( src2 ))
        CV_CALL( src2 = cvGetMat( src2, &bstub ));

    if( !CV_IS_MAT( dst ))
        CV_CALL( dst = cvGetMat( dst, &dstub ));

    if( method & CV_LSQ )
    {
        icvLSQ( src, src2, dst );
        EXIT;
    }

    if( method == CV_SVD || method == CV_SVD_SYM )
    {
        int n = MIN(src->rows,src->cols);

        if( method == CV_SVD_SYM && src->rows != src->cols )
            CV_ERROR( CV_StsBadSize, "CV_SVD_SYM method is used for non-square matrix" );

        CV_CALL( u = cvCreateMat( n, src->rows, src->type ));
        if( method != CV_SVD_SYM )
            CV_CALL( v = cvCreateMat( n, src->cols, src->type ));
        CV_CALL( w = cvCreateMat( n, 1, src->type ));
        CV_CALL( cvSVD( src, w, u, v, CV_SVD_U_T + CV_SVD_V_T ));
        CV_CALL( cvSVBkSb( w, u, v ? v : u, src2, dst, CV_SVD_U_T + CV_SVD_V_T ));
        EXIT;
    }
    else if( method != CV_LU )
        CV_ERROR( CV_StsBadArg, "Unknown inversion method" );

    type = CV_MAT_TYPE( src->type );

    if( !CV_ARE_TYPES_EQ( src, dst ) || !CV_ARE_TYPES_EQ( src, src2 ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( src->width != src->height )
        CV_ERROR( CV_StsBadSize, "The matrix must be square" );

    if( !CV_ARE_SIZES_EQ( src2, dst ) || src->width != src2->height )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( type != CV_32FC1 && type != CV_64FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    // check case of a single equation and small matrix
    if( src->width <= 3 && src2->width == 1 )
    {
        #define bf(y) ((float*)(bdata + y*src2step))[0]
        #define bd(y) ((double*)(bdata + y*src2step))[0]

        uchar* srcdata = src->data.ptr;
        uchar* bdata = src2->data.ptr;
        uchar* dstdata = dst->data.ptr;
        int srcstep = src->step;
        int src2step = src2->step;
        int dststep = dst->step;

        if( src->width == 2 )
        {
            if( type == CV_32FC1 )
            {
                double d = det2(Sf);
                if( d != 0. )
                {
                    float t;
                    d = 1./d;
                    t = (float)((bf(0)*Sf(1,1) - bf(1)*Sf(0,1))*d);
                    Df(1,0) = (float)((bf(1)*Sf(0,0) - bf(0)*Sf(1,0))*d);
                    Df(0,0) = t;
                }
                else
                    result = 0;
            }
            else
            {
                double d = det2(Sd);
                if( d != 0. )
                {
                    double t;
                    d = 1./d;
                    t = (bd(0)*Sd(1,1) - bd(1)*Sd(0,1))*d;
                    Dd(1,0) = (bd(1)*Sd(0,0) - bd(0)*Sd(1,0))*d;
                    Dd(0,0) = t;
                }
                else
                    result = 0;
            }
        }
        else if( src->width == 3 )
        {
            if( type == CV_32FC1 )
            {
                double d = det3(Sf);
                if( d != 0. )
                {
                    float t[3];
                    d = 1./d;

                    t[0] = (float)(d*
                           (bf(0)*(Sf(1,1)*Sf(2,2) - Sf(1,2)*Sf(2,1)) -
                            Sf(0,1)*(bf(1)*Sf(2,2) - Sf(1,2)*bf(2)) +
                            Sf(0,2)*(bf(1)*Sf(2,1) - Sf(1,1)*bf(2))));

                    t[1] = (float)(d*
                           (Sf(0,0)*(bf(1)*Sf(2,2) - Sf(1,2)*bf(2)) -
                            bf(0)*(Sf(1,0)*Sf(2,2) - Sf(1,2)*Sf(2,0)) +
                            Sf(0,2)*(Sf(1,0)*bf(2) - bf(1)*Sf(2,0))));

                    t[2] = (float)(d*
                           (Sf(0,0)*(Sf(1,1)*bf(2) - bf(1)*Sf(2,1)) -
                            Sf(0,1)*(Sf(1,0)*bf(2) - bf(1)*Sf(2,0)) +
                            bf(0)*(Sf(1,0)*Sf(2,1) - Sf(1,1)*Sf(2,0))));

                    Df(0,0) = t[0];
                    Df(1,0) = t[1];
                    Df(2,0) = t[2];
                }
                else
                    result = 0;
            }
            else
            {
                double d = det3(Sd);
                if( d != 0. )
                {
                    double t[9];

                    d = 1./d;
                    
                    t[0] = ((Sd(1,1) * Sd(2,2) - Sd(1,2) * Sd(2,1))*bd(0) +
                            (Sd(0,2) * Sd(2,1) - Sd(0,1) * Sd(2,2))*bd(1) +
                            (Sd(0,1) * Sd(1,2) - Sd(0,2) * Sd(1,1))*bd(2))*d;

                    t[1] = ((Sd(1,2) * Sd(2,0) - Sd(1,0) * Sd(2,2))*bd(0) +
                            (Sd(0,0) * Sd(2,2) - Sd(0,2) * Sd(2,0))*bd(1) +
                            (Sd(0,2) * Sd(1,0) - Sd(0,0) * Sd(1,2))*bd(2))*d;

                    t[2] = ((Sd(1,0) * Sd(2,1) - Sd(1,1) * Sd(2,0))*bd(0) +
                            (Sd(0,1) * Sd(2,0) - Sd(0,0) * Sd(2,1))*bd(1) +
                            (Sd(0,0) * Sd(1,1) - Sd(0,1) * Sd(1,0))*bd(2))*d;

                    Dd(0,0) = t[0];
                    Dd(1,0) = t[1];
                    Dd(2,0) = t[2];
                }
                else
                    result = 0;
            }
        }
        else
        {
            assert( src->width == 1 );

            if( type == CV_32FC1 )
            {
                double d = Sf(0,0);
                if( d != 0. )
                    Df(0,0) = (float)(bf(0)/d);
                else
                    result = 0;
            }
            else
            {
                double d = Sd(0,0);
                if( d != 0. )
                    Dd(0,0) = (bd(0)/d);
                else
                    result = 0;
            }
        }
    }
    else
    {
        CvLUDecompFunc decomp_func;
        CvLUBackFunc back_func;
        CvSize size = cvGetMatSize( src );
        CvSize dstsize = cvGetMatSize( dst );
        int worktype = CV_64FC1;
        int buf_size = size.width*size.height*CV_ELEM_SIZE(worktype);
        double d = 0;
        CvMat tmat;

        if( !lu_inittab )
        {
            icvInitLUTable( &lu_decomp_tab, &lu_back_tab );
            lu_inittab = 1;
        }

        if( size.width <= CV_MAX_LOCAL_MAT_SIZE )
        {
            buffer = (uchar*)cvStackAlloc( buf_size );
            local_alloc = 1;
        }
        else
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
        }

        CV_CALL( cvInitMatHeader( &tmat, size.height, size.width, worktype, buffer ));
        if( type == worktype )
        {
            CV_CALL( cvCopy( src, &tmat ));
        }
        else
            CV_CALL( cvConvert( src, &tmat ));

        if( src2->data.ptr != dst->data.ptr )
        {
            CV_CALL( cvCopy( src2, dst ));
        }

        decomp_func = (CvLUDecompFunc)(lu_decomp_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        back_func = (CvLUBackFunc)(lu_back_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        assert( decomp_func && back_func );

        IPPI_CALL( decomp_func( tmat.data.db, tmat.step, size,
                                dst->data.ptr, dst->step, dstsize, &d ));

        if( d != 0 )
        {
            IPPI_CALL( back_func( tmat.data.db, tmat.step, size,
                                  dst->data.ptr, dst->step, dstsize ));
        }
        else
            result = 0;
    }

    if( !result )
        CV_CALL( cvSetZero( dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    if( u || v || w )
    {
        cvReleaseMat( &u );
        cvReleaseMat( &v );
        cvReleaseMat( &w );
    }

    return result;
}



/****************************************************************************************\
*                               3D vector cross-product                                  *
\****************************************************************************************/

CV_IMPL void
cvCrossProduct( const CvArr* srcAarr, const CvArr* srcBarr, CvArr* dstarr )
{
    CV_FUNCNAME( "cvCrossProduct" );

    __BEGIN__;

    CvMat stubA, *srcA = (CvMat*)srcAarr;
    CvMat stubB, *srcB = (CvMat*)srcBarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    int type;

    if( !CV_IS_MAT(srcA))
        CV_CALL( srcA = cvGetMat( srcA, &stubA ));

    type = CV_MAT_TYPE( srcA->type );

    if( srcA->width*srcA->height*CV_MAT_CN(type) != 3 )
        CV_ERROR( CV_StsBadArg, "All the input arrays must be continuous 3-vectors" );

    if( !srcB || !dst )
        CV_ERROR( CV_StsNullPtr, "" );

    if( (srcA->type & ~CV_MAT_CONT_FLAG) == (srcB->type & ~CV_MAT_CONT_FLAG) &&
        (srcA->type & ~CV_MAT_CONT_FLAG) == (dst->type & ~CV_MAT_CONT_FLAG) )
    {
        if( !srcB->data.ptr || !dst->data.ptr )
            CV_ERROR( CV_StsNullPtr, "" );
    }
    else
    {
        if( !CV_IS_MAT(srcB))
            CV_CALL( srcB = cvGetMat( srcB, &stubB ));

        if( !CV_IS_MAT(dst))
            CV_CALL( dst = cvGetMat( dst, &dstub ));

        if( !CV_ARE_TYPES_EQ( srcA, srcB ) ||
            !CV_ARE_TYPES_EQ( srcB, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );
    }

    if( !CV_ARE_SIZES_EQ( srcA, srcB ) || !CV_ARE_SIZES_EQ( srcB, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( CV_MAT_DEPTH(type) == CV_32F )
    {
        float* dstdata = (float*)(dst->data.ptr);
        const float* src1data = (float*)(srcA->data.ptr);
        const float* src2data = (float*)(srcB->data.ptr);

        if( CV_IS_MAT_CONT(srcA->type & srcB->type & dst->type) )
        {
            dstdata[2] = src1data[0] * src2data[1] - src1data[1] * src2data[0];
            dstdata[0] = src1data[1] * src2data[2] - src1data[2] * src2data[1];
            dstdata[1] = src1data[2] * src2data[0] - src1data[0] * src2data[2];
        }
        else
        {
            int step1 = srcA->step ? srcA->step/sizeof(src1data[0]) : 1;
            int step2 = srcB->step ? srcB->step/sizeof(src1data[0]) : 1;
            int step = dst->step ? dst->step/sizeof(src1data[0]) : 1;

            dstdata[2*step] = src1data[0] * src2data[step2] - src1data[step1] * src2data[0];
            dstdata[0] = src1data[step1] * src2data[step2*2] - src1data[step1*2] * src2data[step2];
            dstdata[step] = src1data[step1*2] * src2data[0] - src1data[0] * src2data[step2*2];
        }
    }
    else if( CV_MAT_DEPTH(type) == CV_64F )
    {
        double* dstdata = (double*)(dst->data.ptr);
        const double* src1data = (double*)(srcA->data.ptr);
        const double* src2data = (double*)(srcB->data.ptr);
        
        if( CV_IS_MAT_CONT(srcA->type & srcB->type & dst->type) )
        {
            dstdata[2] = src1data[0] * src2data[1] - src1data[1] * src2data[0];
            dstdata[0] = src1data[1] * src2data[2] - src1data[2] * src2data[1];
            dstdata[1] = src1data[2] * src2data[0] - src1data[0] * src2data[2];
        }
        else
        {
            int step1 = srcA->step ? srcA->step/sizeof(src1data[0]) : 1;
            int step2 = srcB->step ? srcB->step/sizeof(src1data[0]) : 1;
            int step = dst->step ? dst->step/sizeof(src1data[0]) : 1;

            dstdata[2*step] = src1data[0] * src2data[step2] - src1data[step1] * src2data[0];
            dstdata[0] = src1data[step1] * src2data[step2*2] - src1data[step1*2] * src2data[step2];
            dstdata[step] = src1data[step1*2] * src2data[0] - src1data[0] * src2data[step2*2];
        }
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    __END__;
}


CV_IMPL void
cvCalcPCA( const CvArr* data_arr, CvArr* avg_arr, CvArr* eigenvals, CvArr* eigenvects, int flags )
{
    CvMat* tmp_avg = 0;
    CvMat* tmp_avg_r = 0;
    CvMat* tmp_cov = 0;
    CvMat* tmp_evals = 0;
    CvMat* tmp_evects = 0;
    CvMat* tmp_evects2 = 0;
    CvMat* tmp_data = 0;
    
    CV_FUNCNAME( "cvCalcPCA" );

    __BEGIN__;

    CvMat stub, *data = (CvMat*)data_arr;
    CvMat astub, *avg = (CvMat*)avg_arr;
    CvMat evalstub, *evals = (CvMat*)eigenvals;
    CvMat evectstub, *evects = (CvMat*)eigenvects;
    int covar_flags = CV_COVAR_SCALE;
    int i, len, in_count, count, out_count;

    if( !CV_IS_MAT(data) )
        CV_CALL( data = cvGetMat( data, &stub ));

    if( !CV_IS_MAT(avg) )
        CV_CALL( avg = cvGetMat( avg, &astub ));

    if( !CV_IS_MAT(evals) )
        CV_CALL( evals = cvGetMat( evals, &evalstub ));

    if( !CV_IS_MAT(evects) )
        CV_CALL( evects = cvGetMat( evects, &evectstub ));

    if( CV_MAT_CN(data->type) != 1 || CV_MAT_CN(avg->type) != 1 ||
        CV_MAT_CN(evals->type) != 1 || CV_MAT_CN(evects->type) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "All the input and output arrays must be 1-channel" );

    if( CV_MAT_DEPTH(avg->type) < CV_32F || !CV_ARE_DEPTHS_EQ(avg, evals) ||
        !CV_ARE_DEPTHS_EQ(avg, evects) )
        CV_ERROR( CV_StsUnsupportedFormat, "All the output arrays must have the same type, 32fC1 or 64fC1" );

    if( flags & CV_PCA_DATA_AS_COL )
    {
        len = data->rows;
        in_count = data->cols;
        covar_flags |= CV_COVAR_COLS;

        if( avg->cols != 1 || avg->rows != len )
            CV_ERROR( CV_StsBadSize,
            "The mean (average) vector should be data->rows x 1 when CV_PCA_DATA_AS_COL is used" );

        CV_CALL( tmp_avg = cvCreateMat( len, 1, CV_64F ));
    }
    else
    {
        len = data->cols;
        in_count = data->rows;
        covar_flags |= CV_COVAR_ROWS;

        if( avg->rows != 1 || avg->cols != len )
            CV_ERROR( CV_StsBadSize,
            "The mean (average) vector should be 1 x data->cols when CV_PCA_DATA_AS_ROW is used" );

        CV_CALL( tmp_avg = cvCreateMat( 1, len, CV_64F ));
    }

    count = MIN(len, in_count);
    out_count = evals->cols + evals->rows - 1;
    
    if( (evals->cols != 1 && evals->rows != 1) || out_count > count )
        CV_ERROR( CV_StsBadSize,
        "The array of eigenvalues must be 1d vector containing "
        "no more than min(data->rows,data->cols) elements" );

    if( evects->cols != len || evects->rows != out_count )
        CV_ERROR( CV_StsBadSize,
        "The matrix of eigenvalues must have the same number of columns as the input vector length "
        "and the same number of rows as the number of eigenvalues" );

    // "scrambled" way to compute PCA (when cols(A)>rows(A)):
    // B = A'A; B*x=b*x; C = AA'; C*y=c*y -> AA'*y=c*y -> A'A*(A'*y)=c*(A'*y) -> c = b, x=A'*y
    if( len <= in_count )
        covar_flags |= CV_COVAR_NORMAL;

    if( flags & CV_PCA_USE_AVG ){
        covar_flags |= CV_COVAR_USE_AVG;
		CV_CALL( cvConvert( avg, tmp_avg ) );
	}

    CV_CALL( tmp_cov = cvCreateMat( count, count, CV_64F ));
    CV_CALL( tmp_evals = cvCreateMat( 1, count, CV_64F ));
    CV_CALL( tmp_evects = cvCreateMat( count, count, CV_64F ));

    CV_CALL( cvCalcCovarMatrix( &data_arr, 0, tmp_cov, tmp_avg, covar_flags ));
    CV_CALL( cvSVD( tmp_cov, tmp_evals, tmp_evects, 0, CV_SVD_MODIFY_A + CV_SVD_U_T ));
    tmp_evects->rows = out_count;
    tmp_evals->cols = out_count;
    cvZero( evects );
    cvZero( evals );

    if( covar_flags & CV_COVAR_NORMAL )
    {
        CV_CALL( cvConvert( tmp_evects, evects ));
    }
    else
    {
        // CV_PCA_DATA_AS_ROW: cols(A)>rows(A). x=A'*y -> x'=y'*A
        // CV_PCA_DATA_AS_COL: rows(A)>cols(A). x=A''*y -> x'=y'*A'
        int block_count = 0;

        CV_CALL( tmp_data = cvCreateMat( count, count, CV_64F ));
        CV_CALL( tmp_avg_r = cvCreateMat( count, count, CV_64F ));
        CV_CALL( tmp_evects2 = cvCreateMat( out_count, count, CV_64F ));

        for( i = 0; i < len; i += block_count )
        {
            CvMat data_part, tdata_part, part, dst_part, avg_part, tmp_avg_part;
            int gemm_flags;

            block_count = MIN( count, len - i );

            if( flags & CV_PCA_DATA_AS_COL )
            {
                cvGetRows( data, &data_part, i, i + block_count );
                cvGetRows( tmp_data, &tdata_part, 0, block_count );
                cvGetRows( tmp_avg, &avg_part, i, i + block_count );
                cvGetRows( tmp_avg_r, &tmp_avg_part, 0, block_count );
                gemm_flags = CV_GEMM_B_T;
            }
            else
            {
                cvGetCols( data, &data_part, i, i + block_count );
                cvGetCols( tmp_data, &tdata_part, 0, block_count );
                cvGetCols( tmp_avg, &avg_part, i, i + block_count );
                cvGetCols( tmp_avg_r, &tmp_avg_part, 0, block_count );
                gemm_flags = 0;
            }

            cvGetCols( tmp_evects2, &part, 0, block_count );
            cvGetCols( evects, &dst_part, i, i + block_count );

            cvConvert( &data_part, &tdata_part );
            cvRepeat( &avg_part, &tmp_avg_part );
            cvSub( &tdata_part, &tmp_avg_part, &tdata_part );
            cvGEMM( tmp_evects, &tdata_part, 1, 0, 0, &part, gemm_flags );
            cvConvert( &part, &dst_part );
        }

        // normalize eigenvectors
        for( i = 0; i < out_count; i++ )
        {
            CvMat ei;
            cvGetRow( evects, &ei, i );
			cvNormalize( &ei, &ei );
        }
    }

    if( tmp_evals->rows != evals->rows )
        cvReshape( tmp_evals, tmp_evals, 1, evals->rows );
    cvConvert( tmp_evals, evals );
    cvConvert( tmp_avg, avg );

    __END__;

    cvReleaseMat( &tmp_avg );
    cvReleaseMat( &tmp_avg_r );
    cvReleaseMat( &tmp_cov );
    cvReleaseMat( &tmp_evals );
    cvReleaseMat( &tmp_evects );
    cvReleaseMat( &tmp_evects2 );
    cvReleaseMat( &tmp_data );
}


CV_IMPL void
cvProjectPCA( const CvArr* data_arr, const CvArr* avg_arr,
              const CvArr* eigenvects, CvArr* result_arr )
{
    uchar* buffer = 0;
    int local_alloc = 0;
    
    CV_FUNCNAME( "cvProjectPCA" );

    __BEGIN__;

    CvMat stub, *data = (CvMat*)data_arr;
    CvMat astub, *avg = (CvMat*)avg_arr;
    CvMat evectstub, *evects = (CvMat*)eigenvects;
    CvMat rstub, *result = (CvMat*)result_arr;
    CvMat avg_repeated;
    int i, len, in_count;
    int gemm_flags, as_cols, convert_data;
    int block_count0, block_count, buf_size, elem_size;
    uchar* tmp_data_ptr;

    if( !CV_IS_MAT(data) )
        CV_CALL( data = cvGetMat( data, &stub ));

    if( !CV_IS_MAT(avg) )
        CV_CALL( avg = cvGetMat( avg, &astub ));

    if( !CV_IS_MAT(evects) )
        CV_CALL( evects = cvGetMat( evects, &evectstub ));

    if( !CV_IS_MAT(result) )
        CV_CALL( result = cvGetMat( result, &rstub ));

    if( CV_MAT_CN(data->type) != 1 || CV_MAT_CN(avg->type) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "All the input and output arrays must be 1-channel" );

    if( (CV_MAT_TYPE(avg->type) != CV_32FC1 && CV_MAT_TYPE(avg->type) != CV_64FC1) ||
        !CV_ARE_TYPES_EQ(avg, evects) || !CV_ARE_TYPES_EQ(avg, result) )
        CV_ERROR( CV_StsUnsupportedFormat,
        "All the input and output arrays (except for data) must have the same type, 32fC1 or 64fC1" );

    if( (avg->cols != 1 || avg->rows != data->rows) &&
        (avg->rows != 1 || avg->cols != data->cols) )
        CV_ERROR( CV_StsBadSize,
        "The mean (average) vector should be either 1 x data->cols or data->rows x 1" );

    if( avg->cols == 1 )
    {
        len = data->rows;
        in_count = data->cols;

        gemm_flags = CV_GEMM_A_T + CV_GEMM_B_T;
        as_cols = 1;
    }
    else
    {
        len = data->cols;
        in_count = data->rows;

        gemm_flags = CV_GEMM_B_T;
        as_cols = 0;
    }

    if( evects->cols != len )
        CV_ERROR( CV_StsUnmatchedSizes,
        "Eigenvectors must be stored as rows and be of the same size as input vectors" );

    if( result->cols > evects->rows )
        CV_ERROR( CV_StsOutOfRange,
        "The output matrix of coefficients must have the number of columns "
        "less than or equal to the number of eigenvectors (number of rows in eigenvectors matrix)" );

    evects = cvGetRows( evects, &evectstub, 0, result->cols );

    block_count0 = (1 << 16)/len;
    block_count0 = MAX( block_count0, 4 );
    block_count0 = MIN( block_count0, in_count );
    elem_size = CV_ELEM_SIZE(avg->type);
    convert_data = CV_MAT_DEPTH(data->type) < CV_MAT_DEPTH(avg->type);

    buf_size = block_count0*len*((block_count0 > 1) + 1)*elem_size;

    if( buf_size < CV_MAX_LOCAL_SIZE )
    {
        buffer = (uchar*)cvStackAlloc( buf_size );
        local_alloc = 1;
    }
    else
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));

    tmp_data_ptr = buffer;
    if( block_count0 > 1 )
    {
        avg_repeated = cvMat( as_cols ? len : block_count0,
                              as_cols ? block_count0 : len, avg->type, buffer );
        cvRepeat( avg, &avg_repeated );
        tmp_data_ptr += block_count0*len*elem_size;
    }
    else
        avg_repeated = *avg;

    for( i = 0; i < in_count; i += block_count )
    {
        CvMat data_part, norm_data, avg_part, *src = &data_part, out_part;
        
        block_count = MIN( block_count0, in_count - i );
        if( as_cols )
        {
            cvGetCols( data, &data_part, i, i + block_count );
            cvGetCols( &avg_repeated, &avg_part, 0, block_count );
            norm_data = cvMat( len, block_count, avg->type, tmp_data_ptr );
        }
        else
        {
            cvGetRows( data, &data_part, i, i + block_count );
            cvGetRows( &avg_repeated, &avg_part, 0, block_count );
            norm_data = cvMat( block_count, len, avg->type, tmp_data_ptr );
        }

        if( convert_data )
        {
            cvConvert( src, &norm_data );
            src = &norm_data;
        }
        
        cvSub( src, &avg_part, &norm_data );

        cvGetRows( result, &out_part, i, i + block_count );
        cvGEMM( &norm_data, evects, 1, 0, 0, &out_part, gemm_flags );
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


CV_IMPL void
cvBackProjectPCA( const CvArr* proj_arr, const CvArr* avg_arr,
                  const CvArr* eigenvects, CvArr* result_arr )
{
    uchar* buffer = 0;
    int local_alloc = 0;
    
    CV_FUNCNAME( "cvProjectPCA" );

    __BEGIN__;

    CvMat pstub, *data = (CvMat*)proj_arr;
    CvMat astub, *avg = (CvMat*)avg_arr;
    CvMat evectstub, *evects = (CvMat*)eigenvects;
    CvMat rstub, *result = (CvMat*)result_arr;
    CvMat avg_repeated;
    int i, len, in_count, as_cols;
    int block_count0, block_count, buf_size, elem_size;

    if( !CV_IS_MAT(data) )
        CV_CALL( data = cvGetMat( data, &pstub ));

    if( !CV_IS_MAT(avg) )
        CV_CALL( avg = cvGetMat( avg, &astub ));

    if( !CV_IS_MAT(evects) )
        CV_CALL( evects = cvGetMat( evects, &evectstub ));

    if( !CV_IS_MAT(result) )
        CV_CALL( result = cvGetMat( result, &rstub ));

    if( (CV_MAT_TYPE(avg->type) != CV_32FC1 && CV_MAT_TYPE(avg->type) != CV_64FC1) ||
        !CV_ARE_TYPES_EQ(avg, data) || !CV_ARE_TYPES_EQ(avg, evects) || !CV_ARE_TYPES_EQ(avg, result) )
        CV_ERROR( CV_StsUnsupportedFormat,
        "All the input and output arrays must have the same type, 32fC1 or 64fC1" );

    if( (avg->cols != 1 || avg->rows != result->rows) &&
        (avg->rows != 1 || avg->cols != result->cols) )
        CV_ERROR( CV_StsBadSize,
        "The mean (average) vector should be either 1 x result->cols or result->rows x 1" );

    if( avg->cols == 1 )
    {
        len = result->rows;
        in_count = result->cols;
        as_cols = 1;
    }
    else
    {
        len = result->cols;
        in_count = result->rows;
        as_cols = 0;
    }

    if( evects->cols != len )
        CV_ERROR( CV_StsUnmatchedSizes,
        "Eigenvectors must be stored as rows and be of the same size as the output vectors" );

    if( data->cols > evects->rows )
        CV_ERROR( CV_StsOutOfRange,
        "The input matrix of coefficients must have the number of columns "
        "less than or equal to the number of eigenvectors (number of rows in eigenvectors matrix)" );

    evects = cvGetRows( evects, &evectstub, 0, data->cols );

    block_count0 = (1 << 16)/len;
    block_count0 = MAX( block_count0, 4 );
    block_count0 = MIN( block_count0, in_count );
    elem_size = CV_ELEM_SIZE(avg->type);

    buf_size = block_count0*len*(block_count0 > 1)*elem_size;

    if( buf_size < CV_MAX_LOCAL_SIZE )
    {
        buffer = (uchar*)cvStackAlloc( MAX(buf_size,16) );
        local_alloc = 1;
    }
    else
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));

    if( block_count0 > 1 )
    {
        avg_repeated = cvMat( as_cols ? len : block_count0,
                              as_cols ? block_count0 : len, avg->type, buffer );
        cvRepeat( avg, &avg_repeated );
    }
    else
        avg_repeated = *avg;

    for( i = 0; i < in_count; i += block_count )
    {
        CvMat data_part, avg_part, out_part;
        
        block_count = MIN( block_count0, in_count - i );
        cvGetRows( data, &data_part, i, i + block_count );

        if( as_cols )
        {
            cvGetCols( result, &out_part, i, i + block_count );
            cvGetCols( &avg_repeated, &avg_part, 0, block_count );
            cvGEMM( evects, &data_part, 1, &avg_part, 1, &out_part, CV_GEMM_A_T + CV_GEMM_B_T );
        }
        else
        {
            cvGetRows( result, &out_part, i, i + block_count );
            cvGetRows( &avg_repeated, &avg_part, 0, block_count );
            cvGEMM( &data_part, evects, 1, &avg_part, 1, &out_part, 0 );
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


/* End of file. */
