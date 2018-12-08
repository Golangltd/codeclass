/* Original code has been submitted by Liu Liu. Here is the copyright.
----------------------------------------------------------------------------------
 * An OpenCV Implementation of SURF
 * Further Information Refer to "SURF: Speed-Up Robust Feature"
 * Author: Liu Liu
 * liuliu.1987+opencv@gmail.com
 *
 * There are still serveral lacks for this experimental implementation:
 * 1.The interpolation of sub-pixel mentioned in article was not implemented yet;
 * 2.A comparision with original libSurf.so shows that the hessian detector is not a 100% match to their implementation;
 * 3.Due to above reasons, I recommanded the original one for study and reuse;
 *
 * However, the speed of this implementation is something comparable to original one.
 *
 * Copyright© 2008, Liu Liu All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 	Redistributions of source code must retain the above
 * 	copyright notice, this list of conditions and the following
 * 	disclaimer.
 * 	Redistributions in binary form must reproduce the above
 * 	copyright notice, this list of conditions and the following
 * 	disclaimer in the documentation and/or other materials
 * 	provided with the distribution.
 * 	The name of Contributor may not be used to endorse or
 * 	promote products derived from this software without
 * 	specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/* 
   The following changes have been made, comparing to the original contribution:
   1. A lot of small optimizations, less memory allocations, got rid of global buffers
   2. Reversed order of cvGetQuadrangleSubPix and cvResize calls; probably less accurate, but much faster
   3. The descriptor computing part (which is most expensive) is threaded using OpenMP
   (subpixel-accurate keypoint localization and scale estimation are still TBD)
*/

#include "_cv.h"

CvSURFParams cvSURFParams(double threshold, int extended)
{
    CvSURFParams params;
    params.hessianThreshold = threshold;
    params.extended = extended;
    params.nOctaves = 3;
    params.nOctaveLayers = 4;
    return params;
}

struct CvSurfHF
{
    int p0, p1, p2, p3;
    float w;
};

CV_INLINE float
icvCalcHaarPattern( const int* origin, const CvSurfHF* f, int n )
{
    double d = 0;
    for( int k = 0; k < n; k++ )
        d += (origin[f[k].p0] + origin[f[k].p3] - origin[f[k].p1] - origin[f[k].p2])*f[k].w;
    return (float)d;
}

static void
icvResizeHaarPattern( const int src[][5], CvSurfHF* dst, int n, int oldSize, int newSize, int widthStep )
{
    for( int k = 0; k < n; k++ )
    {
        int dx1 = src[k][0]*newSize/oldSize;
        int dy1 = src[k][1]*newSize/oldSize;
        int dx2 = src[k][2]*newSize/oldSize;
        int dy2 = src[k][3]*newSize/oldSize;
        dst[k].p0 = dy1*widthStep + dx1;
        dst[k].p1 = dy2*widthStep + dx1;
        dst[k].p2 = dy1*widthStep + dx2;
        dst[k].p3 = dy2*widthStep + dx2;
        dst[k].w = src[k][4]/((float)(dx2-dx1)*(dy2-dy1));
    }
}

static CvSeq* icvFastHessianDetector( const CvMat* sum, const CvMat* mask_sum,
    CvMemStorage* storage, const CvSURFParams* params )
{
    CvSeq* points = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvSURFPoint), storage );
    
    int totalLayers = params->nOctaves*(params->nOctaveLayers+2);
    CvMat** hessians = (CvMat**)cvStackAlloc(totalLayers*sizeof(hessians[0]));
    CvMat** traces = (CvMat**)cvStackAlloc(totalLayers*sizeof(traces[0]));
    int size, *sizeCache = (int*)cvStackAlloc(totalLayers*sizeof(sizeCache[0]));
    int scale, *scaleCache = (int*)cvStackAlloc(totalLayers*sizeof(scaleCache[0]));

    const int NX=3, NY=3, NXY=4, SIZE0=9;
    int dx_s[NX][5] = { {0, 2, 3, 7, 1}, {3, 2, 6, 7, -2}, {6, 2, 9, 7, 1} };
    int dy_s[NY][5] = { {2, 0, 7, 3, 1}, {2, 3, 7, 6, -2}, {2, 6, 7, 9, 1} };
    int dxy_s[NXY][5] = { {1, 1, 4, 4, 1}, {5, 1, 8, 4, -1}, {1, 5, 4, 8, -1}, {5, 5, 8, 8, 1} };
    int dm[1][5] = { {0, 0, 9, 9, 1} };
    CvSurfHF Dx[NX], Dy[NY], Dxy[NXY], Dm;
    double dx = 0, dy = 0, dxy = 0;
    int hessian_rows, hessian_cols;
    
    int octave, sc;
    int i, j, k, z;
    int* xofs = (int*)cvStackAlloc(sum->cols*sizeof(xofs[0]));

    /* hessian detector */
    for( octave = k = 0; octave < params->nOctaves; octave++ )
    {
        for( sc = -1; sc <= params->nOctaveLayers; sc++, k++ )
        {
            if ( sc < 0 )
                sizeCache[k] = size = 7 << octave; // gaussian scale 1.0;
            else
                sizeCache[k] = size = (sc*6 + 9) << octave; // gaussian scale size*1.2/9.;
            scaleCache[k] = scale = MAX(size, SIZE0);

            hessian_rows = (sum->rows)*SIZE0/scale;
            hessian_cols = (sum->cols)*SIZE0/scale;
            hessians[k] = cvCreateMat( hessian_rows, hessian_cols, CV_32FC1 );
            traces[k] = cvCreateMat( hessian_rows, hessian_cols, CV_32FC1 );

            icvResizeHaarPattern( dx_s, Dx, NX, SIZE0, size, sum->cols );
            icvResizeHaarPattern( dy_s, Dy, NY, SIZE0, size, sum->cols );
            icvResizeHaarPattern( dxy_s, Dxy, NXY, SIZE0, size, sum->cols );
            for( i = 0; i < NXY; i++ )
                Dxy[i].w *= 0.9f;

            float* hessian = hessians[k]->data.fl;
            float* trace = traces[k]->data.fl;

            for( i = 0; i < hessian_cols*(SIZE0/2); i++ )
                hessian[i] = hessian[hessian_cols*hessian_rows-1-i] =
                trace[i] = trace[hessian_cols*hessian_rows-1-i] = 0.f;

            hessian += (SIZE0/2)*(hessian_cols + 1);
            trace += (SIZE0/2)*(hessian_cols + 1);

            for( j = 0; j <= hessian_cols - SIZE0; j++ )
                xofs[j] = j*scale/SIZE0;

            for( i = 0; i < hessian_rows - SIZE0; i++,
                trace += hessian_cols, hessian += hessian_cols )
            {
                const int* sum_ptr = sum->data.i + sum->cols*(i*scale/SIZE0);
                for( j = 0; j < SIZE0/2; j++ )
                    hessian[-j-1] = hessian[hessian_cols - SIZE0 + j] =
                    trace[-j-1] = trace[hessian_cols - SIZE0 + j] = 0.f;
                for( j = 0; j <= hessian_cols - SIZE0; j++ )
                {
                    const int* s = sum_ptr + xofs[j];
                    dx = (s[Dx[0].p0] + s[Dx[0].p3] - s[Dx[0].p1] - s[Dx[0].p2])*Dx[0].w +
                        (s[Dx[1].p0] + s[Dx[1].p3] - s[Dx[1].p1] - s[Dx[1].p2])*Dx[1].w +
                        (s[Dx[2].p0] + s[Dx[2].p3] - s[Dx[2].p1] - s[Dx[2].p2])*Dx[2].w;
                    dy = (s[Dy[0].p0] + s[Dy[0].p3] - s[Dy[0].p1] - s[Dy[0].p2])*Dy[0].w +
                        (s[Dy[1].p0] + s[Dy[1].p3] - s[Dy[1].p1] - s[Dy[1].p2])*Dy[1].w +
                        (s[Dy[2].p0] + s[Dy[2].p3] - s[Dy[2].p1] - s[Dy[2].p2])*Dy[2].w;
                    dxy = (s[Dxy[0].p0] + s[Dxy[0].p3] - s[Dxy[0].p1] - s[Dxy[0].p2])*Dxy[0].w +
                        (s[Dxy[1].p0] + s[Dxy[1].p3] - s[Dxy[1].p1] - s[Dxy[1].p2])*Dxy[1].w +
                        (s[Dxy[2].p0] + s[Dxy[2].p3] - s[Dxy[2].p1] - s[Dxy[2].p2])*Dxy[2].w +
                        (s[Dxy[3].p0] + s[Dxy[3].p3] - s[Dxy[3].p1] - s[Dxy[3].p2])*Dxy[3].w;
                    hessian[j] = (float)(dx*dy - dxy*dxy);
                    trace[j] = (float)(dx + dy);
                }
            }
        }
    }

    for( octave = 0, k = 1; octave < params->nOctaves; octave++, k+=2 )
    {
        for( sc = 0; sc < params->nOctaveLayers; sc++, k++ )
        {
            size = sizeCache[k];
            scale = scaleCache[k];
            hessian_rows = hessians[k]->rows;
            hessian_cols = hessians[k]->cols;
            icvResizeHaarPattern( dm, &Dm, 1, SIZE0, size, mask_sum ? mask_sum->cols : sum->cols );
            int margin = 5*scaleCache[k+1]/scale;
            for( i = margin; i < hessian_rows-margin; i++ )
            {
                const float* hessian = hessians[k]->data.fl + i*hessian_cols;
                const float* trace = traces[k]->data.fl + i*hessian_cols;
                for( j = margin; j < hessian_cols-margin; j++ )
                {
                    float val0 = hessian[j];
                    if( val0 > params->hessianThreshold )
                    {
                        bool suppressed = false;
                        if( mask_sum )
                        {
                            const int* mask_ptr = mask_sum->data.i +
                                mask_sum->cols*((i-SIZE0/2)*scale/SIZE0) +
                                (j - SIZE0/2)*scale/SIZE0;
                            float mval = icvCalcHaarPattern( mask_ptr, &Dm, 1 );
                            if( mval < 0.5 )
                                continue;
                        }

                        /* non-maxima suppression */
                        for( z = k-1; z < k+2; z++ )
                        {
                            int hcols_z = hessians[z]->cols;
                            const float* hessian = hessians[z]->data.fl + (j*scale+scaleCache[z]/2)/scaleCache[z]-1 +
                                ((i*scale + scaleCache[z]/2)/scaleCache[z]-1)*hcols_z;
                            if( val0 < hessian[0] || val0 < hessian[1] || val0 < hessian[2] ||
                                val0 < hessian[hcols_z] || val0 < hessian[hcols_z+1] ||
                                val0 < hessian[hcols_z+2] || val0 < hessian[hcols_z*2] ||
                                val0 < hessian[hcols_z*2+1] || val0 < hessian[hcols_z*2+2] )
                            {
                                suppressed = true;
                                break;
                            }
                        }
                        if( !suppressed )
                        {
                            double trace_val = trace[j];
                            CvSURFPoint point = cvSURFPoint( cvPoint2D32f(j*scale/9.f, i*scale/9.f),
                                CV_SIGN(trace_val), sizeCache[k], 0, val0 );
                            cvSeqPush( points, &point );
                        }
                    }
                }
            }
        }
    }

    for( octave = k = 0; octave < params->nOctaves; octave++ )
        for( sc = -1; sc <= params->nOctaveLayers; sc++, k++ )
        {
            cvReleaseMat( &hessians[k] );
            cvReleaseMat( &traces[k] );
        }
    return points;
}


CV_IMPL void
cvExtractSURF( const CvArr* _img, const CvArr* _mask,
               CvSeq** _keypoints, CvSeq** _descriptors,
               CvMemStorage* storage, CvSURFParams params )
{
    CvMat *sum = 0, *mask1 = 0, *mask_sum = 0;

    if( _keypoints )
        *_keypoints = 0;
    if( _descriptors )
        *_descriptors = 0;

    CV_FUNCNAME( "cvExtractSURF" );

    __BEGIN__;

    CvSeq *keypoints, *descriptors = 0;
    CvMat imghdr, *img = cvGetMat(_img, &imghdr);
    CvMat maskhdr, *mask = _mask ? cvGetMat(_mask, &maskhdr) : 0;
    
    int descriptor_size = params.extended ? 128 : 64;
    const int descriptor_data_type = CV_32F;
    const int NX=2, NY=2;
    const float sqrt_2 = 1.4142135623730950488016887242097f;
    const int PATCH_SZ = 20;
    const int RS_PATCH_SZ = 30; // ceil((PATCH_SZ+1)*sqrt_2);
    int dx_s[NX][5] = {{0, 0, 2, 4, -1}, {2, 0, 4, 4, 1}};
    int dy_s[NY][5] = {{0, 0, 4, 2, 1}, {0, 2, 4, 4, -1}};
    float G[9] = {0,0,0,0,0,0,0,0,0};
    CvMat _G = cvMat(1, 9, CV_32F, G);
    float DW[PATCH_SZ][PATCH_SZ];
    CvMat _DW = cvMat(PATCH_SZ, PATCH_SZ, CV_32F, DW);
    CvPoint apt[81];
    int i, j, k, nangle0 = 0, N;

    CV_ASSERT( img != 0 && CV_MAT_TYPE(img->type) == CV_8UC1 &&
        (mask == 0 || (CV_ARE_SIZES_EQ(img,mask) &&
        CV_MAT_TYPE(mask->type) == CV_8UC1)) &&
        storage != 0 && params.hessianThreshold >= 0 &&
        params.nOctaves > 0 && params.nOctaveLayers > 0 );

    sum = cvCreateMat( img->height+1, img->width+1, CV_32SC1 );
    cvIntegral( img, sum );
    if( mask )
    {
        mask1 = cvCreateMat( img->height, img->width, CV_8UC1 );
        mask_sum = cvCreateMat( img->height+1, img->width+1, CV_32SC1 );
        cvMinS( mask, 1, mask1 );
        cvIntegral( mask1, mask_sum );
    }
    keypoints = icvFastHessianDetector( sum, mask_sum, storage, &params );
    N = keypoints->total;
    if( _descriptors )
    {
        descriptors = cvCreateSeq( 0, sizeof(CvSeq),
            descriptor_size*CV_ELEM_SIZE(descriptor_data_type), storage );
        cvSeqPushMulti( descriptors, 0, N );
    }

    CvSepFilter::init_gaussian_kernel( &_G, 2.5 );

    {
    const double sigma = 3.3;
    double c2 = 1./(sigma*sigma*2), gs = 0;
    for( i = 0; i < PATCH_SZ; i++ )
    {
        for( j = 0; j < PATCH_SZ; j++ )
        {
            double x = j - PATCH_SZ*0.5, y = i - PATCH_SZ*0.5;
            double val = exp(-(x*x+y*y)*c2);
            DW[i][j] = (float)val;
            gs += val;
        }
    }
    cvScale( &_DW, &_DW, 1./gs );
    }

    for( i = -4; i <= 4; i++ )
        for( j = -4; j <= 4; j++ )
        {
            if( i*i + j*j <= 16 )
                apt[nangle0++] = cvPoint(j,i);
        }

    {
#ifdef _OPENMP
    int nthreads = cvGetNumThreads();
#pragma omp parallel for num_threads(nthreads) schedule(dynamic)
#endif
    for( k = 0; k < N; k++ )
    {
        const int* sum_ptr = sum->data.i;
        int sum_cols = sum->cols;
        int i, j, kk, x, y, nangle;
        CvSurfHF dx_t[NX], dy_t[NY];
        float X[81], Y[81], angle[81];
        uchar PATCH[PATCH_SZ+1][PATCH_SZ+1], RS_PATCH[RS_PATCH_SZ][RS_PATCH_SZ];
        float DX[PATCH_SZ][PATCH_SZ], DY[PATCH_SZ][PATCH_SZ];
        CvMat _X = cvMat(1, 81, CV_32F, X);
        CvMat _Y = cvMat(1, 81, CV_32F, Y);
        CvMat _angle = cvMat(1, 81, CV_32F, angle);
        CvMat _patch = cvMat(PATCH_SZ+1, PATCH_SZ+1, CV_8U, PATCH);
        CvMat _rs_patch = cvMat(RS_PATCH_SZ, RS_PATCH_SZ, CV_8U, RS_PATCH);
        CvMat _src, *src = img;
        
        CvSURFPoint* kp = (CvSURFPoint*)cvGetSeqElem( keypoints, k );
        CvPoint2D32f center = kp->pt;
        int size = kp->size;
        icvResizeHaarPattern( dx_s, dx_t, NX, 9, size, sum->cols );
        icvResizeHaarPattern( dy_s, dy_t, NY, 9, size, sum->cols );
        CvPoint pt = cvPointFrom32f(center);
        float* vec;
        float alpha0, beta0, sz0, scale0;

        for( kk = 0, nangle = 0; kk < nangle0; kk++ )
        {
            j = apt[kk].x; i = apt[kk].y;
            int x = pt.x + (j-2)*size/9;
            int y = pt.y + (i-2)*size/9;
            const int* ptr;
            float vx, vy, w;
            if( (unsigned)y >= (unsigned)sum->rows - size ||
                (unsigned)x >= (unsigned)sum->cols - size )
                continue;
            ptr = sum_ptr + x + y*sum_cols;
            w = G[i+4]*G[j+4];
            vx = icvCalcHaarPattern( ptr, dx_t, NX )*w;
            vy = icvCalcHaarPattern( ptr, dy_t, NX )*w;
            X[nangle] = vx; Y[nangle] = vy;
            nangle++;
        }
        _X.cols = _Y.cols = _angle.cols = nangle;
        cvCartToPolar( &_X, &_Y, 0, &_angle, 1 );

        float bestx = 0, besty = 0, descriptor_mod = 0;
        for( i = 0; i < 360; i += 5 )
        {
            float sumx = 0, sumy = 0, temp_mod;
            for( j = 0; j < nangle; j++ )
            {
                int d = abs(cvRound(angle[j]) - i);
                if( d < 60 || d > 300 )
                {
                    sumx += X[j];
                    sumy += Y[j];
                }
            }
            temp_mod = sumx*sumx + sumy*sumy;
            if( temp_mod > descriptor_mod )
            {
                descriptor_mod = temp_mod;
                bestx = sumx;
                besty = sumy;
            }
        }
        
        float descriptor_dir = cvFastArctan( besty, bestx );
        kp->dir = descriptor_dir;

        if( !_descriptors )
            continue;
        descriptor_dir *= (float)(CV_PI/180);
        
        alpha0 = (float)cos(descriptor_dir);
        beta0 = (float)sin(descriptor_dir);
        sz0 = (float)((PATCH_SZ+1)*size*1.2/9.);
        scale0 = sz0/(PATCH_SZ+1);

        if( sz0 > (PATCH_SZ+1)*1.5f )
        {
            float rd = (float)(sz0*sqrt_2*0.5);
            float alpha1 = (alpha0 - beta0)*sqrt_2*0.5f, beta1 = (alpha0 + beta0)*sqrt_2*0.5f;
            CvRect patch_rect0 = { INT_MAX, INT_MAX, INT_MIN, INT_MIN }, patch_rect, sr_patch_rect;

            for( i = 0; i < 4; i++ )
            {
                float a, b, r = i < 2 ? rd : -rd;
                if( i % 2 == 0 )
                    a = alpha1, b = beta1;
                else
                    a = -beta1, b = alpha1;
                float xf = center.x + r*a;
                float yf = center.y - r*b;
                x = cvFloor(xf); patch_rect0.x = MIN(patch_rect0.x, x);
                y = cvFloor(yf); patch_rect0.y = MIN(patch_rect0.y, y);
                x = cvCeil(xf)+1; patch_rect0.width = MAX(patch_rect0.width, x);
                y = cvCeil(yf)+1; patch_rect0.height = MAX(patch_rect0.height, y);
            }

            patch_rect = patch_rect0;
            patch_rect.x = MAX(patch_rect.x, 0);
            patch_rect.y = MAX(patch_rect.y, 0);
            patch_rect.width = MIN(patch_rect.width, img->width) - patch_rect.x;
            patch_rect.height = MIN(patch_rect.height, img->height) - patch_rect.y;
            patch_rect0.width -= patch_rect0.x;
            patch_rect0.height -= patch_rect0.y;

            CvMat _src0;
            float scale = MIN(1.f,MIN((float)RS_PATCH_SZ/patch_rect0.width,
                (float)RS_PATCH_SZ/patch_rect0.height));
            cvGetSubArr( img, &_src0, patch_rect );
            sr_patch_rect = cvRect(0,0, RS_PATCH_SZ, RS_PATCH_SZ);
            sr_patch_rect.width = cvRound(patch_rect.width*scale);
            sr_patch_rect.height = cvRound(patch_rect.height*scale);
            src = cvGetSubArr( &_rs_patch, &_src, sr_patch_rect );
            cvResize( &_src0, &_src, CV_INTER_AREA );
            center.x = RS_PATCH_SZ*0.5f - (patch_rect.x - patch_rect0.x)*scale;
            center.y = RS_PATCH_SZ*0.5f - (patch_rect.y - patch_rect0.y)*scale;
            scale0 *= scale;
        }
        
        {
        float w[] =
        {
            alpha0*scale0, beta0*scale0, center.x,
            -beta0*scale0, alpha0*scale0, center.y
        };
        CvMat W = cvMat(2, 3, CV_32F, w);
        cvGetQuadrangleSubPix( src, &_patch, &W );
        }

        for( i = 0; i < PATCH_SZ; i++ )
            for( j = 0; j < PATCH_SZ; j++ )
            {
                float dw = DW[i][j];
                float vx = (PATCH[i][j+1] - PATCH[i][j] + PATCH[i+1][j+1] - PATCH[i+1][j])*dw;
                float vy = (PATCH[i+1][j] - PATCH[i][j] + PATCH[i+1][j+1] - PATCH[i][j+1])*dw;
                DX[i][j] = vx;
                DY[i][j] = vy;
            }

        vec = (float*)cvGetSeqElem( descriptors, k );
        for( kk = 0; kk < (int)(descriptors->elem_size/sizeof(vec[0])); kk++ )
            vec[kk] = 0;
        if( params.extended )
        {
            /* 128-bin descriptor */
            for( i = 0; i < 4; i++ )
                for( j = 0; j < 4; j++ )
                {
                    for( y = i*5; y < i*5+5; y++ )
                    {
                        for( x = j*5; x < j*5+5; x++ )
                        {
                            float tx = DX[y][x], ty = DY[y][x];
                            if( ty >= 0 )
                            {
                                vec[0] += tx;
                                vec[1] += (float)fabs(tx);
                            } else {
                                vec[2] += tx;
                                vec[3] += (float)fabs(tx);
                            }
                            if ( tx >= 0 )
                            {
                                vec[4] += ty;
                                vec[5] += (float)fabs(ty);
                            } else {
                                vec[6] += ty;
                                vec[7] += (float)fabs(ty);
                            }
                        }
                    }
                    /* unit vector is essential for contrast invariance */
                    double normalize = 0;
                    for( kk = 0; kk < 8; kk++ )
                        normalize += vec[kk]*vec[kk];
                    normalize = 1./(sqrt(normalize) + DBL_EPSILON);
                    for( kk = 0; kk < 8; kk++ )
                        vec[kk] = (float)(vec[kk]*normalize);
                    vec += 8;
                }
        }
        else
        {
            /* 64-bin descriptor */
            for( i = 0; i < 4; i++ )
                for( j = 0; j < 4; j++ )
                {
                    for( y = i*5; y < i*5+5; y++ )
                    {
                        for( x = j*5; x < j*5+5; x++ )
                        {
                            float tx = DX[y][x], ty = DY[y][x];
                            vec[0] += tx; vec[1] += ty;
                            vec[2] += (float)fabs(tx); vec[3] += (float)fabs(ty);
                        }
                    }
                    double normalize = 0;
                    for( kk = 0; kk < 4; kk++ )
                        normalize += vec[kk]*vec[kk];
                    normalize = 1./(sqrt(normalize) + DBL_EPSILON);
                    for( kk = 0; kk < 4; kk++ )
                        vec[kk] = (float)(vec[kk]*normalize);
                    vec+=4;
                }
        }
    }
    }

    if( _keypoints )
        *_keypoints = keypoints;
    if( _descriptors )
        *_descriptors = descriptors;

    __END__;

    cvReleaseMat( &sum );
    cvReleaseMat( &mask1 );
    cvReleaseMat( &mask_sum );
}
