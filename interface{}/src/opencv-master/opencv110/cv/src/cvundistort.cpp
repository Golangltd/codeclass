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

static CvStatus
icvUnDistort_8u_CnR( const uchar* src, int srcstep,
                     uchar* dst, int dststep, CvSize size,
                     const float* intrinsic_matrix,
                     const float* dist_coeffs, int cn )
{
    int u, v, i;
    float u0 = intrinsic_matrix[2], v0 = intrinsic_matrix[5];
    float x0 = (size.width-1)*0.5f, y0 = (size.height-1)*0.5f;
    float fx = intrinsic_matrix[0], fy = intrinsic_matrix[4];
    float ifx = 1.f/fx, ify = 1.f/fy;
    float k1 = dist_coeffs[0], k2 = dist_coeffs[1], k3 = dist_coeffs[4];
    float p1 = dist_coeffs[2], p2 = dist_coeffs[3];

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);

    for( v = 0; v < size.height; v++, dst += dststep )
    {
        float y = (v - v0)*ify, y2 = y*y;

        for( u = 0; u < size.width; u++ )
        {
            float x = (u - u0)*ifx, x2 = x*x, r2 = x2 + y2, _2xy = 2*x*y;
            float kr = 1 + ((k3*r2 + k2)*r2 + k1)*r2;
            float _x = fx*(x*kr + p1*_2xy + p2*(r2 + 2*x2)) + x0;
            float _y = fy*(y*kr + p1*(r2 + 2*y2) + p2*_2xy) + y0;
            int ix = cvFloor(_x), iy = cvFloor(_y);

            if( (unsigned)iy < (unsigned)(size.height - 1) &&
                (unsigned)ix < (unsigned)(size.width - 1) )
            {
                const uchar* ptr = src + iy*srcstep + ix*cn;
                _x -= ix; _y -= iy;
                for( i = 0; i < cn; i++ )
                {
                    float t0 = CV_8TO32F(ptr[i]), t1 = CV_8TO32F(ptr[i+srcstep]);
                    t0 += _x*(CV_8TO32F(ptr[i+cn]) - t0);
                    t1 += _x*(CV_8TO32F(ptr[i + srcstep + cn]) - t1);
                    dst[u*cn + i] = (uchar)cvRound(t0 + _y*(t1 - t0));
                }
            }
            else
            {
                for( i = 0; i < cn; i++ )
                    dst[u*cn + i] = 0;
            }
            
        }
    }

    return CV_OK;
}


icvUndistortGetSize_t icvUndistortGetSize_p = 0;
icvCreateMapCameraUndistort_32f_C1R_t icvCreateMapCameraUndistort_32f_C1R_p = 0;
icvUndistortRadial_8u_C1R_t icvUndistortRadial_8u_C1R_p = 0;
icvUndistortRadial_8u_C3R_t icvUndistortRadial_8u_C3R_p = 0;

typedef CvStatus (CV_STDCALL * CvUndistortRadialIPPFunc)
    ( const void* pSrc, int srcStep, void* pDst, int dstStep, CvSize roiSize,
      float fx, float fy, float cx, float cy, float k1, float k2, uchar *pBuffer );

CV_IMPL void
cvUndistort2( const CvArr* _src, CvArr* _dst, const CvMat* A, const CvMat* dist_coeffs )
{
    static int inittab = 0;

    CV_FUNCNAME( "cvUndistort2" );

    __BEGIN__;

    float a[9], k[5]={0,0,0,0,0};
    int coi1 = 0, coi2 = 0;
    CvMat srcstub, *src = (CvMat*)_src;
    CvMat dststub, *dst = (CvMat*)_dst;
    CvMat _a = cvMat( 3, 3, CV_32F, a ), _k;
    int cn, src_step, dst_step;
    CvSize size;

    if( !inittab )
    {
        icvInitLinearCoeffTab();
        icvInitCubicCoeffTab();
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "The function does not support COI" );

    if( CV_MAT_DEPTH(src->type) != CV_8U )
        CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

    if( src->data.ptr == dst->data.ptr )
        CV_ERROR( CV_StsNotImplemented, "In-place undistortion is not implemented" );

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( !CV_IS_MAT(A) || A->rows != 3 || A->cols != 3  ||
        (CV_MAT_TYPE(A->type) != CV_32FC1 && CV_MAT_TYPE(A->type) != CV_64FC1) )
        CV_ERROR( CV_StsBadArg, "Intrinsic matrix must be a valid 3x3 floating-point matrix" );

    if( !CV_IS_MAT(dist_coeffs) || (dist_coeffs->rows != 1 && dist_coeffs->cols != 1) ||
        (dist_coeffs->rows*dist_coeffs->cols*CV_MAT_CN(dist_coeffs->type) != 4 &&
        dist_coeffs->rows*dist_coeffs->cols*CV_MAT_CN(dist_coeffs->type) != 5) ||
        (CV_MAT_DEPTH(dist_coeffs->type) != CV_64F &&
        CV_MAT_DEPTH(dist_coeffs->type) != CV_32F) )
        CV_ERROR( CV_StsBadArg,
            "Distortion coefficients must be 1x4, 4x1, 1x5 or 5x1 floating-point vector" );

    cvConvert( A, &_a );
    _k = cvMat( dist_coeffs->rows, dist_coeffs->cols,
                CV_MAKETYPE(CV_32F, CV_MAT_CN(dist_coeffs->type)), k );
    cvConvert( dist_coeffs, &_k );

    cn = CV_MAT_CN(src->type);
    size = cvGetMatSize(src);
    src_step = src->step ? src->step : CV_STUB_STEP;
    dst_step = dst->step ? dst->step : CV_STUB_STEP;

    icvUnDistort_8u_CnR( src->data.ptr, src_step,
        dst->data.ptr, dst_step, size, a, k, cn );

    __END__;
}


CV_IMPL void
cvInitUndistortMap( const CvMat* A, const CvMat* dist_coeffs,
                    CvArr* mapxarr, CvArr* mapyarr )
{
    CV_FUNCNAME( "cvInitUndistortMap" );

    __BEGIN__;
    
    float a[9], k[5]={0,0,0,0,0};
    int coi1 = 0, coi2 = 0;
    CvMat mapxstub, *_mapx = (CvMat*)mapxarr;
    CvMat mapystub, *_mapy = (CvMat*)mapyarr;
    CvMat _a = cvMat( 3, 3, CV_32F, a ), _k;
    int u, v;
    float u0, v0, fx, fy, ifx, ify, x0, y0, k1, k2, k3, p1, p2;
    CvSize size;

    CV_CALL( _mapx = cvGetMat( _mapx, &mapxstub, &coi1 ));
    CV_CALL( _mapy = cvGetMat( _mapy, &mapystub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "The function does not support COI" );

    if( CV_MAT_TYPE(_mapx->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Both maps must have 32fC1 type" );

    if( !CV_ARE_TYPES_EQ( _mapx, _mapy ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( _mapx, _mapy ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize(_mapx);

    if( !CV_IS_MAT(A) || A->rows != 3 || A->cols != 3  ||
        (CV_MAT_TYPE(A->type) != CV_32FC1 && CV_MAT_TYPE(A->type) != CV_64FC1) )
        CV_ERROR( CV_StsBadArg, "Intrinsic matrix must be a valid 3x3 floating-point matrix" );

    if( !CV_IS_MAT(dist_coeffs) || (dist_coeffs->rows != 1 && dist_coeffs->cols != 1) ||
        (dist_coeffs->rows*dist_coeffs->cols*CV_MAT_CN(dist_coeffs->type) != 4 &&
        dist_coeffs->rows*dist_coeffs->cols*CV_MAT_CN(dist_coeffs->type) != 5) ||
        (CV_MAT_DEPTH(dist_coeffs->type) != CV_64F &&
        CV_MAT_DEPTH(dist_coeffs->type) != CV_32F) )
        CV_ERROR( CV_StsBadArg,
            "Distortion coefficients must be 1x4, 4x1, 1x5 or 5x1 floating-point vector" );

    cvConvert( A, &_a );
    _k = cvMat( dist_coeffs->rows, dist_coeffs->cols,
                CV_MAKETYPE(CV_32F, CV_MAT_CN(dist_coeffs->type)), k );
    cvConvert( dist_coeffs, &_k );

    u0 = a[2]; v0 = a[5];
    fx = a[0]; fy = a[4];
    ifx = 1.f/fx; ify = 1.f/fy;
    k1 = k[0]; k2 = k[1]; k3 = k[4];
    p1 = k[2]; p2 = k[3];
    x0 = (size.width-1)*0.5f;
    y0 = (size.height-1)*0.5f;

    for( v = 0; v < size.height; v++ )
    {
        float* mapx = (float*)(_mapx->data.ptr + _mapx->step*v);
        float* mapy = (float*)(_mapy->data.ptr + _mapy->step*v);
        float y = (v - v0)*ify, y2 = y*y;

        for( u = 0; u < size.width; u++ )
        {
            float x = (u - u0)*ifx, x2 = x*x, r2 = x2 + y2, _2xy = 2*x*y;
            double kr = 1 + ((k3*r2 + k2)*r2 + k1)*r2;
            double _x = fx*(x*kr + p1*_2xy + p2*(r2 + 2*x2)) + x0;
            double _y = fy*(y*kr + p1*(r2 + 2*y2) + p2*_2xy) + y0; 
            mapx[u] = (float)_x;
            mapy[u] = (float)_y;
        }
    }

    __END__;
}


void
cvInitUndistortRectifyMap( const CvMat* A, const CvMat* distCoeffs,
    const CvMat *R, const CvMat* Ar, CvArr* mapxarr, CvArr* mapyarr )
{
    CV_FUNCNAME( "cvInitUndistortMap" );

    __BEGIN__;
    
    double a[9], ar[9], r[9], ir[9], k[5]={0,0,0,0,0};
    int coi1 = 0, coi2 = 0;
    CvMat mapxstub, *_mapx = (CvMat*)mapxarr;
    CvMat mapystub, *_mapy = (CvMat*)mapyarr;
    CvMat _a = cvMat( 3, 3, CV_64F, a );
    CvMat _k = cvMat( 4, 1, CV_64F, k );
    CvMat _ar = cvMat( 3, 3, CV_64F, ar );
    CvMat _r = cvMat( 3, 3, CV_64F, r );
    CvMat _ir = cvMat( 3, 3, CV_64F, ir );
    int i, j;
    double fx, fy, u0, v0, k1, k2, k3, p1, p2;
    CvSize size;

    CV_CALL( _mapx = cvGetMat( _mapx, &mapxstub, &coi1 ));
    CV_CALL( _mapy = cvGetMat( _mapy, &mapystub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "The function does not support COI" );

    if( CV_MAT_TYPE(_mapx->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Both maps must have 32fC1 type" );

    if( !CV_ARE_TYPES_EQ( _mapx, _mapy ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( _mapx, _mapy ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( A )
    {
        if( !CV_IS_MAT(A) || A->rows != 3 || A->cols != 3  ||
            (CV_MAT_TYPE(A->type) != CV_32FC1 && CV_MAT_TYPE(A->type) != CV_64FC1) )
            CV_ERROR( CV_StsBadArg, "Intrinsic matrix must be a valid 3x3 floating-point matrix" );
        cvConvert( A, &_a );
    }
    else
        cvSetIdentity( &_a );

    if( Ar )
    {
        CvMat Ar33;
        if( !CV_IS_MAT(Ar) || Ar->rows != 3 || (Ar->cols != 3 && Ar->cols != 4) ||
            (CV_MAT_TYPE(Ar->type) != CV_32FC1 && CV_MAT_TYPE(Ar->type) != CV_64FC1) )
            CV_ERROR( CV_StsBadArg, "The new intrinsic matrix must be a valid 3x3 floating-point matrix" );
        cvGetCols( Ar, &Ar33, 0, 3 );
        cvConvert( &Ar33, &_ar );
    }
    else
        cvSetIdentity( &_ar );

    if( !CV_IS_MAT(R) || R->rows != 3 || R->cols != 3  ||
        (CV_MAT_TYPE(R->type) != CV_32FC1 && CV_MAT_TYPE(R->type) != CV_64FC1) )
        CV_ERROR( CV_StsBadArg, "Rotaion/homography matrix must be a valid 3x3 floating-point matrix" );

    if( distCoeffs )
    {
        CV_ASSERT( CV_IS_MAT(distCoeffs) &&
            (distCoeffs->rows == 1 || distCoeffs->cols == 1) &&
            (distCoeffs->rows*distCoeffs->cols*CV_MAT_CN(distCoeffs->type) == 4 ||
            distCoeffs->rows*distCoeffs->cols*CV_MAT_CN(distCoeffs->type) == 5) &&
            (CV_MAT_DEPTH(distCoeffs->type) == CV_64F ||
            CV_MAT_DEPTH(distCoeffs->type) == CV_32F) );
        _k = cvMat( distCoeffs->rows, distCoeffs->cols,
                CV_MAKETYPE(CV_64F, CV_MAT_CN(distCoeffs->type)), k );
        cvConvert( distCoeffs, &_k );
    }
    else
        cvZero( &_k );
    
    cvConvert( R, &_r );    // rectification matrix
    cvMatMul( &_ar, &_r, &_r ); // Ar*R
    cvInvert( &_r, &_ir );  // inverse: R^-1*Ar^-1

    u0 = a[2]; v0 = a[5];
    fx = a[0]; fy = a[4];
    k1 = k[0]; k2 = k[1]; k3 = k[4];
    p1 = k[2]; p2 = k[3];

    size = cvGetMatSize(_mapx);

    for( i = 0; i < size.height; i++ )
    {
        float* mapx = (float*)(_mapx->data.ptr + _mapx->step*i);
        float* mapy = (float*)(_mapy->data.ptr + _mapy->step*i);
        double _x = i*ir[1] + ir[2], _y = i*ir[4] + ir[5], _w = i*ir[7] + ir[8];

        for( j = 0; j < size.width; j++, _x += ir[0], _y += ir[3], _w += ir[6] )
        {
            double w = 1./_w, x = _x*w, y = _y*w;
            double x2 = x*x, y2 = y*y;
            double r2 = x2 + y2, _2xy = 2*x*y;
            double kr = 1 + ((k3*r2 + k2)*r2 + k1)*r2;
            double u = fx*(x*kr + p1*_2xy + p2*(r2 + 2*x2)) + u0;
            double v = fy*(y*kr + p1*(r2 + 2*y2) + p2*_2xy) + v0; 
            mapx[j] = (float)u;
            mapy[j] = (float)v;
        }
    }

    __END__;
}


void
cvUndistortPoints( const CvMat* _src, CvMat* _dst, const CvMat* _cameraMatrix,
                   const CvMat* _distCoeffs,
                   const CvMat* _R, const CvMat* _P )
{
    CV_FUNCNAME( "cvUndistortPoints" );

    __BEGIN__;

    double A[3][3], RR[3][3], k[5]={0,0,0,0,0}, fx, fy, ifx, ify, cx, cy;
    CvMat _A=cvMat(3, 3, CV_64F, A), _Dk;
    CvMat _RR=cvMat(3, 3, CV_64F, RR);
    const CvPoint2D32f* srcf;
    const CvPoint2D64f* srcd;
    CvPoint2D32f* dstf;
    CvPoint2D64f* dstd;
    int stype, dtype;
    int sstep, dstep;
    int i, j, n;

    CV_ASSERT( CV_IS_MAT(_src) && CV_IS_MAT(_dst) &&
        (_src->rows == 1 || _src->cols == 1) &&
        (_dst->rows == 1 || _dst->cols == 1) &&
        CV_ARE_SIZES_EQ(_src, _dst) &&
        (CV_MAT_TYPE(_src->type) == CV_32FC2 || CV_MAT_TYPE(_src->type) == CV_64FC2) &&
        (CV_MAT_TYPE(_dst->type) == CV_32FC2 || CV_MAT_TYPE(_dst->type) == CV_64FC2));

    CV_ASSERT( CV_IS_MAT(_cameraMatrix) && CV_IS_MAT(_distCoeffs) &&
        _cameraMatrix->rows == 3 && _cameraMatrix->cols == 3 &&
        (_distCoeffs->rows == 1 || _distCoeffs->cols == 1) &&
        (_distCoeffs->rows*_distCoeffs->cols == 4 ||
        _distCoeffs->rows*_distCoeffs->cols == 5) );
    _Dk = cvMat( _distCoeffs->rows, _distCoeffs->cols,
        CV_MAKETYPE(CV_64F,CV_MAT_CN(_distCoeffs->type)), k);
    cvConvert( _cameraMatrix, &_A );
    cvConvert( _distCoeffs, &_Dk );

    if( _R )
    {
        CV_ASSERT( CV_IS_MAT(_R) && _R->rows == 3 && _R->cols == 3 );
        cvConvert( _R, &_RR );
    }
    else
        cvSetIdentity(&_RR);

    if( _P )
    {
        double PP[3][3];
        CvMat _P3x3, _PP=cvMat(3, 3, CV_64F, PP);
        CV_ASSERT( CV_IS_MAT(_P) && _P->rows == 3 && (_P->cols == 3 || _P->cols == 4));
        cvConvert( cvGetCols(_P, &_P3x3, 0, 3), &_PP );
        cvMatMul( &_PP, &_RR, &_RR );
    }
    
    srcf = (const CvPoint2D32f*)_src->data.ptr;
    srcd = (const CvPoint2D64f*)_src->data.ptr;
    dstf = (CvPoint2D32f*)_dst->data.ptr;
    dstd = (CvPoint2D64f*)_dst->data.ptr;
    stype = CV_MAT_TYPE(_src->type);
    dtype = CV_MAT_TYPE(_dst->type);
    sstep = _src->rows == 1 ? 1 : _src->step/CV_ELEM_SIZE(stype);
    dstep = _dst->rows == 1 ? 1 : _dst->step/CV_ELEM_SIZE(dtype);
    
    n = _src->rows + _src->cols - 1;

    fx = A[0][0];
    fy = A[1][1];
    ifx = 1./fx;
    ify = 1./fy;
    cx = A[0][2];
    cy = A[1][2];

    for( i = 0; i < n; i++ )
    {
        double x, y, x0, y0;
        if( stype == CV_32FC2 )
        {
            x = srcf[i*sstep].x;
            y = srcf[i*sstep].y;
        }
        else
        {
            x = srcd[i*sstep].x;
            y = srcd[i*sstep].y;
        }
        
        x0 = x = (x - cx)*ifx;
        y0 = y = (y - cy)*ify;

        // compensate distortion iteratively
        for( j = 0; j < 5; j++ )
        {
            double r2 = x*x + y*y;
            double icdist = 1./(1 + ((k[4]*r2 + k[1])*r2 + k[0])*r2);
            double deltaX = 2*k[2]*x*y + k[3]*(r2 + 2*x*x);
            double deltaY = k[2]*(r2 + 2*y*y) + 2*k[3]*x*y;
            x = (x0 - deltaX)*icdist;
            y = (y0 - deltaY)*icdist;
        }
        
        double xx = RR[0][0]*x + RR[0][1]*y + RR[0][2];
        double yy = RR[1][0]*x + RR[1][1]*y + RR[1][2];
        double ww = 1./(RR[2][0]*x + RR[2][1]*y + RR[2][2]);
        x = xx*ww;
        y = yy*ww;

        if( dtype == CV_32FC2 )
        {
            dstf[i*dstep].x = (float)x;
            dstf[i*dstep].y = (float)y;
        }
        else
        {
            dstd[i*dstep].x = x;
            dstd[i*dstep].y = y;
        }
    }

    __END__;
}

/*  End of file  */
