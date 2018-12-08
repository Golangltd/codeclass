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


CV_IMPL CvRect
cvMaxRect( const CvRect* rect1, const CvRect* rect2 )
{
    if( rect1 && rect2 )
    {
        CvRect max_rect;
        int a, b;

        max_rect.x = a = rect1->x;
        b = rect2->x;
        if( max_rect.x > b )
            max_rect.x = b;

        max_rect.width = a += rect1->width;
        b += rect2->width;

        if( max_rect.width < b )
            max_rect.width = b;
        max_rect.width -= max_rect.x;

        max_rect.y = a = rect1->y;
        b = rect2->y;
        if( max_rect.y > b )
            max_rect.y = b;

        max_rect.height = a += rect1->height;
        b += rect2->height;

        if( max_rect.height < b )
            max_rect.height = b;
        max_rect.height -= max_rect.y;
        return max_rect;
    }
    else if( rect1 )
        return *rect1;
    else if( rect2 )
        return *rect2;
    else
        return cvRect(0,0,0,0);
}


CV_IMPL void
cvBoxPoints( CvBox2D box, CvPoint2D32f pt[4] )
{
    CV_FUNCNAME( "cvBoxPoints" );

    __BEGIN__;
    
    double angle = box.angle*CV_PI/180.;
    float a = (float)cos(angle)*0.5f;
    float b = (float)sin(angle)*0.5f;

    if( !pt )
        CV_ERROR( CV_StsNullPtr, "NULL vertex array pointer" );

    pt[0].x = box.center.x - a*box.size.height - b*box.size.width;
    pt[0].y = box.center.y + b*box.size.height - a*box.size.width;
    pt[1].x = box.center.x + a*box.size.height - b*box.size.width;
    pt[1].y = box.center.y - b*box.size.height - a*box.size.width;
    pt[2].x = 2*box.center.x - pt[0].x;
    pt[2].y = 2*box.center.y - pt[0].y;
    pt[3].x = 2*box.center.x - pt[1].x;
    pt[3].y = 2*box.center.y - pt[1].y;

    __END__;
}


int
icvIntersectLines( double x1, double dx1, double y1, double dy1,
                   double x2, double dx2, double y2, double dy2, double *t2 )
{
    double d = dx1 * dy2 - dx2 * dy1;
    int result = -1;

    if( d != 0 )
    {
        *t2 = ((x2 - x1) * dy1 - (y2 - y1) * dx1) / d;
        result = 0;
    }
    return result;
}


void
icvCreateCenterNormalLine( CvSubdiv2DEdge edge, double *_a, double *_b, double *_c )
{
    CvPoint2D32f org = cvSubdiv2DEdgeOrg( edge )->pt;
    CvPoint2D32f dst = cvSubdiv2DEdgeDst( edge )->pt;

    double a = dst.x - org.x;
    double b = dst.y - org.y;
    double c = -(a * (dst.x + org.x) + b * (dst.y + org.y));

    *_a = a + a;
    *_b = b + b;
    *_c = c;
}


void
icvIntersectLines3( double *a0, double *b0, double *c0,
                    double *a1, double *b1, double *c1, CvPoint2D32f * point )
{
    double det = a0[0] * b1[0] - a1[0] * b0[0];

    if( det != 0 )
    {
        det = 1. / det;
        point->x = (float) ((b0[0] * c1[0] - b1[0] * c0[0]) * det);
        point->y = (float) ((a1[0] * c0[0] - a0[0] * c1[0]) * det);
    }
    else
    {
        point->x = point->y = FLT_MAX;
    }
}


CV_IMPL double
cvPointPolygonTest( const CvArr* _contour, CvPoint2D32f pt, int measure_dist )
{
    double result = 0;
    CV_FUNCNAME( "cvCheckPointPolygon" );

    __BEGIN__;
    
    CvSeqBlock block;
    CvContour header;
    CvSeq* contour = (CvSeq*)_contour;
    CvSeqReader reader;
    int i, total, counter = 0;
    int is_float;
    double min_dist_num = FLT_MAX, min_dist_denom = 1;
    CvPoint ip = {0,0};

    if( !CV_IS_SEQ(contour) )
    {
        CV_CALL( contour = cvPointSeqFromMat( CV_SEQ_KIND_CURVE + CV_SEQ_FLAG_CLOSED,
                                              _contour, &header, &block ));
    }
    else if( CV_IS_SEQ_POLYGON(contour) )
    {
        if( contour->header_size == sizeof(CvContour) && !measure_dist )
        {
            CvRect r = ((CvContour*)contour)->rect;
            if( pt.x < r.x || pt.y < r.y ||
                pt.x >= r.x + r.width || pt.y >= r.y + r.height )
                return -100;
        }
    }
    else if( CV_IS_SEQ_CHAIN(contour) )
    {
        CV_ERROR( CV_StsBadArg,
            "Chains are not supported. Convert them to polygonal representation using cvApproxChains()" );
    }
    else
        CV_ERROR( CV_StsBadArg, "Input contour is neither a valid sequence nor a matrix" );

    total = contour->total;
    is_float = CV_SEQ_ELTYPE(contour) == CV_32FC2;
    cvStartReadSeq( contour, &reader, -1 );

    if( !is_float && !measure_dist && (ip.x = cvRound(pt.x)) == pt.x && (ip.y = cvRound(pt.y)) == pt.y )
    {
        // the fastest "pure integer" branch
        CvPoint v0, v;
        CV_READ_SEQ_ELEM( v, reader );

        for( i = 0; i < total; i++ )
        {
            int dist;
            v0 = v;
            CV_READ_SEQ_ELEM( v, reader );

            if( (v0.y <= ip.y && v.y <= ip.y) ||
                (v0.y > ip.y && v.y > ip.y) ||
                (v0.x < ip.x && v.x < ip.x) )
            {
                if( ip.y == v.y && (ip.x == v.x || (ip.y == v0.y &&
                    ((v0.x <= ip.x && ip.x <= v.x) || (v.x <= ip.x && ip.x <= v0.x)))) )
                    EXIT;
                continue;
            }

            dist = (ip.y - v0.y)*(v.x - v0.x) - (ip.x - v0.x)*(v.y - v0.y);
            if( dist == 0 )
                EXIT;
            if( v.y < v0.y )
                dist = -dist;
            counter += dist > 0;
        }

        result = counter % 2 == 0 ? -100 : 100;
    }
    else
    {
        CvPoint2D32f v0, v;
        CvPoint iv;

        if( is_float )
        {
            CV_READ_SEQ_ELEM( v, reader );
        }
        else
        {
            CV_READ_SEQ_ELEM( iv, reader );
            v = cvPointTo32f( iv );
        }

        if( !measure_dist )
        {
            for( i = 0; i < total; i++ )
            {
                double dist;
                v0 = v;
                if( is_float )
                {
                    CV_READ_SEQ_ELEM( v, reader );
                }
                else
                {
                    CV_READ_SEQ_ELEM( iv, reader );
                    v = cvPointTo32f( iv );
                }

                if( (v0.y <= pt.y && v.y <= pt.y) ||
                    (v0.y > pt.y && v.y > pt.y) ||
                    (v0.x < pt.x && v.x < pt.x) )
                {
                    if( pt.y == v.y && (pt.x == v.x || (pt.y == v0.y &&
                        ((v0.x <= pt.x && pt.x <= v.x) || (v.x <= pt.x && pt.x <= v0.x)))) )
                        EXIT;
                    continue;
                }

                dist = (double)(pt.y - v0.y)*(v.x - v0.x) - (double)(pt.x - v0.x)*(v.y - v0.y);
                if( dist == 0 )
                    EXIT;
                if( v.y < v0.y )
                    dist = -dist;
                counter += dist > 0;
            }

            result = counter % 2 == 0 ? -100 : 100;
        }
        else
        {
            for( i = 0; i < total; i++ )
            {
                double dx, dy, dx1, dy1, dx2, dy2, dist_num, dist_denom = 1;
        
                v0 = v;
                if( is_float )
                {
                    CV_READ_SEQ_ELEM( v, reader );
                }
                else
                {
                    CV_READ_SEQ_ELEM( iv, reader );
                    v = cvPointTo32f( iv );
                }
        
                dx = v.x - v0.x; dy = v.y - v0.y;
                dx1 = pt.x - v0.x; dy1 = pt.y - v0.y;
                dx2 = pt.x - v.x; dy2 = pt.y - v.y;
        
                if( dx1*dx + dy1*dy <= 0 )
                    dist_num = dx1*dx1 + dy1*dy1;
                else if( dx2*dx + dy2*dy >= 0 )
                    dist_num = dx2*dx2 + dy2*dy2;
                else
                {
                    dist_num = (dy1*dx - dx1*dy);
                    dist_num *= dist_num;
                    dist_denom = dx*dx + dy*dy;
                }

                if( dist_num*min_dist_denom < min_dist_num*dist_denom )
                {
                    min_dist_num = dist_num;
                    min_dist_denom = dist_denom;
                    if( min_dist_num == 0 )
                        break;
                }

                if( (v0.y <= pt.y && v.y <= pt.y) ||
                    (v0.y > pt.y && v.y > pt.y) ||
                    (v0.x < pt.x && v.x < pt.x) )
                    continue;

                dist_num = dy1*dx - dx1*dy;
                if( dy < 0 )
                    dist_num = -dist_num;
                counter += dist_num > 0;
            }

            result = sqrt(min_dist_num/min_dist_denom);
            if( counter % 2 == 0 )
                result = -result;
        }
    }

    __END__;

    return result;
}


CV_IMPL void
cvRQDecomp3x3( const CvMat *matrixM, CvMat *matrixR, CvMat *matrixQ,
               CvMat *matrixQx, CvMat *matrixQy, CvMat *matrixQz,
               CvPoint3D64f *eulerAngles)
{
    CV_FUNCNAME("cvRQDecomp3x3");
    __BEGIN__;

    double _M[3][3], _R[3][3], _Q[3][3];
    CvMat M = cvMat(3, 3, CV_64F, _M);
    CvMat R = cvMat(3, 3, CV_64F, _R);
    CvMat Q = cvMat(3, 3, CV_64F, _Q);
    double z, c, s;

    /* Validate parameters. */
    CV_ASSERT( CV_IS_MAT(matrixM) && CV_IS_MAT(matrixR) && CV_IS_MAT(matrixQ) &&
        matrixM->cols == 3 && matrixM->rows == 3 &&
        CV_ARE_SIZES_EQ(matrixM, matrixR) && CV_ARE_SIZES_EQ(matrixM, matrixQ));

    cvConvert(matrixM, &M);

    {
    /* Find Givens rotation Q_x for x axis (left multiplication). */
    /*
         ( 1  0  0 )
    Qx = ( 0  c  s ), c = m33/sqrt(m32^2 + m33^2), s = m32/sqrt(m32^2 + m33^2)
         ( 0 -s  c )
    */
    s = _M[2][1];
    c = _M[2][2];
    z = 1./sqrt(c * c + s * s + DBL_EPSILON);
    c *= z;
    s *= z;

    double _Qx[3][3] = { {1, 0, 0}, {0, c, s}, {0, -s, c} };
    CvMat Qx = cvMat(3, 3, CV_64F, _Qx);

    cvMatMul(&M, &Qx, &R);
    assert(fabs(_R[2][1]) < FLT_EPSILON);
    _R[2][1] = 0;

    /* Find Givens rotation for y axis. */
    /*
         ( c  0  s )
    Qy = ( 0  1  0 ), c = m33/sqrt(m31^2 + m33^2), s = m31/sqrt(m31^2 + m33^2)
         (-s  0  c )
    */
    s = _R[2][0];
    c = _R[2][2];
    z = 1./sqrt(c * c + s * s + DBL_EPSILON);
    c *= z;
    s *= z;

    double _Qy[3][3] = { {c, 0, s}, {0, 1, 0}, {-s, 0, c} };
    CvMat Qy = cvMat(3, 3, CV_64F, _Qy);
    cvMatMul(&R, &Qy, &M);

    assert(fabs(_M[2][0]) < FLT_EPSILON);
    _M[2][0] = 0;

    /* Find Givens rotation for z axis. */
    /*
         ( c  s  0 )
    Qz = (-s  c  0 ), c = m22/sqrt(m21^2 + m22^2), s = m21/sqrt(m21^2 + m22^2)
         ( 0  0  1 )
    */

    s = _M[1][0];
    c = _M[1][1];
    z = 1./sqrt(c * c + s * s + DBL_EPSILON);
    c *= z;
    s *= z;

    double _Qz[3][3] = { {c, s, 0}, {-s, c, 0}, {0, 0, 1} };
    CvMat Qz = cvMat(3, 3, CV_64F, _Qz);

    cvMatMul(&M, &Qz, &R);
    assert(fabs(_R[1][0]) < FLT_EPSILON);
    _R[1][0] = 0;

    // Solve the decomposition ambiguity.
    // Diagonal entries of R, except the last one, shall be positive.
    // Further rotate R by 180 degree if necessary
    if( _R[0][0] < 0 )
    {
        if( _R[1][1] < 0 )
        {
            // rotate around z for 180 degree, i.e. a rotation matrix of
            // [-1,  0,  0],
            // [ 0, -1,  0],
            // [ 0,  0,  1]
            _R[0][0] *= -1;
            _R[0][1] *= -1;
            _R[1][1] *= -1;

            _Qz[0][0] *= -1;
            _Qz[0][1] *= -1;
            _Qz[1][0] *= -1;
            _Qz[1][1] *= -1;
        }
        else
        {
            // rotate around y for 180 degree, i.e. a rotation matrix of
            // [-1,  0,  0],
            // [ 0,  1,  0],
            // [ 0,  0, -1]
            _R[0][0] *= -1;
            _R[0][2] *= -1;
            _R[1][2] *= -1;
            _R[2][2] *= -1;

            cvTranspose( &Qz, &Qz );

            _Qy[0][0] *= -1;
            _Qy[0][2] *= -1;
            _Qy[2][0] *= -1;
            _Qy[2][2] *= -1;
        }
    }
    else if( _R[1][1] < 0 )
    {
        // ??? for some reason, we never get here ???

        // rotate around x for 180 degree, i.e. a rotation matrix of
        // [ 1,  0,  0],
        // [ 0, -1,  0],
        // [ 0,  0, -1]
        _R[0][1] *= -1;
        _R[0][2] *= -1;
        _R[1][1] *= -1;
        _R[1][2] *= -1;
        _R[2][2] *= -1;

        cvTranspose( &Qz, &Qz );
        cvTranspose( &Qy, &Qy );

        _Qx[1][1] *= -1;
        _Qx[1][2] *= -1;
        _Qx[2][1] *= -1;
        _Qx[2][2] *= -1;
    }

    // calculate the euler angle
    if( eulerAngles )
    {
        eulerAngles->x = acos(_Qx[1][1]) * (_Qx[1][2] >= 0 ? 1 : -1) * (180.0 / CV_PI);
        eulerAngles->y = acos(_Qy[0][0]) * (_Qy[0][2] >= 0 ? 1 : -1) * (180.0 / CV_PI);
        eulerAngles->z = acos(_Qz[0][0]) * (_Qz[0][1] >= 0 ? 1 : -1) * (180.0 / CV_PI);
    }

    /* Calulate orthogonal matrix. */
    /*
    Q = QzT * QyT * QxT
    */
    cvGEMM( &Qz, &Qy, 1, 0, 0, &M, CV_GEMM_A_T + CV_GEMM_B_T );
    cvGEMM( &M, &Qx, 1, 0, 0, &Q, CV_GEMM_B_T );

    /* Save R and Q matrices. */
    cvConvert( &R, matrixR );
    cvConvert( &Q, matrixQ );

    if( matrixQx )
        cvConvert(&Qx, matrixQx);
    if( matrixQy )
        cvConvert(&Qy, matrixQy);
    if( matrixQz )
        cvConvert(&Qz, matrixQz);
    }

    __END__;
}


CV_IMPL void
cvDecomposeProjectionMatrix( const CvMat *projMatr, CvMat *calibMatr,
                             CvMat *rotMatr, CvMat *posVect,
                             CvMat *rotMatrX, CvMat *rotMatrY,
                             CvMat *rotMatrZ, CvPoint3D64f *eulerAngles)
{
    CvMat *tmpProjMatr = 0;
    CvMat *tmpMatrixD = 0;
    CvMat *tmpMatrixV = 0;
    CvMat *tmpMatrixM = 0;

    CV_FUNCNAME("cvDecomposeProjectionMatrix");
    __BEGIN__;

    /* Validate parameters. */
    if(projMatr == 0 || calibMatr == 0 || rotMatr == 0 || posVect == 0)
        CV_ERROR(CV_StsNullPtr, "Some of parameters is a NULL pointer!");

    if(!CV_IS_MAT(projMatr) || !CV_IS_MAT(calibMatr) || !CV_IS_MAT(rotMatr) || !CV_IS_MAT(posVect))
        CV_ERROR(CV_StsUnsupportedFormat, "Input parameters must be a matrices!");

    if(projMatr->cols != 4 || projMatr->rows != 3)
        CV_ERROR(CV_StsUnmatchedSizes, "Size of projection matrix must be 3x4!");

    if(calibMatr->cols != 3 || calibMatr->rows != 3 || rotMatr->cols != 3 || rotMatr->rows != 3)
        CV_ERROR(CV_StsUnmatchedSizes, "Size of calibration and rotation matrices must be 3x3!");

    if(posVect->cols != 1 || posVect->rows != 4)
        CV_ERROR(CV_StsUnmatchedSizes, "Size of position vector must be 4x1!");

    CV_CALL(tmpProjMatr = cvCreateMat(4, 4, CV_64F));
    CV_CALL(tmpMatrixD = cvCreateMat(4, 4, CV_64F));
    CV_CALL(tmpMatrixV = cvCreateMat(4, 4, CV_64F));
    CV_CALL(tmpMatrixM = cvCreateMat(3, 3, CV_64F));

    /* Compute position vector. */

    cvSetZero(tmpProjMatr); // Add zero row to make matrix square.
    int i, k;
    for(i = 0; i < 3; i++)
        for(k = 0; k < 4; k++)
            cvmSet(tmpProjMatr, i, k, cvmGet(projMatr, i, k));

    CV_CALL(cvSVD(tmpProjMatr, tmpMatrixD, NULL, tmpMatrixV, CV_SVD_MODIFY_A + CV_SVD_V_T));

    /* Save position vector. */

    for(i = 0; i < 4; i++)
        cvmSet(posVect, i, 0, cvmGet(tmpMatrixV, 3, i)); // Solution is last row of V.

    /* Compute calibration and rotation matrices via RQ decomposition. */

    cvGetCols(projMatr, tmpMatrixM, 0, 3); // M is first square matrix of P.

    assert(cvDet(tmpMatrixM) != 0.0); // So far only finite cameras could be decomposed, so M has to be nonsingular [det(M) != 0].

    CV_CALL(cvRQDecomp3x3(tmpMatrixM, calibMatr, rotMatr, rotMatrX, rotMatrY, rotMatrZ, eulerAngles));

    __END__;

    cvReleaseMat(&tmpProjMatr);
    cvReleaseMat(&tmpMatrixD);
    cvReleaseMat(&tmpMatrixV);
    cvReleaseMat(&tmpMatrixM);
}

/* End of file. */
