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
#include <float.h>
#include <stdio.h>

static void
intersect( CvPoint2D32f pt, CvSize win_size, CvSize imgSize,
           CvPoint* min_pt, CvPoint* max_pt )
{
    CvPoint ipt;

    ipt.x = cvFloor( pt.x );
    ipt.y = cvFloor( pt.y );

    ipt.x -= win_size.width;
    ipt.y -= win_size.height;

    win_size.width = win_size.width * 2 + 1;
    win_size.height = win_size.height * 2 + 1;

    min_pt->x = MAX( 0, -ipt.x );
    min_pt->y = MAX( 0, -ipt.y );
    max_pt->x = MIN( win_size.width, imgSize.width - ipt.x );
    max_pt->y = MIN( win_size.height, imgSize.height - ipt.y );
}


static int icvMinimalPyramidSize( CvSize imgSize )
{
    return cvAlign(imgSize.width,8) * imgSize.height / 3;
}


static void
icvInitPyramidalAlgorithm( const CvMat* imgA, const CvMat* imgB,
                           CvMat* pyrA, CvMat* pyrB,
                           int level, CvTermCriteria * criteria,
                           int max_iters, int flags,
                           uchar *** imgI, uchar *** imgJ,
                           int **step, CvSize** size,
                           double **scale, uchar ** buffer )
{
    CV_FUNCNAME( "icvInitPyramidalAlgorithm" );

    __BEGIN__;

    const int ALIGN = 8;
    int pyrBytes, bufferBytes = 0, elem_size;
    int level1 = level + 1;

    int i;
    CvSize imgSize, levelSize;

    *buffer = 0;
    *imgI = *imgJ = 0;
    *step = 0;
    *scale = 0;
    *size = 0;

    /* check input arguments */
    if( ((flags & CV_LKFLOW_PYR_A_READY) != 0 && !pyrA) ||
        ((flags & CV_LKFLOW_PYR_B_READY) != 0 && !pyrB) )
        CV_ERROR( CV_StsNullPtr, "Some of the precomputed pyramids are missing" );

    if( level < 0 )
        CV_ERROR( CV_StsOutOfRange, "The number of pyramid layers is negative" );

    switch( criteria->type )
    {
    case CV_TERMCRIT_ITER:
        criteria->epsilon = 0.f;
        break;
    case CV_TERMCRIT_EPS:
        criteria->max_iter = max_iters;
        break;
    case CV_TERMCRIT_ITER | CV_TERMCRIT_EPS:
        break;
    default:
        assert( 0 );
        CV_ERROR( CV_StsBadArg, "Invalid termination criteria" );
    }

    /* compare squared values */
    criteria->epsilon *= criteria->epsilon;

    /* set pointers and step for every level */
    pyrBytes = 0;

    imgSize = cvGetSize(imgA);
    elem_size = CV_ELEM_SIZE(imgA->type);
    levelSize = imgSize;

    for( i = 1; i < level1; i++ )
    {
        levelSize.width = (levelSize.width + 1) >> 1;
        levelSize.height = (levelSize.height + 1) >> 1;

        int tstep = cvAlign(levelSize.width,ALIGN) * elem_size;
        pyrBytes += tstep * levelSize.height;
    }

    assert( pyrBytes <= imgSize.width * imgSize.height * elem_size * 4 / 3 );

    /* buffer_size = <size for patches> + <size for pyramids> */
    bufferBytes = (int)((level1 >= 0) * ((pyrA->data.ptr == 0) +
        (pyrB->data.ptr == 0)) * pyrBytes +
        (sizeof(imgI[0][0]) * 2 + sizeof(step[0][0]) +
         sizeof(size[0][0]) + sizeof(scale[0][0])) * level1);

    CV_CALL( *buffer = (uchar *)cvAlloc( bufferBytes ));

    *imgI = (uchar **) buffer[0];
    *imgJ = *imgI + level1;
    *step = (int *) (*imgJ + level1);
    *scale = (double *) (*step + level1);
    *size = (CvSize *)(*scale + level1);

    imgI[0][0] = imgA->data.ptr;
    imgJ[0][0] = imgB->data.ptr;
    step[0][0] = imgA->step;
    scale[0][0] = 1;
    size[0][0] = imgSize;

    if( level > 0 )
    {
        uchar *bufPtr = (uchar *) (*size + level1);
        uchar *ptrA = pyrA->data.ptr;
        uchar *ptrB = pyrB->data.ptr;

        if( !ptrA )
        {
            ptrA = bufPtr;
            bufPtr += pyrBytes;
        }

        if( !ptrB )
            ptrB = bufPtr;

        levelSize = imgSize;

        /* build pyramids for both frames */
        for( i = 1; i <= level; i++ )
        {
            int levelBytes;
            CvMat prev_level, next_level;

            levelSize.width = (levelSize.width + 1) >> 1;
            levelSize.height = (levelSize.height + 1) >> 1;

            size[0][i] = levelSize;
            step[0][i] = cvAlign( levelSize.width, ALIGN ) * elem_size;
            scale[0][i] = scale[0][i - 1] * 0.5;

            levelBytes = step[0][i] * levelSize.height;
            imgI[0][i] = (uchar *) ptrA;
            ptrA += levelBytes;

            if( !(flags & CV_LKFLOW_PYR_A_READY) )
            {
                prev_level = cvMat( size[0][i-1].height, size[0][i-1].width, CV_8UC1 );
                next_level = cvMat( size[0][i].height, size[0][i].width, CV_8UC1 );
                cvSetData( &prev_level, imgI[0][i-1], step[0][i-1] );
                cvSetData( &next_level, imgI[0][i], step[0][i] );
                cvPyrDown( &prev_level, &next_level );
            }

            imgJ[0][i] = (uchar *) ptrB;
            ptrB += levelBytes;

            if( !(flags & CV_LKFLOW_PYR_B_READY) )
            {
                prev_level = cvMat( size[0][i-1].height, size[0][i-1].width, CV_8UC1 );
                next_level = cvMat( size[0][i].height, size[0][i].width, CV_8UC1 );
                cvSetData( &prev_level, imgJ[0][i-1], step[0][i-1] );
                cvSetData( &next_level, imgJ[0][i], step[0][i] );
                cvPyrDown( &prev_level, &next_level );
            }
        }
    }

    __END__;
}


/* compute dI/dx and dI/dy */
static void
icvCalcIxIy_32f( const float* src, int src_step, float* dstX, float* dstY, int dst_step,
                 CvSize src_size, const float* smooth_k, float* buffer0 )
{
    int src_width = src_size.width, dst_width = src_size.width-2;
    int x, height = src_size.height - 2;
    float* buffer1 = buffer0 + src_width;

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dstX[0]);

    for( ; height--; src += src_step, dstX += dst_step, dstY += dst_step )
    {
        const float* src2 = src + src_step;
        const float* src3 = src + src_step*2;

        for( x = 0; x < src_width; x++ )
        {
            float t0 = (src3[x] + src[x])*smooth_k[0] + src2[x]*smooth_k[1];
            float t1 = src3[x] - src[x];
            buffer0[x] = t0; buffer1[x] = t1;
        }

        for( x = 0; x < dst_width; x++ )
        {
            float t0 = buffer0[x+2] - buffer0[x];
            float t1 = (buffer1[x] + buffer1[x+2])*smooth_k[0] + buffer1[x+1]*smooth_k[1];
            dstX[x] = t0; dstY[x] = t1;
        }
    }
}


icvOpticalFlowPyrLKInitAlloc_8u_C1R_t icvOpticalFlowPyrLKInitAlloc_8u_C1R_p = 0;
icvOpticalFlowPyrLKFree_8u_C1R_t icvOpticalFlowPyrLKFree_8u_C1R_p = 0;
icvOpticalFlowPyrLK_8u_C1R_t icvOpticalFlowPyrLK_8u_C1R_p = 0;


CV_IMPL void
cvCalcOpticalFlowPyrLK( const void* arrA, const void* arrB,
                        void* pyrarrA, void* pyrarrB,
                        const CvPoint2D32f * featuresA,
                        CvPoint2D32f * featuresB,
                        int count, CvSize winSize, int level,
                        char *status, float *error,
                        CvTermCriteria criteria, int flags )
{
    uchar *pyrBuffer = 0;
    uchar *buffer = 0;
    float* _error = 0;
    char* _status = 0;

    void* ipp_optflow_state = 0;
    
    CV_FUNCNAME( "cvCalcOpticalFlowPyrLK" );

    __BEGIN__;

    const int MAX_ITERS = 100;

    CvMat stubA, *imgA = (CvMat*)arrA;
    CvMat stubB, *imgB = (CvMat*)arrB;
    CvMat pstubA, *pyrA = (CvMat*)pyrarrA;
    CvMat pstubB, *pyrB = (CvMat*)pyrarrB;
    CvSize imgSize;
    static const float smoothKernel[] = { 0.09375, 0.3125, 0.09375 };  /* 3/32, 10/32, 3/32 */
    
    int bufferBytes = 0;
    uchar **imgI = 0;
    uchar **imgJ = 0;
    int *step = 0;
    double *scale = 0;
    CvSize* size = 0;

    int threadCount = cvGetNumThreads();
    float* _patchI[CV_MAX_THREADS];
    float* _patchJ[CV_MAX_THREADS];
    float* _Ix[CV_MAX_THREADS];
    float* _Iy[CV_MAX_THREADS];

    int i, l;

    CvSize patchSize = cvSize( winSize.width * 2 + 1, winSize.height * 2 + 1 );
    int patchLen = patchSize.width * patchSize.height;
    int srcPatchLen = (patchSize.width + 2)*(patchSize.height + 2);

    CV_CALL( imgA = cvGetMat( imgA, &stubA ));
    CV_CALL( imgB = cvGetMat( imgB, &stubB ));

    if( CV_MAT_TYPE( imgA->type ) != CV_8UC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( !CV_ARE_TYPES_EQ( imgA, imgB ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( imgA, imgB ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( imgA->step != imgB->step )
        CV_ERROR( CV_StsUnmatchedSizes, "imgA and imgB must have equal steps" );

    imgSize = cvGetMatSize( imgA );

    if( pyrA )
    {
        CV_CALL( pyrA = cvGetMat( pyrA, &pstubA ));

        if( pyrA->step*pyrA->height < icvMinimalPyramidSize( imgSize ) )
            CV_ERROR( CV_StsBadArg, "pyramid A has insufficient size" );
    }
    else
    {
        pyrA = &pstubA;
        pyrA->data.ptr = 0;
    }

    if( pyrB )
    {
        CV_CALL( pyrB = cvGetMat( pyrB, &pstubB ));

        if( pyrB->step*pyrB->height < icvMinimalPyramidSize( imgSize ) )
            CV_ERROR( CV_StsBadArg, "pyramid B has insufficient size" );
    }
    else
    {
        pyrB = &pstubB;
        pyrB->data.ptr = 0;
    }

    if( count == 0 )
        EXIT;

    if( !featuresA || !featuresB )
        CV_ERROR( CV_StsNullPtr, "Some of arrays of point coordinates are missing" );

    if( count < 0 )
        CV_ERROR( CV_StsOutOfRange, "The number of tracked points is negative or zero" );

    if( winSize.width <= 1 || winSize.height <= 1 )
        CV_ERROR( CV_StsBadSize, "Invalid search window size" );

    for( i = 0; i < threadCount; i++ )
        _patchI[i] = _patchJ[i] = _Ix[i] = _Iy[i] = 0;

    CV_CALL( icvInitPyramidalAlgorithm( imgA, imgB, pyrA, pyrB,
        level, &criteria, MAX_ITERS, flags,
        &imgI, &imgJ, &step, &size, &scale, &pyrBuffer ));

    if( !status )
        CV_CALL( status = _status = (char*)cvAlloc( count*sizeof(_status[0]) ));

#if 0
    if( icvOpticalFlowPyrLKInitAlloc_8u_C1R_p &&
        icvOpticalFlowPyrLKFree_8u_C1R_p &&
        icvOpticalFlowPyrLK_8u_C1R_p &&
        winSize.width == winSize.height &&
        icvOpticalFlowPyrLKInitAlloc_8u_C1R_p( &ipp_optflow_state, imgSize,
                                               winSize.width*2+1, cvAlgHintAccurate ) >= 0 )
    {
        CvPyramid ipp_pyrA, ipp_pyrB;
        static const double rate[] = { 1, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625, 0.0078125,
                                       0.00390625, 0.001953125, 0.0009765625, 0.00048828125, 0.000244140625,
                                       0.0001220703125 };
        // initialize pyramid structures
        assert( level < 14 );
        ipp_pyrA.ptr = imgI;
        ipp_pyrB.ptr = imgJ;
        ipp_pyrA.sz = ipp_pyrB.sz = size;
        ipp_pyrA.rate = ipp_pyrB.rate = (double*)rate;
        ipp_pyrA.step = ipp_pyrB.step = step;
        ipp_pyrA.state = ipp_pyrB.state = 0;
        ipp_pyrA.level = ipp_pyrB.level = level;

        if( !error )
            CV_CALL( error = _error = (float*)cvAlloc( count*sizeof(_error[0]) ));

        for( i = 0; i < count; i++ )
            featuresB[i] = featuresA[i];

        if( icvOpticalFlowPyrLK_8u_C1R_p( &ipp_pyrA, &ipp_pyrB,
            (const float*)featuresA, (float*)featuresB, status, error, count,
            winSize.width*2 + 1, level, criteria.max_iter,
            (float)criteria.epsilon, ipp_optflow_state ) >= 0 )
        {
            for( i = 0; i < count; i++ )
                status[i] = status[i] == 0;
            EXIT;
        }
    }
#endif

    /* buffer_size = <size for patches> + <size for pyramids> */
    bufferBytes = (srcPatchLen + patchLen * 3) * sizeof( _patchI[0][0] ) * threadCount;
    CV_CALL( buffer = (uchar*)cvAlloc( bufferBytes ));

    for( i = 0; i < threadCount; i++ )
    {
        _patchI[i] = i == 0 ? (float*)buffer : _Iy[i-1] + patchLen;
        _patchJ[i] = _patchI[i] + srcPatchLen;
        _Ix[i] = _patchJ[i] + patchLen;
        _Iy[i] = _Ix[i] + patchLen;
    }

    memset( status, 1, count );
    if( error )
        memset( error, 0, count*sizeof(error[0]) );

    if( !(flags & CV_LKFLOW_INITIAL_GUESSES) )
        memcpy( featuresB, featuresA, count*sizeof(featuresA[0]));

    /* do processing from top pyramid level (smallest image)
       to the bottom (original image) */
    for( l = level; l >= 0; l-- )
    {
        CvSize levelSize = size[l];
        int levelStep = step[l];

        {
#ifdef _OPENMP
        #pragma omp parallel for num_threads(threadCount) schedule(dynamic) 
#endif // _OPENMP
        /* find flow for each given point */
        for( i = 0; i < count; i++ )
        {
            CvPoint2D32f v;
            CvPoint minI, maxI, minJ, maxJ;
            CvSize isz, jsz;
            int pt_status;
            CvPoint2D32f u;
            CvPoint prev_minJ = { -1, -1 }, prev_maxJ = { -1, -1 };
            double Gxx = 0, Gxy = 0, Gyy = 0, D = 0, minEig = 0;
            float prev_mx = 0, prev_my = 0;
            int j, x, y;
            int threadIdx = cvGetThreadNum();
            float* patchI = _patchI[threadIdx];
            float* patchJ = _patchJ[threadIdx];
            float* Ix = _Ix[threadIdx];
            float* Iy = _Iy[threadIdx];

            v.x = featuresB[i].x;
            v.y = featuresB[i].y;
            if( l < level )
            {
                v.x += v.x;
                v.y += v.y;
            }
            else
            {
                v.x = (float)(v.x * scale[l]);
                v.y = (float)(v.y * scale[l]);
            }

            pt_status = status[i];
            if( !pt_status )
                continue;

            minI = maxI = minJ = maxJ = cvPoint( 0, 0 );

            u.x = (float) (featuresA[i].x * scale[l]);
            u.y = (float) (featuresA[i].y * scale[l]);

            intersect( u, winSize, levelSize, &minI, &maxI );
            isz = jsz = cvSize(maxI.x - minI.x + 2, maxI.y - minI.y + 2);
            u.x += (minI.x - (patchSize.width - maxI.x + 1))*0.5f;
            u.y += (minI.y - (patchSize.height - maxI.y + 1))*0.5f;

            if( isz.width < 3 || isz.height < 3 ||
                icvGetRectSubPix_8u32f_C1R( imgI[l], levelStep, levelSize,
                    patchI, isz.width*sizeof(patchI[0]), isz, u ) < 0 )
            {
                /* point is outside the image. take the next */
                status[i] = 0;
                continue;
            }

            icvCalcIxIy_32f( patchI, isz.width*sizeof(patchI[0]), Ix, Iy,
                (isz.width-2)*sizeof(patchI[0]), isz, smoothKernel, patchJ );

            for( j = 0; j < criteria.max_iter; j++ )
            {
                double bx = 0, by = 0;
                float mx, my;
                CvPoint2D32f _v;

                intersect( v, winSize, levelSize, &minJ, &maxJ );

                minJ.x = MAX( minJ.x, minI.x );
                minJ.y = MAX( minJ.y, minI.y );

                maxJ.x = MIN( maxJ.x, maxI.x );
                maxJ.y = MIN( maxJ.y, maxI.y );

                jsz = cvSize(maxJ.x - minJ.x, maxJ.y - minJ.y);

                _v.x = v.x + (minJ.x - (patchSize.width - maxJ.x + 1))*0.5f;
                _v.y = v.y + (minJ.y - (patchSize.height - maxJ.y + 1))*0.5f;

                if( jsz.width < 1 || jsz.height < 1 ||
                    icvGetRectSubPix_8u32f_C1R( imgJ[l], levelStep, levelSize, patchJ,
                                                jsz.width*sizeof(patchJ[0]), jsz, _v ) < 0 )
                {
                    /* point is outside image. take the next */
                    pt_status = 0;
                    break;
                }

                if( maxJ.x == prev_maxJ.x && maxJ.y == prev_maxJ.y &&
                    minJ.x == prev_minJ.x && minJ.y == prev_minJ.y )
                {
                    for( y = 0; y < jsz.height; y++ )
                    {
                        const float* pi = patchI +
                            (y + minJ.y - minI.y + 1)*isz.width + minJ.x - minI.x + 1;
                        const float* pj = patchJ + y*jsz.width;
                        const float* ix = Ix +
                            (y + minJ.y - minI.y)*(isz.width-2) + minJ.x - minI.x;
                        const float* iy = Iy + (ix - Ix);

                        for( x = 0; x < jsz.width; x++ )
                        {
                            double t0 = pi[x] - pj[x];
                            bx += t0 * ix[x];
                            by += t0 * iy[x];
                        }
                    }
                }
                else
                {
                    Gxx = Gyy = Gxy = 0;
                    for( y = 0; y < jsz.height; y++ )
                    {
                        const float* pi = patchI +
                            (y + minJ.y - minI.y + 1)*isz.width + minJ.x - minI.x + 1;
                        const float* pj = patchJ + y*jsz.width;
                        const float* ix = Ix +
                            (y + minJ.y - minI.y)*(isz.width-2) + minJ.x - minI.x;
                        const float* iy = Iy + (ix - Ix);

                        for( x = 0; x < jsz.width; x++ )
                        {
                            double t = pi[x] - pj[x];
                            bx += (double) (t * ix[x]);
                            by += (double) (t * iy[x]);
                            Gxx += ix[x] * ix[x];
                            Gxy += ix[x] * iy[x];
                            Gyy += iy[x] * iy[x];
                        }
                    }

                    D = Gxx * Gyy - Gxy * Gxy;
                    if( D < DBL_EPSILON )
                    {
                        pt_status = 0;
                        break;
                    }

                    // Adi Shavit - 2008.05
                    if( flags & CV_LKFLOW_GET_MIN_EIGENVALS )
                        minEig = (Gyy + Gxx - sqrt((Gxx-Gyy)*(Gxx-Gyy) + 4.*Gxy*Gxy))/(2*jsz.height*jsz.width);

                    D = 1. / D;

                    prev_minJ = minJ;
                    prev_maxJ = maxJ;
                }

                mx = (float) ((Gyy * bx - Gxy * by) * D);
                my = (float) ((Gxx * by - Gxy * bx) * D);

                v.x += mx;
                v.y += my;

                if( mx * mx + my * my < criteria.epsilon )
                    break;

                if( j > 0 && fabs(mx + prev_mx) < 0.01 && fabs(my + prev_my) < 0.01 )
                {
                    v.x -= mx*0.5f;
                    v.y -= my*0.5f;
                    break;
                }
                prev_mx = mx;
                prev_my = my;
            }

            featuresB[i] = v;
            status[i] = (char)pt_status;
            if( l == 0 && error && pt_status )
            {
                /* calc error */
                double err = 0;
                if( flags & CV_LKFLOW_GET_MIN_EIGENVALS )
                    err = minEig;
                else
                {
                    for( y = 0; y < jsz.height; y++ )
                    {
                        const float* pi = patchI +
                            (y + minJ.y - minI.y + 1)*isz.width + minJ.x - minI.x + 1;
                        const float* pj = patchJ + y*jsz.width;

                        for( x = 0; x < jsz.width; x++ )
                        {
                            double t = pi[x] - pj[x];
                            err += t * t;
                        }
                    }
                    err = sqrt(err);
                }
                error[i] = (float)err;
            }
        } // end of point processing loop (i)
        }
    } // end of pyramid levels loop (l)

    __END__;

    if( ipp_optflow_state )
        icvOpticalFlowPyrLKFree_8u_C1R_p( ipp_optflow_state );

    cvFree( &pyrBuffer );
    cvFree( &buffer );
    cvFree( &_error );
    cvFree( &_status );
}


/* Affine tracking algorithm */

CV_IMPL void
cvCalcAffineFlowPyrLK( const void* arrA, const void* arrB,
                       void* pyrarrA, void* pyrarrB,
                       const CvPoint2D32f * featuresA,
                       CvPoint2D32f * featuresB,
                       float *matrices, int count,
                       CvSize winSize, int level,
                       char *status, float *error,
                       CvTermCriteria criteria, int flags )
{
    const int MAX_ITERS = 100;

    char* _status = 0;
    uchar *buffer = 0;
    uchar *pyr_buffer = 0;

    CV_FUNCNAME( "cvCalcAffineFlowPyrLK" );

    __BEGIN__;

    CvMat stubA, *imgA = (CvMat*)arrA;
    CvMat stubB, *imgB = (CvMat*)arrB;
    CvMat pstubA, *pyrA = (CvMat*)pyrarrA;
    CvMat pstubB, *pyrB = (CvMat*)pyrarrB;

    static const float smoothKernel[] = { 0.09375, 0.3125, 0.09375 };  /* 3/32, 10/32, 3/32 */

    int bufferBytes = 0;

    uchar **imgI = 0;
    uchar **imgJ = 0;
    int *step = 0;
    double *scale = 0;
    CvSize* size = 0;

    float *patchI;
    float *patchJ;
    float *Ix;
    float *Iy;

    int i, j, k, l;

    CvSize patchSize = cvSize( winSize.width * 2 + 1, winSize.height * 2 + 1 );
    int patchLen = patchSize.width * patchSize.height;
    int patchStep = patchSize.width * sizeof( patchI[0] );

    CvSize srcPatchSize = cvSize( patchSize.width + 2, patchSize.height + 2 );
    int srcPatchLen = srcPatchSize.width * srcPatchSize.height;
    int srcPatchStep = srcPatchSize.width * sizeof( patchI[0] );
    CvSize imgSize;
    float eps = (float)MIN(winSize.width, winSize.height);

    CV_CALL( imgA = cvGetMat( imgA, &stubA ));
    CV_CALL( imgB = cvGetMat( imgB, &stubB ));

    if( CV_MAT_TYPE( imgA->type ) != CV_8UC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( !CV_ARE_TYPES_EQ( imgA, imgB ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( imgA, imgB ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( imgA->step != imgB->step )
        CV_ERROR( CV_StsUnmatchedSizes, "imgA and imgB must have equal steps" );

    if( !matrices )
        CV_ERROR( CV_StsNullPtr, "" );

    imgSize = cvGetMatSize( imgA );

    if( pyrA )
    {
        CV_CALL( pyrA = cvGetMat( pyrA, &pstubA ));

        if( pyrA->step*pyrA->height < icvMinimalPyramidSize( imgSize ) )
            CV_ERROR( CV_StsBadArg, "pyramid A has insufficient size" );
    }
    else
    {
        pyrA = &pstubA;
        pyrA->data.ptr = 0;
    }

    if( pyrB )
    {
        CV_CALL( pyrB = cvGetMat( pyrB, &pstubB ));

        if( pyrB->step*pyrB->height < icvMinimalPyramidSize( imgSize ) )
            CV_ERROR( CV_StsBadArg, "pyramid B has insufficient size" );
    }
    else
    {
        pyrB = &pstubB;
        pyrB->data.ptr = 0;
    }

    if( count == 0 )
        EXIT;

    /* check input arguments */
    if( !featuresA || !featuresB || !matrices )
        CV_ERROR( CV_StsNullPtr, "" );

    if( winSize.width <= 1 || winSize.height <= 1 )
        CV_ERROR( CV_StsOutOfRange, "the search window is too small" );

    if( count < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    CV_CALL( icvInitPyramidalAlgorithm( imgA, imgB,
        pyrA, pyrB, level, &criteria, MAX_ITERS, flags,
        &imgI, &imgJ, &step, &size, &scale, &pyr_buffer ));

    /* buffer_size = <size for patches> + <size for pyramids> */
    bufferBytes = (srcPatchLen + patchLen*3)*sizeof(patchI[0]) + (36*2 + 6)*sizeof(double);

    CV_CALL( buffer = (uchar*)cvAlloc(bufferBytes));

    if( !status )
        CV_CALL( status = _status = (char*)cvAlloc(count) );

    patchI = (float *) buffer;
    patchJ = patchI + srcPatchLen;
    Ix = patchJ + patchLen;
    Iy = Ix + patchLen;

    if( status )
        memset( status, 1, count );

    if( !(flags & CV_LKFLOW_INITIAL_GUESSES) )
    {
        memcpy( featuresB, featuresA, count * sizeof( featuresA[0] ));
        for( i = 0; i < count * 4; i += 4 )
        {
            matrices[i] = matrices[i + 3] = 1.f;
            matrices[i + 1] = matrices[i + 2] = 0.f;
        }
    }

    for( i = 0; i < count; i++ )
    {
        featuresB[i].x = (float)(featuresB[i].x * scale[level] * 0.5);
        featuresB[i].y = (float)(featuresB[i].y * scale[level] * 0.5);
    }

    /* do processing from top pyramid level (smallest image)
       to the bottom (original image) */
    for( l = level; l >= 0; l-- )
    {
        CvSize levelSize = size[l];
        int levelStep = step[l];

        /* find flow for each given point at the particular level */
        for( i = 0; i < count; i++ )
        {
            CvPoint2D32f u;
            float Av[6];
            double G[36];
            double meanI = 0, meanJ = 0;
            int x, y;
            int pt_status = status[i];
            CvMat mat;

            if( !pt_status )
                continue;

            Av[0] = matrices[i*4];
            Av[1] = matrices[i*4+1];
            Av[3] = matrices[i*4+2];
            Av[4] = matrices[i*4+3];

            Av[2] = featuresB[i].x += featuresB[i].x;
            Av[5] = featuresB[i].y += featuresB[i].y;

            u.x = (float) (featuresA[i].x * scale[l]);
            u.y = (float) (featuresA[i].y * scale[l]);

            if( u.x < -eps || u.x >= levelSize.width+eps ||
                u.y < -eps || u.y >= levelSize.height+eps ||
                icvGetRectSubPix_8u32f_C1R( imgI[l], levelStep,
                levelSize, patchI, srcPatchStep, srcPatchSize, u ) < 0 )
            {
                /* point is outside the image. take the next */
                if( l == 0 )
                    status[i] = 0;
                continue;
            }

            icvCalcIxIy_32f( patchI, srcPatchStep, Ix, Iy,
                (srcPatchSize.width-2)*sizeof(patchI[0]), srcPatchSize,
                smoothKernel, patchJ );

            /* repack patchI (remove borders) */
            for( k = 0; k < patchSize.height; k++ )
                memcpy( patchI + k * patchSize.width,
                        patchI + (k + 1) * srcPatchSize.width + 1, patchStep );

            memset( G, 0, sizeof( G ));

            /* calculate G matrix */
            for( y = -winSize.height, k = 0; y <= winSize.height; y++ )
            {
                for( x = -winSize.width; x <= winSize.width; x++, k++ )
                {
                    double ixix = ((double) Ix[k]) * Ix[k];
                    double ixiy = ((double) Ix[k]) * Iy[k];
                    double iyiy = ((double) Iy[k]) * Iy[k];

                    double xx, xy, yy;

                    G[0] += ixix;
                    G[1] += ixiy;
                    G[2] += x * ixix;
                    G[3] += y * ixix;
                    G[4] += x * ixiy;
                    G[5] += y * ixiy;

                    // G[6] == G[1]
                    G[7] += iyiy;
                    // G[8] == G[4]
                    // G[9] == G[5]
                    G[10] += x * iyiy;
                    G[11] += y * iyiy;

                    xx = x * x;
                    xy = x * y;
                    yy = y * y;

                    // G[12] == G[2]
                    // G[13] == G[8] == G[4]
                    G[14] += xx * ixix;
                    G[15] += xy * ixix;
                    G[16] += xx * ixiy;
                    G[17] += xy * ixiy;

                    // G[18] == G[3]
                    // G[19] == G[9]
                    // G[20] == G[15]
                    G[21] += yy * ixix;
                    // G[22] == G[17]
                    G[23] += yy * ixiy;

                    // G[24] == G[4]
                    // G[25] == G[10]
                    // G[26] == G[16]
                    // G[27] == G[22]
                    G[28] += xx * iyiy;
                    G[29] += xy * iyiy;

                    // G[30] == G[5]
                    // G[31] == G[11]
                    // G[32] == G[17]
                    // G[33] == G[23]
                    // G[34] == G[29]
                    G[35] += yy * iyiy;

                    meanI += patchI[k];
                }
            }

            meanI /= patchSize.width*patchSize.height;

            G[8] = G[4];
            G[9] = G[5];
            G[22] = G[17];

            // fill part of G below its diagonal
            for( y = 1; y < 6; y++ )
                for( x = 0; x < y; x++ )
                    G[y * 6 + x] = G[x * 6 + y];

            cvInitMatHeader( &mat, 6, 6, CV_64FC1, G );

            if( cvInvert( &mat, &mat, CV_SVD ) < 1e-4 )
            {
                /* bad matrix. take the next point */
                if( l == 0 )
                    status[i] = 0;
                continue;
            }

            for( j = 0; j < criteria.max_iter; j++ )
            {
                double b[6] = {0,0,0,0,0,0}, eta[6];
                double t0, t1, s = 0;

                if( Av[2] < -eps || Av[2] >= levelSize.width+eps ||
                    Av[5] < -eps || Av[5] >= levelSize.height+eps ||
                    icvGetQuadrangleSubPix_8u32f_C1R( imgJ[l], levelStep,
                    levelSize, patchJ, patchStep, patchSize, Av ) < 0 )
                {
                    pt_status = 0;
                    break;
                }

                for( y = -winSize.height, k = 0, meanJ = 0; y <= winSize.height; y++ )
                    for( x = -winSize.width; x <= winSize.width; x++, k++ )
                        meanJ += patchJ[k];

                meanJ = meanJ / (patchSize.width * patchSize.height) - meanI;

                for( y = -winSize.height, k = 0; y <= winSize.height; y++ )
                {
                    for( x = -winSize.width; x <= winSize.width; x++, k++ )
                    {
                        double t = patchI[k] - patchJ[k] + meanJ;
                        double ixt = Ix[k] * t;
                        double iyt = Iy[k] * t;

                        s += t;

                        b[0] += ixt;
                        b[1] += iyt;
                        b[2] += x * ixt;
                        b[3] += y * ixt;
                        b[4] += x * iyt;
                        b[5] += y * iyt;
                    }
                }

                icvTransformVector_64d( G, b, eta, 6, 6 );

                Av[2] = (float)(Av[2] + Av[0] * eta[0] + Av[1] * eta[1]);
                Av[5] = (float)(Av[5] + Av[3] * eta[0] + Av[4] * eta[1]);

                t0 = Av[0] * (1 + eta[2]) + Av[1] * eta[4];
                t1 = Av[0] * eta[3] + Av[1] * (1 + eta[5]);
                Av[0] = (float)t0;
                Av[1] = (float)t1;

                t0 = Av[3] * (1 + eta[2]) + Av[4] * eta[4];
                t1 = Av[3] * eta[3] + Av[4] * (1 + eta[5]);
                Av[3] = (float)t0;
                Av[4] = (float)t1;

                if( eta[0] * eta[0] + eta[1] * eta[1] < criteria.epsilon )
                    break;
            }

            if( pt_status != 0 || l == 0 )
            {
                status[i] = (char)pt_status;
                featuresB[i].x = Av[2];
                featuresB[i].y = Av[5];
            
                matrices[i*4] = Av[0];
                matrices[i*4+1] = Av[1];
                matrices[i*4+2] = Av[3];
                matrices[i*4+3] = Av[4];
            }

            if( pt_status && l == 0 && error )
            {
                /* calc error */
                double err = 0;

                for( y = 0, k = 0; y < patchSize.height; y++ )
                {
                    for( x = 0; x < patchSize.width; x++, k++ )
                    {
                        double t = patchI[k] - patchJ[k] + meanJ;
                        err += t * t;
                    }
                }
                error[i] = (float)sqrt(err);
            }
        }
    }

    __END__;

    cvFree( &pyr_buffer );
    cvFree( &buffer );
    cvFree( &_status );
}



static void
icvGetRTMatrix( const CvPoint2D32f* a, const CvPoint2D32f* b,
                int count, CvMat* M, int full_affine )
{
    if( full_affine )
    {
        double sa[36], sb[6];
        CvMat A = cvMat( 6, 6, CV_64F, sa ), B = cvMat( 6, 1, CV_64F, sb );
        CvMat MM = cvMat( 6, 1, CV_64F, M->data.db );

        int i;

        memset( sa, 0, sizeof(sa) );
        memset( sb, 0, sizeof(sb) );

        for( i = 0; i < count; i++ )
        {
            sa[0] += a[i].x*a[i].x;
            sa[1] += a[i].y*a[i].x;
            sa[2] += a[i].x;

            sa[6] += a[i].x*a[i].y;
            sa[7] += a[i].y*a[i].y;
            sa[8] += a[i].y;

            sa[12] += a[i].x;
            sa[13] += a[i].y;
            sa[14] += 1;

            sb[0] += a[i].x*b[i].x;
            sb[1] += a[i].y*b[i].x;
            sb[2] += b[i].x;
            sb[3] += a[i].x*b[i].y;
            sb[4] += a[i].y*b[i].y;
            sb[5] += b[i].y;
        }

        sa[21] = sa[0];
        sa[22] = sa[1];
        sa[23] = sa[2];
        sa[27] = sa[6];
        sa[28] = sa[7];
        sa[29] = sa[8];
        sa[33] = sa[12];
        sa[34] = sa[13];
        sa[35] = sa[14];

        cvSolve( &A, &B, &MM, CV_SVD );
    }
    else
    {
        double sa[16], sb[4], m[4], *om = M->data.db;
        CvMat A = cvMat( 4, 4, CV_64F, sa ), B = cvMat( 4, 1, CV_64F, sb );
        CvMat MM = cvMat( 4, 1, CV_64F, m );

        int i;

        memset( sa, 0, sizeof(sa) );
        memset( sb, 0, sizeof(sb) );

        for( i = 0; i < count; i++ )
        {
            sa[0] += a[i].x*a[i].x + a[i].y*a[i].y;
            sa[1] += 0;
            sa[2] += a[i].x;
            sa[3] += a[i].y;

            sa[4] += 0;
            sa[5] += a[i].x*a[i].x + a[i].y*a[i].y;
            sa[6] += -a[i].y;
            sa[7] += a[i].x;

            sa[8] += a[i].x;
            sa[9] += -a[i].y;
            sa[10] += 1;
            sa[11] += 0;

            sa[12] += a[i].y;
            sa[13] += a[i].x;
            sa[14] += 0;
            sa[15] += 1;

            sb[0] += a[i].x*b[i].x + a[i].y*b[i].y;
            sb[1] += a[i].x*b[i].y - a[i].y*b[i].x;
            sb[2] += b[i].x;
            sb[3] += b[i].y;
        }

        cvSolve( &A, &B, &MM, CV_SVD );

        om[0] = om[4] = m[0];
        om[1] = -m[1];
        om[3] = m[1];
        om[2] = m[2];
        om[5] = m[3];
    }
}


CV_IMPL int
cvEstimateRigidTransform( const CvArr* _A, const CvArr* _B, CvMat* _M, int full_affine )
{
    int result = 0;
    
    const int COUNT = 15;
    const int WIDTH = 160, HEIGHT = 120;
    const int RANSAC_MAX_ITERS = 100;
    const int RANSAC_SIZE0 = 3;
    const double MIN_TRIANGLE_SIDE = 20;
    const double RANSAC_GOOD_RATIO = 0.5;

    int allocated = 1;
    CvMat *sA = 0, *sB = 0;
    CvPoint2D32f *pA = 0, *pB = 0;
    int* good_idx = 0;
    char *status = 0;
    CvMat* gray = 0;

    CV_FUNCNAME( "cvEstimateRigidTransform" );

    __BEGIN__;

    CvMat stubA, *A;
    CvMat stubB, *B;
    CvSize sz0, sz1;
    int cn, equal_sizes;
    int i, j, k, k1;
    int count_x, count_y, count;
    double scale = 1;
    CvRNG rng = cvRNG(-1);
    double m[6]={0};
    CvMat M = cvMat( 2, 3, CV_64F, m );
    int good_count = 0;

    CV_CALL( A = cvGetMat( _A, &stubA ));
    CV_CALL( B = cvGetMat( _B, &stubB ));

    if( !CV_IS_MAT(_M) )
        CV_ERROR( _M ? CV_StsBadArg : CV_StsNullPtr, "Output parameter M is not a valid matrix" );

    if( !CV_ARE_SIZES_EQ( A, B ) )
        CV_ERROR( CV_StsUnmatchedSizes, "Both input images must have the same size" );

    if( !CV_ARE_TYPES_EQ( A, B ) )
        CV_ERROR( CV_StsUnmatchedFormats, "Both input images must have the same data type" );

    if( CV_MAT_TYPE(A->type) == CV_8UC1 || CV_MAT_TYPE(A->type) == CV_8UC3 )
    {
        cn = CV_MAT_CN(A->type);
        sz0 = cvGetSize(A);
        sz1 = cvSize(WIDTH, HEIGHT);

        scale = MAX( (double)sz1.width/sz0.width, (double)sz1.height/sz0.height );
        scale = MIN( scale, 1. );
        sz1.width = cvRound( sz0.width * scale );
        sz1.height = cvRound( sz0.height * scale );

        equal_sizes = sz1.width == sz0.width && sz1.height == sz0.height;

        if( !equal_sizes || cn != 1 )
        {
            CV_CALL( sA = cvCreateMat( sz1.height, sz1.width, CV_8UC1 ));
            CV_CALL( sB = cvCreateMat( sz1.height, sz1.width, CV_8UC1 ));

            if( !equal_sizes && cn != 1 )
                CV_CALL( gray = cvCreateMat( sz0.height, sz0.width, CV_8UC1 ));

            if( gray )
            {
                cvCvtColor( A, gray, CV_BGR2GRAY );
                cvResize( gray, sA, CV_INTER_AREA );
                cvCvtColor( B, gray, CV_BGR2GRAY );
                cvResize( gray, sB, CV_INTER_AREA );
            }
            else if( cn == 1 )
            {
                cvResize( gray, sA, CV_INTER_AREA );
                cvResize( gray, sB, CV_INTER_AREA );
            }
            else
            {
                cvCvtColor( A, gray, CV_BGR2GRAY );
                cvResize( gray, sA, CV_INTER_AREA );
                cvCvtColor( B, gray, CV_BGR2GRAY );
            }

            cvReleaseMat( &gray );
            A = sA;
            B = sB;
        }

        count_y = COUNT;
        count_x = cvRound((double)COUNT*sz1.width/sz1.height);
        count = count_x * count_y;

        CV_CALL( pA = (CvPoint2D32f*)cvAlloc( count*sizeof(pA[0]) ));
        CV_CALL( pB = (CvPoint2D32f*)cvAlloc( count*sizeof(pB[0]) ));
        CV_CALL( status = (char*)cvAlloc( count*sizeof(status[0]) ));

        for( i = 0, k = 0; i < count_y; i++ )
            for( j = 0; j < count_x; j++, k++ )
            {
                pA[k].x = (j+0.5f)*sz1.width/count_x;
                pA[k].y = (i+0.5f)*sz1.height/count_y;
            }

        // find the corresponding points in B
        cvCalcOpticalFlowPyrLK( A, B, 0, 0, pA, pB, count, cvSize(10,10), 3,
                                status, 0, cvTermCriteria(CV_TERMCRIT_ITER,40,0.1), 0 );

        // repack the remained points
        for( i = 0, k = 0; i < count; i++ )
            if( status[i] )
            {
                if( i > k )
                {
                    pA[k] = pA[i];
                    pB[k] = pB[i];
                }
                k++;
            }

        count = k;
    }
    else if( CV_MAT_TYPE(A->type) == CV_32FC2 || CV_MAT_TYPE(A->type) == CV_32SC2 )
    {
        count = A->cols*A->rows;

        if( CV_IS_MAT_CONT(A->type & B->type) && CV_MAT_TYPE(A->type) == CV_32FC2 )
        {
            pA = (CvPoint2D32f*)A->data.ptr;
            pB = (CvPoint2D32f*)B->data.ptr;
            allocated = 0;
        }
        else
        {
            CvMat _pA, _pB;

            CV_CALL( pA = (CvPoint2D32f*)cvAlloc( count*sizeof(pA[0]) ));
            CV_CALL( pB = (CvPoint2D32f*)cvAlloc( count*sizeof(pB[0]) ));
            _pA = cvMat( A->rows, A->cols, CV_32FC2, pA );
            _pB = cvMat( B->rows, B->cols, CV_32FC2, pB );
            cvConvert( A, &_pA );
            cvConvert( B, &_pB );
        }
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "Both input images must have either 8uC1 or 8uC3 type" );

    CV_CALL( good_idx = (int*)cvAlloc( count*sizeof(good_idx[0]) ));

    if( count < RANSAC_SIZE0 )
        EXIT;

    // RANSAC stuff:
    // 1. find the consensus
    for( k = 0; k < RANSAC_MAX_ITERS; k++ )
    {
        int idx[RANSAC_SIZE0];
        CvPoint2D32f a[3];
        CvPoint2D32f b[3];

        memset( a, 0, sizeof(a) );
        memset( b, 0, sizeof(b) );

        // choose random 3 non-complanar points from A & B
        for( i = 0; i < RANSAC_SIZE0; i++ )
        {
            for( k1 = 0; k1 < RANSAC_MAX_ITERS; k1++ )
            {
                idx[i] = cvRandInt(&rng) % count;
                
                for( j = 0; j < i; j++ )
                {
                    if( idx[j] == idx[i] )
                        break;
                    // check that the points are not very close one each other
                    if( fabs(pA[idx[i]].x - pA[idx[j]].x) +
                        fabs(pA[idx[i]].y - pA[idx[j]].y) < MIN_TRIANGLE_SIDE )
                        break;
                    if( fabs(pB[idx[i]].x - pB[idx[j]].x) +
                        fabs(pB[idx[i]].y - pB[idx[j]].y) < MIN_TRIANGLE_SIDE )
                        break;
                }

                if( j < i )
                    continue;

                if( i+1 == RANSAC_SIZE0 )
                {
                    // additional check for non-complanar vectors
                    a[0] = pA[idx[0]];
                    a[1] = pA[idx[1]];
                    a[2] = pA[idx[2]];

                    b[0] = pB[idx[0]];
                    b[1] = pB[idx[1]];
                    b[2] = pB[idx[2]];

                    if( fabs((a[1].x - a[0].x)*(a[2].y - a[0].y) - (a[1].y - a[0].y)*(a[2].x - a[0].x)) < 1 ||
                        fabs((b[1].x - b[0].x)*(b[2].y - b[0].y) - (b[1].y - b[0].y)*(b[2].x - b[0].x)) < 1 )
                        continue;
                }
                break;
            }

            if( k1 >= RANSAC_MAX_ITERS )
                break;
        }

        if( i < RANSAC_SIZE0 )
            continue;

        // estimate the transformation using 3 points
        icvGetRTMatrix( a, b, 3, &M, full_affine );

        for( i = 0, good_count = 0; i < count; i++ )
        {
            if( fabs( m[0]*pA[i].x + m[1]*pA[i].y + m[2] - pB[i].x ) +
                fabs( m[3]*pA[i].x + m[4]*pA[i].y + m[5] - pB[i].y ) < 8 )
                good_idx[good_count++] = i;
        }

        if( good_count >= count*RANSAC_GOOD_RATIO )
            break;
    }

    if( k >= RANSAC_MAX_ITERS )
        EXIT;

    if( good_count < count )
    {
        for( i = 0; i < good_count; i++ )
        {
            j = good_idx[i];
            pA[i] = pA[j];
            pB[i] = pB[j];
        }
    }

    icvGetRTMatrix( pA, pB, good_count, &M, full_affine );
    m[2] /= scale;
    m[5] /= scale;
    CV_CALL( cvConvert( &M, _M ));
    result = 1;

    __END__;

    cvReleaseMat( &sA );
    cvReleaseMat( &sB );
    cvFree( &pA );
    cvFree( &pB );
    cvFree( &status );
    cvFree( &good_idx );
    cvReleaseMat( &gray );

    return result;
}


/* End of file. */
