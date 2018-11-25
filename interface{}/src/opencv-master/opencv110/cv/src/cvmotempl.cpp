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

IPCVAPI_IMPL( CvStatus, icvUpdateMotionHistory_8u32f_C1IR,
    (const uchar * silIm, int silStep, float *mhiIm, int mhiStep,
     CvSize size, float timestamp, float mhi_duration),
     (silIm, silStep, mhiIm, mhiStep, size, timestamp, mhi_duration) )
{
    int x, y;

    /* function processes floating-point images using integer arithmetics */
    Cv32suf v;
    int ts, delbound;
    int *mhi = (int *) mhiIm;

    v.f = timestamp;
    ts = v.i;

    if( !silIm || !mhiIm )
        return CV_NULLPTR_ERR;

    if( size.height <= 0 || size.width <= 0 ||
        silStep < size.width || mhiStep < size.width * CV_SIZEOF_FLOAT ||
        (mhiStep & (CV_SIZEOF_FLOAT - 1)) != 0 )
        return CV_BADSIZE_ERR;

    if( mhi_duration < 0 )
        return CV_BADFACTOR_ERR;

    mhi_duration = timestamp - mhi_duration;

    v.f = mhi_duration;
    delbound = CV_TOGGLE_FLT( v.i );

    mhiStep /= sizeof(mhi[0]);

    if( mhiStep == size.width && silStep == size.width )
    {
        size.width *= size.height;
        size.height = 1;
    }

    if( delbound > 0 )
        for( y = 0; y < size.height; y++, silIm += silStep, mhi += mhiStep )
            for( x = 0; x < size.width; x++ )
            {
                int val = mhi[x];

                /* val = silIm[x] ? ts : val < delbound ? 0 : val; */
                val &= (val < delbound) - 1;
                val ^= (ts ^ val) & ((silIm[x] == 0) - 1);
                mhi[x] = val;
            }
    else
        for( y = 0; y < size.height; y++, silIm += silStep, mhi += mhiStep )
            for( x = 0; x < size.width; x++ )
            {
                int val = mhi[x];

                /* val = silIm[x] ? ts : val < delbound ? 0 : val; */
                val &= (CV_TOGGLE_FLT( val ) < delbound) - 1;
                val ^= (ts ^ val) & ((silIm[x] == 0) - 1);
                mhi[x] = val;
            }

    return CV_OK;
}


/* motion templates */
CV_IMPL void
cvUpdateMotionHistory( const void* silhouette, void* mhimg,
                       double timestamp, double mhi_duration )
{
    CvSize size;
    CvMat  silhstub, *silh = (CvMat*)silhouette;
    CvMat  mhistub, *mhi = (CvMat*)mhimg;
    int mhi_step, silh_step;

    CV_FUNCNAME( "cvUpdateMHIByTime" );

    __BEGIN__;

    CV_CALL( silh = cvGetMat( silh, &silhstub ));
    CV_CALL( mhi = cvGetMat( mhi, &mhistub ));

    if( !CV_IS_MASK_ARR( silh ))
        CV_ERROR( CV_StsBadMask, "" );

    if( CV_MAT_CN( mhi->type ) > 1 )
        CV_ERROR( CV_BadNumChannels, "" );

    if( CV_MAT_DEPTH( mhi->type ) != CV_32F )
        CV_ERROR( CV_BadDepth, "" );

    if( !CV_ARE_SIZES_EQ( mhi, silh ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( mhi );

    mhi_step = mhi->step;
    silh_step = silh->step;

    if( CV_IS_MAT_CONT( mhi->type & silh->type ))
    {
        size.width *= size.height;
        mhi_step = silh_step = CV_STUB_STEP;
        size.height = 1;
    }

    IPPI_CALL( icvUpdateMotionHistory_8u32f_C1IR( (const uchar*)(silh->data.ptr), silh_step,
                                                  mhi->data.fl, mhi_step, size,
                                                  (float)timestamp, (float)mhi_duration ));
    __END__;
}


CV_IMPL void
cvCalcMotionGradient( const CvArr* mhiimg, CvArr* maskimg,
                      CvArr* orientation,
                      double delta1, double delta2,
                      int aperture_size )
{
    CvMat *dX_min = 0, *dY_max = 0;
    IplConvKernel* el = 0;

    CV_FUNCNAME( "cvCalcMotionGradient" );

    __BEGIN__;

    CvMat  mhistub, *mhi = (CvMat*)mhiimg;
    CvMat  maskstub, *mask = (CvMat*)maskimg;
    CvMat  orientstub, *orient = (CvMat*)orientation;
    CvMat  dX_min_row, dY_max_row, orient_row, mask_row;
    CvSize size;
    int x, y;

    float  gradient_epsilon = 1e-4f * aperture_size * aperture_size;
    float  min_delta, max_delta;

    CV_CALL( mhi = cvGetMat( mhi, &mhistub ));
    CV_CALL( mask = cvGetMat( mask, &maskstub ));
    CV_CALL( orient = cvGetMat( orient, &orientstub ));

    if( !CV_IS_MASK_ARR( mask ))
        CV_ERROR( CV_StsBadMask, "" );

    if( aperture_size < 3 || aperture_size > 7 || (aperture_size & 1) == 0 )
        CV_ERROR( CV_StsOutOfRange, "aperture_size must be 3, 5 or 7" );

    if( delta1 <= 0 || delta2 <= 0 )
        CV_ERROR( CV_StsOutOfRange, "both delta's must be positive" );

    if( CV_MAT_TYPE( mhi->type ) != CV_32FC1 || CV_MAT_TYPE( orient->type ) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "MHI and orientation must be single-channel floating-point images" );

    if( !CV_ARE_SIZES_EQ( mhi, mask ) || !CV_ARE_SIZES_EQ( orient, mhi ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( orient->data.ptr == mhi->data.ptr )
        CV_ERROR( CV_StsInplaceNotSupported, "orientation image must be different from MHI" );

    if( delta1 > delta2 )
    {
        double t;
        CV_SWAP( delta1, delta2, t );
    }

    size = cvGetMatSize( mhi );
    min_delta = (float)delta1;
    max_delta = (float)delta2;
    CV_CALL( dX_min = cvCreateMat( mhi->rows, mhi->cols, CV_32F ));
    CV_CALL( dY_max = cvCreateMat( mhi->rows, mhi->cols, CV_32F ));

    /* calc Dx and Dy */
    CV_CALL( cvSobel( mhi, dX_min, 1, 0, aperture_size ));
    CV_CALL( cvSobel( mhi, dY_max, 0, 1, aperture_size ));
    cvGetRow( dX_min, &dX_min_row, 0 );
    cvGetRow( dY_max, &dY_max_row, 0 );
    cvGetRow( orient, &orient_row, 0 );
    cvGetRow( mask, &mask_row, 0 );

    /* calc gradient */
    for( y = 0; y < size.height; y++ )
    {
        dX_min_row.data.ptr = dX_min->data.ptr + y*dX_min->step;
        dY_max_row.data.ptr = dY_max->data.ptr + y*dY_max->step;
        orient_row.data.ptr = orient->data.ptr + y*orient->step;
        mask_row.data.ptr = mask->data.ptr + y*mask->step;
        cvCartToPolar( &dX_min_row, &dY_max_row, 0, &orient_row, 1 );

        /* make orientation zero where the gradient is very small */
        for( x = 0; x < size.width; x++ )
        {
            float dY = dY_max_row.data.fl[x];
            float dX = dX_min_row.data.fl[x];

            if( fabs(dX) < gradient_epsilon && fabs(dY) < gradient_epsilon )
            {
                mask_row.data.ptr[x] = 0;
                orient_row.data.i[x] = 0;
            }
            else
                mask_row.data.ptr[x] = 1;
        }
    }

    CV_CALL( el = cvCreateStructuringElementEx( aperture_size, aperture_size,
                            aperture_size/2, aperture_size/2, CV_SHAPE_RECT ));
    cvErode( mhi, dX_min, el );
    cvDilate( mhi, dY_max, el );

    /* mask off pixels which have little motion difference in their neighborhood */
    for( y = 0; y < size.height; y++ )
    {
        dX_min_row.data.ptr = dX_min->data.ptr + y*dX_min->step;
        dY_max_row.data.ptr = dY_max->data.ptr + y*dY_max->step;
        mask_row.data.ptr = mask->data.ptr + y*mask->step;
        orient_row.data.ptr = orient->data.ptr + y*orient->step;
        
        for( x = 0; x < size.width; x++ )
        {
            float d0 = dY_max_row.data.fl[x] - dX_min_row.data.fl[x];

            if( mask_row.data.ptr[x] == 0 || d0 < min_delta || max_delta < d0 )
            {
                mask_row.data.ptr[x] = 0;
                orient_row.data.i[x] = 0;
            }
        }
    }

    __END__;

    cvReleaseMat( &dX_min );
    cvReleaseMat( &dY_max );
    cvReleaseStructuringElement( &el );
}


CV_IMPL double
cvCalcGlobalOrientation( const void* orientation, const void* maskimg, const void* mhiimg,
                         double curr_mhi_timestamp, double mhi_duration )
{
    double  angle = 0;
    int hist_size = 12;
    CvHistogram* hist = 0;

    CV_FUNCNAME( "cvCalcGlobalOrientation" );

    __BEGIN__;

    CvMat  mhistub, *mhi = (CvMat*)mhiimg;
    CvMat  maskstub, *mask = (CvMat*)maskimg;
    CvMat  orientstub, *orient = (CvMat*)orientation;
    void*  _orient;
    float _ranges[] = { 0, 360 };
    float* ranges = _ranges;
    int base_orient;
    double shift_orient = 0, shift_weight = 0, fbase_orient;
    double a, b;
    float delbound;
    CvMat mhi_row, mask_row, orient_row;
    int x, y, mhi_rows, mhi_cols;

    CV_CALL( mhi = cvGetMat( mhi, &mhistub ));
    CV_CALL( mask = cvGetMat( mask, &maskstub ));
    CV_CALL( orient = cvGetMat( orient, &orientstub ));

    if( !CV_IS_MASK_ARR( mask ))
        CV_ERROR( CV_StsBadMask, "" );

    if( CV_MAT_TYPE( mhi->type ) != CV_32FC1 || CV_MAT_TYPE( orient->type ) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "MHI and orientation must be single-channel floating-point images" );

    if( !CV_ARE_SIZES_EQ( mhi, mask ) || !CV_ARE_SIZES_EQ( orient, mhi ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( mhi_duration <= 0 )
        CV_ERROR( CV_StsOutOfRange, "MHI duration must be positive" );

    if( orient->data.ptr == mhi->data.ptr )
        CV_ERROR( CV_StsInplaceNotSupported, "orientation image must be different from MHI" );

    // calculate histogram of different orientation values
    CV_CALL( hist = cvCreateHist( 1, &hist_size, CV_HIST_ARRAY, &ranges ));
    _orient = orient;
    cvCalcArrHist( &_orient, hist, 0, mask );

    // find the maximum index (the dominant orientation)
    cvGetMinMaxHistValue( hist, 0, 0, 0, &base_orient );
    base_orient *= 360/hist_size;

    // override timestamp with the maximum value in MHI
    cvMinMaxLoc( mhi, 0, &curr_mhi_timestamp, 0, 0, mask );

    // find the shift relative to the dominant orientation as weighted sum of relative angles
    a = 254. / 255. / mhi_duration;
    b = 1. - curr_mhi_timestamp * a;
    fbase_orient = base_orient;
    delbound = (float)(curr_mhi_timestamp - mhi_duration);
    mhi_rows = mhi->rows;
    mhi_cols = mhi->cols;

    if( CV_IS_MAT_CONT( mhi->type & mask->type & orient->type ))
    {
        mhi_cols *= mhi_rows;
        mhi_rows = 1;
    }

    cvGetRow( mhi, &mhi_row, 0 );
    cvGetRow( mask, &mask_row, 0 );
    cvGetRow( orient, &orient_row, 0 );

    /*
       a = 254/(255*dt)
       b = 1 - t*a = 1 - 254*t/(255*dur) =
       (255*dt - 254*t)/(255*dt) =
       (dt - (t - dt)*254)/(255*dt);
       --------------------------------------------------------
       ax + b = 254*x/(255*dt) + (dt - (t - dt)*254)/(255*dt) =
       (254*x + dt - (t - dt)*254)/(255*dt) =
       ((x - (t - dt))*254 + dt)/(255*dt) =
       (((x - (t - dt))/dt)*254 + 1)/255 = (((x - low_time)/dt)*254 + 1)/255
     */
    for( y = 0; y < mhi_rows; y++ )
    {
        mhi_row.data.ptr = mhi->data.ptr + mhi->step*y;
        mask_row.data.ptr = mask->data.ptr + mask->step*y;
        orient_row.data.ptr = orient->data.ptr + orient->step*y;

        for( x = 0; x < mhi_cols; x++ )
            if( mask_row.data.ptr[x] != 0 && mhi_row.data.fl[x] > delbound )
            {
                /*
                   orient in 0..360, base_orient in 0..360
                   -> (rel_angle = orient - base_orient) in -360..360.
                   rel_angle is translated to -180..180
                 */
                double weight = mhi_row.data.fl[x] * a + b;
                int rel_angle = cvRound( orient_row.data.fl[x] - fbase_orient );

                rel_angle += (rel_angle < -180 ? 360 : 0);
                rel_angle += (rel_angle > 180 ? -360 : 0);

                if( abs(rel_angle) < 90 )
                {
                    shift_orient += weight * rel_angle;
                    shift_weight += weight;
                }
            }
    }

    // add the dominant orientation and the relative shift
    if( shift_weight == 0 )
        shift_weight = 0.01;

    base_orient = base_orient + cvRound( shift_orient / shift_weight );
    base_orient -= (base_orient < 360 ? 0 : 360);
    base_orient += (base_orient >= 0 ? 0 : 360);

    angle = base_orient;

    __END__;

    cvReleaseHist( &hist );
    return angle;
}


CV_IMPL CvSeq*
cvSegmentMotion( const CvArr* mhiimg, CvArr* segmask, CvMemStorage* storage,
                 double timestamp, double seg_thresh )
{
    CvSeq* components = 0;
    CvMat* mask8u = 0;

    CV_FUNCNAME( "cvSegmentMotion" );

    __BEGIN__;

    CvMat  mhistub, *mhi = (CvMat*)mhiimg;
    CvMat  maskstub, *mask = (CvMat*)segmask;
    Cv32suf v, comp_idx;
    int stub_val, ts;
    int x, y;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "NULL memory storage" );

    CV_CALL( mhi = cvGetMat( mhi, &mhistub ));
    CV_CALL( mask = cvGetMat( mask, &maskstub ));

    if( CV_MAT_TYPE( mhi->type ) != CV_32FC1 || CV_MAT_TYPE( mask->type ) != CV_32FC1 )
        CV_ERROR( CV_BadDepth, "Both MHI and the destination mask" );

    if( !CV_ARE_SIZES_EQ( mhi, mask ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    CV_CALL( mask8u = cvCreateMat( mhi->rows + 2, mhi->cols + 2, CV_8UC1 ));
    cvZero( mask8u );
    cvZero( mask );
    CV_CALL( components = cvCreateSeq( CV_SEQ_KIND_GENERIC, sizeof(CvSeq),
                                       sizeof(CvConnectedComp), storage ));
    
    v.f = (float)timestamp; ts = v.i;
    v.f = FLT_MAX*0.1f; stub_val = v.i;
    comp_idx.f = 1;

    for( y = 0; y < mhi->rows; y++ )
    {
        int* mhi_row = (int*)(mhi->data.ptr + y*mhi->step);
        for( x = 0; x < mhi->cols; x++ )
        {
            if( mhi_row[x] == 0 )
                mhi_row[x] = stub_val;
        }
    }

    for( y = 0; y < mhi->rows; y++ )
    {
        int* mhi_row = (int*)(mhi->data.ptr + y*mhi->step);
        uchar* mask8u_row = mask8u->data.ptr + (y+1)*mask8u->step + 1;

        for( x = 0; x < mhi->cols; x++ )
        {
            if( mhi_row[x] == ts && mask8u_row[x] == 0 )
            {
                CvConnectedComp comp;
                int x1, y1;
                CvScalar _seg_thresh = cvRealScalar(seg_thresh);
                CvPoint seed = cvPoint(x,y);

                CV_CALL( cvFloodFill( mhi, seed, cvRealScalar(0), _seg_thresh, _seg_thresh,
                                      &comp, CV_FLOODFILL_MASK_ONLY + 2*256 + 4, mask8u ));

                for( y1 = 0; y1 < comp.rect.height; y1++ )
                {
                    int* mask_row1 = (int*)(mask->data.ptr +
                                    (comp.rect.y + y1)*mask->step) + comp.rect.x;
                    uchar* mask8u_row1 = mask8u->data.ptr +
                                    (comp.rect.y + y1+1)*mask8u->step + comp.rect.x+1;

                    for( x1 = 0; x1 < comp.rect.width; x1++ )
                    {
                        if( mask8u_row1[x1] > 1 )
                        {
                            mask8u_row1[x1] = 1;
                            mask_row1[x1] = comp_idx.i;
                        }
                    }
                }
                comp_idx.f++;
                cvSeqPush( components, &comp );
            }
        }
    }

    for( y = 0; y < mhi->rows; y++ )
    {
        int* mhi_row = (int*)(mhi->data.ptr + y*mhi->step);
        for( x = 0; x < mhi->cols; x++ )
        {
            if( mhi_row[x] == stub_val )
                mhi_row[x] = 0;
        }
    }

    __END__;

    cvReleaseMat( &mask8u );
    return components;
}

/* End of file. */
