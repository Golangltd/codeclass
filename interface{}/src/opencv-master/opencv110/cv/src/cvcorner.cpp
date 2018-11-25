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
#include <stdio.h>


static void
icvCalcMinEigenVal( const float* cov, int cov_step, float* dst,
                    int dst_step, CvSize size, CvMat* buffer )
{
    int j;
    float* buf = buffer->data.fl;
    cov_step /= sizeof(cov[0]);
    dst_step /= sizeof(dst[0]);
    buffer->rows = 1;

    for( ; size.height--; cov += cov_step, dst += dst_step )
    {
        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3]*0.5;
            double b = cov[j*3+1];
            double c = cov[j*3+2]*0.5;
            
            buf[j + size.width] = (float)(a + c);
            buf[j] = (float)((a - c)*(a - c) + b*b);
        }

        cvPow( buffer, buffer, 0.5 );

        for( j = 0; j < size.width ; j++ )
            dst[j] = (float)(buf[j + size.width] - buf[j]);
    }
}


static void
icvCalcHarris( const float* cov, int cov_step, float* dst,
               int dst_step, CvSize size, CvMat* /*buffer*/, double k )
{
    int j;
    cov_step /= sizeof(cov[0]);
    dst_step /= sizeof(dst[0]);

    for( ; size.height--; cov += cov_step, dst += dst_step )
    {
        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3];
            double b = cov[j*3+1];
            double c = cov[j*3+2];
            dst[j] = (float)(a*c - b*b - k*(a + c)*(a + c));
        }
    }
}


static void
icvCalcEigenValsVecs( const float* cov, int cov_step, float* dst,
                      int dst_step, CvSize size, CvMat* buffer )
{
    static int y0 = 0;
    
    int j;
    float* buf = buffer->data.fl;
    cov_step /= sizeof(cov[0]);
    dst_step /= sizeof(dst[0]);

    for( ; size.height--; cov += cov_step, dst += dst_step )
    {
        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3]*0.5;
            double b = cov[j*3+1];
            double c = cov[j*3+2]*0.5;

            buf[j + size.width] = (float)(a + c);
            buf[j] = (float)((a - c)*(a - c) + b*b);
        }

        buffer->rows = 1;
        cvPow( buffer, buffer, 0.5 );

        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3];
            double b = cov[j*3+1];
            double c = cov[j*3+2];
            
            double l1 = buf[j + size.width] + buf[j];
            double l2 = buf[j + size.width] - buf[j];

            double x = b;
            double y = l1 - a;
            double e = fabs(x);

            if( e + fabs(y) < 1e-4 )
            {
                y = b;
                x = l1 - c;
                e = fabs(x);
                if( e + fabs(y) < 1e-4 )
                {
                    e = 1./(e + fabs(y) + FLT_EPSILON);
                    x *= e, y *= e;
                }
            }

            buf[j] = (float)(x*x + y*y + DBL_EPSILON);
            dst[6*j] = (float)l1;
            dst[6*j + 2] = (float)x;
            dst[6*j + 3] = (float)y;

            x = b;
            y = l2 - a;
            e = fabs(x);

            if( e + fabs(y) < 1e-4 )
            {
                y = b;
                x = l2 - c;
                e = fabs(x);
                if( e + fabs(y) < 1e-4 )
                {
                    e = 1./(e + fabs(y) + FLT_EPSILON);
                    x *= e, y *= e;
                }
            }

            buf[j + size.width] = (float)(x*x + y*y + DBL_EPSILON);
            dst[6*j + 1] = (float)l2;
            dst[6*j + 4] = (float)x;
            dst[6*j + 5] = (float)y;
        }

        buffer->rows = 2;
        cvPow( buffer, buffer, -0.5 );

        for( j = 0; j < size.width; j++ )
        {
            double t0 = buf[j]*dst[6*j + 2];
            double t1 = buf[j]*dst[6*j + 3];

            dst[6*j + 2] = (float)t0;
            dst[6*j + 3] = (float)t1;

            t0 = buf[j + size.width]*dst[6*j + 4];
            t1 = buf[j + size.width]*dst[6*j + 5];

            dst[6*j + 4] = (float)t0;
            dst[6*j + 5] = (float)t1;
        }

        y0++;
    }
}


#define ICV_MINEIGENVAL     0
#define ICV_HARRIS          1
#define ICV_EIGENVALSVECS   2

static void
icvCornerEigenValsVecs( const CvMat* src, CvMat* eigenv, int block_size,
                        int aperture_size, int op_type, double k=0. )
{
    CvSepFilter dx_filter, dy_filter;
    CvBoxFilter blur_filter;
    CvMat *tempsrc = 0;
    CvMat *Dx = 0, *Dy = 0, *cov = 0;
    CvMat *sqrt_buf = 0;

    int buf_size = 1 << 12;
    
    CV_FUNCNAME( "icvCornerEigenValsVecs" );

    __BEGIN__;

    int i, j, y, dst_y = 0, max_dy, delta = 0;
    int aperture_size0 = aperture_size;
    int temp_step = 0, d_step;
    uchar* shifted_ptr = 0;
    int depth, d_depth;
    int stage = CV_START;
    CvSobelFixedIPPFunc ipp_sobel_vert = 0, ipp_sobel_horiz = 0;
    CvFilterFixedIPPFunc ipp_scharr_vert = 0, ipp_scharr_horiz = 0;
    CvSize el_size, size, stripe_size;
    int aligned_width;
    CvPoint el_anchor;
    double factorx, factory;
    bool use_ipp = false;

    if( block_size < 3 || !(block_size & 1) )
        CV_ERROR( CV_StsOutOfRange, "averaging window size must be an odd number >= 3" );

    if( (aperture_size < 3 && aperture_size != CV_SCHARR) || !(aperture_size & 1) )
        CV_ERROR( CV_StsOutOfRange,
        "Derivative filter aperture size must be a positive odd number >=3 or CV_SCHARR" );

    depth = CV_MAT_DEPTH(src->type);
    d_depth = depth == CV_8U ? CV_16S : CV_32F;

    size = cvGetMatSize(src);
    aligned_width = cvAlign(size.width, 4);

    aperture_size = aperture_size == CV_SCHARR ? 3 : aperture_size;
    el_size = cvSize( aperture_size, aperture_size );
    el_anchor = cvPoint( aperture_size/2, aperture_size/2 );

    if( aperture_size <= 5 && icvFilterSobelVert_8u16s_C1R_p )
    {
        if( depth == CV_8U && aperture_size0 == CV_SCHARR )
        {
            ipp_scharr_vert = icvFilterScharrVert_8u16s_C1R_p;
            ipp_scharr_horiz = icvFilterScharrHoriz_8u16s_C1R_p;
        }
        else if( depth == CV_32F && aperture_size0 == CV_SCHARR )
        {
            ipp_scharr_vert = icvFilterScharrVert_32f_C1R_p;
            ipp_scharr_horiz = icvFilterScharrHoriz_32f_C1R_p;
        }
        else if( depth == CV_8U )
        {
            ipp_sobel_vert = icvFilterSobelVert_8u16s_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_8u16s_C1R_p;
        }
        else if( depth == CV_32F )
        {
            ipp_sobel_vert = icvFilterSobelVert_32f_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_32f_C1R_p;
        }
    }
    
    if( (ipp_sobel_vert && ipp_sobel_horiz) ||
        (ipp_scharr_vert && ipp_scharr_horiz) )
    {
        CV_CALL( tempsrc = icvIPPFilterInit( src, buf_size,
            cvSize(el_size.width,el_size.height + block_size)));
        shifted_ptr = tempsrc->data.ptr + el_anchor.y*tempsrc->step +
                      el_anchor.x*CV_ELEM_SIZE(depth);
        temp_step = tempsrc->step ? tempsrc->step : CV_STUB_STEP;
        max_dy = tempsrc->rows - aperture_size + 1;
        use_ipp = true;
    }
    else
    {
        ipp_sobel_vert = ipp_sobel_horiz = 0;
        ipp_scharr_vert = ipp_scharr_horiz = 0;

        CV_CALL( dx_filter.init_deriv( size.width, depth, d_depth, 1, 0, aperture_size0 ));
        CV_CALL( dy_filter.init_deriv( size.width, depth, d_depth, 0, 1, aperture_size0 ));
        max_dy = buf_size / src->cols;
        max_dy = MAX( max_dy, aperture_size + block_size );
    }

    CV_CALL( Dx = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( Dy = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( cov = cvCreateMat( max_dy + block_size + 1, size.width, CV_32FC3 ));
    CV_CALL( sqrt_buf = cvCreateMat( 2, size.width, CV_32F ));
    Dx->cols = Dy->cols = size.width;

    if( !use_ipp )
        max_dy -= aperture_size - 1;
    d_step = Dx->step ? Dx->step : CV_STUB_STEP;

    CV_CALL(blur_filter.init(size.width, CV_32FC3, CV_32FC3, 0, cvSize(block_size,block_size)));
    stripe_size = size;

    factorx = (double)(1 << (aperture_size - 1)) * block_size;
    if( aperture_size0 == CV_SCHARR )
        factorx *= 2;
    if( depth == CV_8U )
        factorx *= 255.;
    factory = factorx = 1./factorx;
    if( ipp_sobel_vert )
        factory = -factory;

    for( y = 0; y < size.height; y += delta )
    {
        if( !use_ipp )
        {
            delta = MIN( size.height - y, max_dy );
            if( y + delta == size.height )
                stage = stage & CV_START ? CV_START + CV_END : CV_END;
            dx_filter.process( src, Dx, cvRect(0,y,-1,delta), cvPoint(0,0), stage );
            stripe_size.height = dy_filter.process( src, Dy, cvRect(0,y,-1,delta),
                                                    cvPoint(0,0), stage );
        }
        else
        {
            delta = icvIPPFilterNextStripe( src, tempsrc, y, el_size, el_anchor );
            stripe_size.height = delta;

            if( ipp_sobel_vert )
            {
                IPPI_CALL( ipp_sobel_vert( shifted_ptr, temp_step,
                        Dx->data.ptr, d_step, stripe_size,
                        aperture_size*10 + aperture_size ));
                IPPI_CALL( ipp_sobel_horiz( shifted_ptr, temp_step,
                        Dy->data.ptr, d_step, stripe_size,
                        aperture_size*10 + aperture_size ));
            }
            else /*if( ipp_scharr_vert )*/
            {
                IPPI_CALL( ipp_scharr_vert( shifted_ptr, temp_step,
                        Dx->data.ptr, d_step, stripe_size ));
                IPPI_CALL( ipp_scharr_horiz( shifted_ptr, temp_step,
                        Dy->data.ptr, d_step, stripe_size ));
            }
        }

        for( i = 0; i < stripe_size.height; i++ )
        {
            float* cov_data = (float*)(cov->data.ptr + i*cov->step);
            if( d_depth == CV_16S )
            {
                const short* dxdata = (const short*)(Dx->data.ptr + i*Dx->step);
                const short* dydata = (const short*)(Dy->data.ptr + i*Dy->step);

                for( j = 0; j < size.width; j++ )
                {
                    double dx = dxdata[j]*factorx;
                    double dy = dydata[j]*factory;

                    cov_data[j*3] = (float)(dx*dx);
                    cov_data[j*3+1] = (float)(dx*dy);
                    cov_data[j*3+2] = (float)(dy*dy);
                }
            }
            else
            {
                const float* dxdata = (const float*)(Dx->data.ptr + i*Dx->step);
                const float* dydata = (const float*)(Dy->data.ptr + i*Dy->step);

                for( j = 0; j < size.width; j++ )
                {
                    double dx = dxdata[j]*factorx;
                    double dy = dydata[j]*factory;

                    cov_data[j*3] = (float)(dx*dx);
                    cov_data[j*3+1] = (float)(dx*dy);
                    cov_data[j*3+2] = (float)(dy*dy);
                }
            }
        }

        if( y + stripe_size.height >= size.height )
            stage = stage & CV_START ? CV_START + CV_END : CV_END;

        stripe_size.height = blur_filter.process(cov,cov,
            cvRect(0,0,-1,stripe_size.height),cvPoint(0,0),stage+CV_ISOLATED_ROI);

        if( op_type == ICV_MINEIGENVAL )
            icvCalcMinEigenVal( cov->data.fl, cov->step,
                (float*)(eigenv->data.ptr + dst_y*eigenv->step), eigenv->step,
                stripe_size, sqrt_buf );
        else if( op_type == ICV_HARRIS )
            icvCalcHarris( cov->data.fl, cov->step,
                (float*)(eigenv->data.ptr + dst_y*eigenv->step), eigenv->step,
                stripe_size, sqrt_buf, k );
        else if( op_type == ICV_EIGENVALSVECS )
            icvCalcEigenValsVecs( cov->data.fl, cov->step,
                (float*)(eigenv->data.ptr + dst_y*eigenv->step), eigenv->step,
                stripe_size, sqrt_buf );

        dst_y += stripe_size.height;
        stage = CV_MIDDLE;
    }

    __END__;

    cvReleaseMat( &Dx );
    cvReleaseMat( &Dy );
    cvReleaseMat( &cov );
    cvReleaseMat( &sqrt_buf );
    cvReleaseMat( &tempsrc );
}


CV_IMPL void
cvCornerMinEigenVal( const void* srcarr, void* eigenvarr,
                     int block_size, int aperture_size )
{
    CV_FUNCNAME( "cvCornerMinEigenVal" );

    __BEGIN__;

    CvMat stub, *src = (CvMat*)srcarr;
    CvMat eigstub, *eigenv = (CvMat*)eigenvarr;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( eigenv = cvGetMat( eigenv, &eigstub ));

    if( (CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1) ||
        CV_MAT_TYPE(eigenv->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    if( !CV_ARE_SIZES_EQ( src, eigenv ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    CV_CALL( icvCornerEigenValsVecs( src, eigenv, block_size, aperture_size, ICV_MINEIGENVAL ));

    __END__;
}


CV_IMPL void
cvCornerHarris( const CvArr* srcarr, CvArr* harris_responce,
                int block_size, int aperture_size, double k )
{
    CV_FUNCNAME( "cvCornerHarris" );

    __BEGIN__;

    CvMat stub, *src = (CvMat*)srcarr;
    CvMat eigstub, *eigenv = (CvMat*)harris_responce;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( eigenv = cvGetMat( eigenv, &eigstub ));

    if( (CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1) ||
        CV_MAT_TYPE(eigenv->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    if( !CV_ARE_SIZES_EQ( src, eigenv ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    CV_CALL( icvCornerEigenValsVecs( src, eigenv, block_size, aperture_size, ICV_HARRIS, k ));

    __END__;
}


CV_IMPL void
cvCornerEigenValsAndVecs( const void* srcarr, void* eigenvarr,
                          int block_size, int aperture_size )
{
    CV_FUNCNAME( "cvCornerEigenValsAndVecs" );

    __BEGIN__;

    CvMat stub, *src = (CvMat*)srcarr;
    CvMat eigstub, *eigenv = (CvMat*)eigenvarr;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( eigenv = cvGetMat( eigenv, &eigstub ));

    if( CV_MAT_CN(eigenv->type)*eigenv->cols != src->cols*6 ||
        eigenv->rows != src->rows )
        CV_ERROR( CV_StsUnmatchedSizes, "Output array should be 6 times "
            "wider than the input array and they should have the same height");

    if( (CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1) ||
        CV_MAT_TYPE(eigenv->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    CV_CALL( icvCornerEigenValsVecs( src, eigenv, block_size, aperture_size, ICV_EIGENVALSVECS ));

    __END__;
}


CV_IMPL void
cvPreCornerDetect( const void* srcarr, void* dstarr, int aperture_size )
{
    CvSepFilter dx_filter, dy_filter, d2x_filter, d2y_filter, dxy_filter;
    CvMat *Dx = 0, *Dy = 0, *D2x = 0, *D2y = 0, *Dxy = 0;
    CvMat *tempsrc = 0;
    
    int buf_size = 1 << 12;

    CV_FUNCNAME( "cvPreCornerDetect" );

    __BEGIN__;

    int i, j, y, dst_y = 0, max_dy, delta = 0;
    int temp_step = 0, d_step;
    uchar* shifted_ptr = 0;
    int depth, d_depth;
    int stage = CV_START;
    CvSobelFixedIPPFunc ipp_sobel_vert = 0, ipp_sobel_horiz = 0,
                        ipp_sobel_vert_second = 0, ipp_sobel_horiz_second = 0,
                        ipp_sobel_cross = 0;
    CvSize el_size, size, stripe_size;
    int aligned_width;
    CvPoint el_anchor;
    double factor;
    CvMat stub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    bool use_ipp = false;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( (CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1) ||
        CV_MAT_TYPE(dst->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( aperture_size == CV_SCHARR )
        CV_ERROR( CV_StsOutOfRange, "CV_SCHARR is not supported by this function" );

    if( aperture_size < 3 || aperture_size > 7 || !(aperture_size & 1) )
        CV_ERROR( CV_StsOutOfRange,
        "Derivative filter aperture size must be 3, 5 or 7" );

    depth = CV_MAT_DEPTH(src->type);
    d_depth = depth == CV_8U ? CV_16S : CV_32F;

    size = cvGetMatSize(src);
    aligned_width = cvAlign(size.width, 4);

    el_size = cvSize( aperture_size, aperture_size );
    el_anchor = cvPoint( aperture_size/2, aperture_size/2 );

    if( aperture_size <= 5 && icvFilterSobelVert_8u16s_C1R_p )
    {
        if( depth == CV_8U )
        {
            ipp_sobel_vert = icvFilterSobelVert_8u16s_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_8u16s_C1R_p;
            ipp_sobel_vert_second = icvFilterSobelVertSecond_8u16s_C1R_p;
            ipp_sobel_horiz_second = icvFilterSobelHorizSecond_8u16s_C1R_p;
            ipp_sobel_cross = icvFilterSobelCross_8u16s_C1R_p;
        }
        else if( depth == CV_32F )
        {
            ipp_sobel_vert = icvFilterSobelVert_32f_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_32f_C1R_p;
            ipp_sobel_vert_second = icvFilterSobelVertSecond_32f_C1R_p;
            ipp_sobel_horiz_second = icvFilterSobelHorizSecond_32f_C1R_p;
            ipp_sobel_cross = icvFilterSobelCross_32f_C1R_p;
        }
    }

    if( ipp_sobel_vert && ipp_sobel_horiz && ipp_sobel_vert_second &&
        ipp_sobel_horiz_second && ipp_sobel_cross )
    {
        CV_CALL( tempsrc = icvIPPFilterInit( src, buf_size, el_size ));
        shifted_ptr = tempsrc->data.ptr + el_anchor.y*tempsrc->step +
                      el_anchor.x*CV_ELEM_SIZE(depth);
        temp_step = tempsrc->step ? tempsrc->step : CV_STUB_STEP;
        max_dy = tempsrc->rows - aperture_size + 1;
        use_ipp = true;
    }
    else
    {
        ipp_sobel_vert = ipp_sobel_horiz = 0;
        ipp_sobel_vert_second = ipp_sobel_horiz_second = ipp_sobel_cross = 0;
        dx_filter.init_deriv( size.width, depth, d_depth, 1, 0, aperture_size );
        dy_filter.init_deriv( size.width, depth, d_depth, 0, 1, aperture_size );
        d2x_filter.init_deriv( size.width, depth, d_depth, 2, 0, aperture_size );
        d2y_filter.init_deriv( size.width, depth, d_depth, 0, 2, aperture_size );
        dxy_filter.init_deriv( size.width, depth, d_depth, 1, 1, aperture_size );
        max_dy = buf_size / src->cols;
        max_dy = MAX( max_dy, aperture_size );
    }

    CV_CALL( Dx = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( Dy = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( D2x = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( D2y = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( Dxy = cvCreateMat( max_dy, aligned_width, d_depth ));
    Dx->cols = Dy->cols = D2x->cols = D2y->cols = Dxy->cols = size.width;

    if( !use_ipp )
        max_dy -= aperture_size - 1;
    d_step = Dx->step ? Dx->step : CV_STUB_STEP;

    stripe_size = size;

    factor = 1 << (aperture_size - 1);
    if( depth == CV_8U )
        factor *= 255;
    factor = 1./(factor * factor * factor);

    aperture_size = aperture_size * 10 + aperture_size;

    for( y = 0; y < size.height; y += delta )
    {
        if( !use_ipp )
        {
            delta = MIN( size.height - y, max_dy );
            CvRect roi = cvRect(0,y,size.width,delta);
            CvPoint origin=cvPoint(0,0);

            if( y + delta == size.height )
                stage = stage & CV_START ? CV_START + CV_END : CV_END;
            
            dx_filter.process(src,Dx,roi,origin,stage);
            dy_filter.process(src,Dy,roi,origin,stage);
            d2x_filter.process(src,D2x,roi,origin,stage);
            d2y_filter.process(src,D2y,roi,origin,stage);
            stripe_size.height = dxy_filter.process(src,Dxy,roi,origin,stage);
        }
        else
        {
            delta = icvIPPFilterNextStripe( src, tempsrc, y, el_size, el_anchor );
            stripe_size.height = delta;

            IPPI_CALL( ipp_sobel_vert( shifted_ptr, temp_step,
                Dx->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_horiz( shifted_ptr, temp_step,
                Dy->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_vert_second( shifted_ptr, temp_step,
                D2x->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_horiz_second( shifted_ptr, temp_step,
                D2y->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_cross( shifted_ptr, temp_step,
                Dxy->data.ptr, d_step, stripe_size, aperture_size ));
        }

        for( i = 0; i < stripe_size.height; i++, dst_y++ )
        {
            float* dstdata = (float*)(dst->data.ptr + dst_y*dst->step);
            
            if( d_depth == CV_16S )
            {
                const short* dxdata = (const short*)(Dx->data.ptr + i*Dx->step);
                const short* dydata = (const short*)(Dy->data.ptr + i*Dy->step);
                const short* d2xdata = (const short*)(D2x->data.ptr + i*D2x->step);
                const short* d2ydata = (const short*)(D2y->data.ptr + i*D2y->step);
                const short* dxydata = (const short*)(Dxy->data.ptr + i*Dxy->step);
                
                for( j = 0; j < stripe_size.width; j++ )
                {
                    double dx = dxdata[j];
                    double dx2 = dx * dx;
                    double dy = dydata[j];
                    double dy2 = dy * dy;

                    dstdata[j] = (float)(factor*(dx2*d2ydata[j] + dy2*d2xdata[j] - 2*dx*dy*dxydata[j]));
                }
            }
            else
            {
                const float* dxdata = (const float*)(Dx->data.ptr + i*Dx->step);
                const float* dydata = (const float*)(Dy->data.ptr + i*Dy->step);
                const float* d2xdata = (const float*)(D2x->data.ptr + i*D2x->step);
                const float* d2ydata = (const float*)(D2y->data.ptr + i*D2y->step);
                const float* dxydata = (const float*)(Dxy->data.ptr + i*Dxy->step);
                
                for( j = 0; j < stripe_size.width; j++ )
                {
                    double dx = dxdata[j];
                    double dy = dydata[j];
                    dstdata[j] = (float)(factor*(dx*dx*d2ydata[j] + dy*dy*d2xdata[j] - 2*dx*dy*dxydata[j]));
                }
            }
        }

        stage = CV_MIDDLE;
    }

    __END__;

    cvReleaseMat( &Dx );
    cvReleaseMat( &Dy );
    cvReleaseMat( &D2x );
    cvReleaseMat( &D2y );
    cvReleaseMat( &Dxy );
    cvReleaseMat( &tempsrc );
}

/* End of file */
