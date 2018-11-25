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

/*
    This is stright-forward port v3 of Matlab calibration engine by Jean-Yves Bouguet
    that is (in a large extent) based on the paper:
    Z. Zhang. "A flexible new technique for camera calibration".
    IEEE Transactions on Pattern Analysis and Machine Intelligence, 22(11):1330-1334, 2000.

    The 1st initial port was done by Valery Mosyagin.
*/

CvLevMarq::CvLevMarq()
{
    mask = prevParam = param = J = err = JtJ = JtJN = JtErr = JtJV = JtJW = 0;
    lambdaLg10 = 0; state = DONE;
    criteria = cvTermCriteria(0,0,0);
    iters = 0;
    completeSymmFlag = false;
}

CvLevMarq::CvLevMarq( int nparams, int nerrs, CvTermCriteria criteria0, bool _completeSymmFlag )
{
    mask = prevParam = param = J = err = JtJ = JtJN = JtErr = JtJV = JtJW = 0;
    init(nparams, nerrs, criteria0, _completeSymmFlag);
}

void CvLevMarq::clear()
{
    cvReleaseMat(&mask);
    cvReleaseMat(&prevParam);
    cvReleaseMat(&param);
    cvReleaseMat(&J);
    cvReleaseMat(&err);
    cvReleaseMat(&JtJ);
    cvReleaseMat(&JtJN);
    cvReleaseMat(&JtErr);
    cvReleaseMat(&JtJV);
    cvReleaseMat(&JtJW);
}

CvLevMarq::~CvLevMarq()
{
    clear();
}

void CvLevMarq::init( int nparams, int nerrs, CvTermCriteria criteria0, bool _completeSymmFlag )
{
    if( !param || param->rows != nparams || nerrs != (err ? err->rows : 0) )
        clear();
    mask = cvCreateMat( nparams, 1, CV_8U );
    cvSet(mask, cvScalarAll(1));
    prevParam = cvCreateMat( nparams, 1, CV_64F );
    param = cvCreateMat( nparams, 1, CV_64F );
    JtJ = cvCreateMat( nparams, nparams, CV_64F );
    JtJN = cvCreateMat( nparams, nparams, CV_64F );
    JtJV = cvCreateMat( nparams, nparams, CV_64F );
    JtJW = cvCreateMat( nparams, 1, CV_64F );
    JtErr = cvCreateMat( nparams, 1, CV_64F );
    if( nerrs > 0 )
    {
        J = cvCreateMat( nerrs, nparams, CV_64F );
        err = cvCreateMat( nerrs, 1, CV_64F );
    }
    prevErrNorm = DBL_MAX;
    lambdaLg10 = -3;
    criteria = criteria0;
    if( criteria.type & CV_TERMCRIT_ITER )
        criteria.max_iter = MIN(MAX(criteria.max_iter,1),1000);
    else
        criteria.max_iter = 30;
    if( criteria.type & CV_TERMCRIT_EPS )
        criteria.epsilon = MAX(criteria.epsilon, 0);
    else
        criteria.epsilon = DBL_EPSILON;
    state = STARTED;
    iters = 0;
    completeSymmFlag = _completeSymmFlag;
}

bool CvLevMarq::update( const CvMat*& _param, CvMat*& _J, CvMat*& _err )
{
    double change;

    assert( err != 0 );
    if( state == DONE )
    {
        _param = param;
        return false;
    }

    if( state == STARTED )
    {
        _param = param;
        cvZero( J );
        cvZero( err );
        _J = J;
        _err = err;
        state = CALC_J;
        return true;
    }

    if( state == CALC_J )
    {
        cvMulTransposed( J, JtJ, 1 );
        cvGEMM( J, err, 1, 0, 0, JtErr, CV_GEMM_A_T );
        cvCopy( param, prevParam );
        step();
        if( iters == 0 )
            prevErrNorm = cvNorm(err, 0, CV_L2);
        _param = param;
        cvZero( err );
        _err = err;
        state = CHECK_ERR;
        return true;
    }

    assert( state == CHECK_ERR );
    errNorm = cvNorm( err, 0, CV_L2 );
    if( errNorm > prevErrNorm )
    {
        lambdaLg10++;
        step();
        _param = param;
        cvZero( err );
        _err = err;
        state = CHECK_ERR;
        return true;
    }

    lambdaLg10 = MAX(lambdaLg10-1, -16);
    if( ++iters >= criteria.max_iter ||
        (change = cvNorm(param, prevParam, CV_RELATIVE_L2)) < criteria.epsilon )
    {
        _param = param;
        state = DONE;
        return true;
    }

    prevErrNorm = errNorm;
    _param = param;
    cvZero(J);
    _J = J;
    state = CALC_J;
    return false;
}


bool CvLevMarq::updateAlt( const CvMat*& _param, CvMat*& _JtJ, CvMat*& _JtErr, double*& _errNorm )
{
    double change;

    assert( err == 0 );
    if( state == DONE )
    {
        _param = param;
        return false;
    }

    if( state == STARTED )
    {
        _param = param;
        cvZero( JtJ );
        cvZero( JtErr );
        errNorm = 0;
        _JtJ = JtJ;
        _JtErr = JtErr;
        _errNorm = &errNorm;
        state = CALC_J;
        return true;
    }

    if( state == CALC_J )
    {
        cvCopy( param, prevParam );
        step();
        _param = param;
        prevErrNorm = errNorm;
        errNorm = 0;
        _errNorm = &errNorm;
        state = CHECK_ERR;
        return true;
    }

    assert( state == CHECK_ERR );
    if( errNorm > prevErrNorm )
    {
        lambdaLg10++;
        step();
        _param = param;
        errNorm = 0;
        _errNorm = &errNorm;
        state = CHECK_ERR;
        return true;
    }

    lambdaLg10 = MAX(lambdaLg10-1, -16);
    if( ++iters >= criteria.max_iter ||
        (change = cvNorm(param, prevParam, CV_RELATIVE_L2)) < criteria.epsilon )
    {
        _param = param;
        state = DONE;
        return false;
    }

    prevErrNorm = errNorm;
    cvZero( JtJ );
    cvZero( JtErr );
    _param = param;
    _JtJ = JtJ;
    _JtErr = JtErr;
    state = CALC_J;
    return true;
}

void CvLevMarq::step()
{
    const double LOG10 = log(10.);
    double lambda = exp(lambdaLg10*LOG10);
    int i, j, nparams = param->rows;

    for( i = 0; i < nparams; i++ )
        if( mask->data.ptr[i] == 0 )
        {
            double *row = JtJ->data.db + i*nparams, *col = JtJ->data.db + i;
            for( j = 0; j < nparams; j++ )
                row[j] = col[j*nparams] = 0;
            JtErr->data.db[i] = 0;
        }

    if( !err )
        cvCompleteSymm( JtJ, completeSymmFlag );
    cvSetIdentity( JtJN, cvRealScalar(lambda) );
    cvAdd( JtJ, JtJN, JtJN );
    cvSVD( JtJN, JtJW, 0, JtJV, CV_SVD_MODIFY_A + CV_SVD_U_T + CV_SVD_V_T );
    cvSVBkSb( JtJW, JtJV, JtJV, JtErr, param, CV_SVD_U_T + CV_SVD_V_T );
    for( i = 0; i < nparams; i++ )
        param->data.db[i] = prevParam->data.db[i] - (mask->data.ptr[i] ? param->data.db[i] : 0);
}

// reimplementation of dAB.m
CV_IMPL void
cvCalcMatMulDeriv( const CvMat* A, const CvMat* B, CvMat* dABdA, CvMat* dABdB )
{
    CV_FUNCNAME( "cvCalcMatMulDeriv" );

    __BEGIN__;

    int i, j, M, N, L;
    int bstep;

    CV_ASSERT( CV_IS_MAT(A) && CV_IS_MAT(B) );
    CV_ASSERT( CV_ARE_TYPES_EQ(A, B) &&
        (CV_MAT_TYPE(A->type) == CV_32F || CV_MAT_TYPE(A->type) == CV_64F) );
    CV_ASSERT( A->cols == B->rows );

    M = A->rows;
    L = A->cols;
    N = B->cols;
    bstep = B->step/CV_ELEM_SIZE(B->type);

    if( dABdA )
    {
        CV_ASSERT( CV_ARE_TYPES_EQ(A, dABdA) &&
            dABdA->rows == A->rows*B->cols && dABdA->cols == A->rows*A->cols );
    }

    if( dABdB )
    {
        CV_ASSERT( CV_ARE_TYPES_EQ(A, dABdB) &&
            dABdB->rows == A->rows*B->cols && dABdB->cols == B->rows*B->cols );
    }

    if( CV_MAT_TYPE(A->type) == CV_32F )
    {
        for( i = 0; i < M*N; i++ )
        {
            int i1 = i / N,  i2 = i % N;

            if( dABdA )
            {
                float* dcda = (float*)(dABdA->data.ptr + dABdA->step*i);
                const float* b = (const float*)B->data.ptr + i2;

                for( j = 0; j < M*L; j++ )
                    dcda[j] = 0;
                for( j = 0; j < L; j++ )
                    dcda[i1*L + j] = b[j*bstep];
            }

            if( dABdB )
            {
                float* dcdb = (float*)(dABdB->data.ptr + dABdB->step*i);
                const float* a = (const float*)(A->data.ptr + A->step*i1);

                for( j = 0; j < L*N; j++ )
                    dcdb[j] = 0;
                for( j = 0; j < L; j++ )
                    dcdb[j*N + i2] = a[j];
            }
        }
    }
    else
    {
        for( i = 0; i < M*N; i++ )
        {
            int i1 = i / N,  i2 = i % N;

            if( dABdA )
            {
                double* dcda = (double*)(dABdA->data.ptr + dABdA->step*i);
                const double* b = (const double*)B->data.ptr + i2;

                for( j = 0; j < M*L; j++ )
                    dcda[j] = 0;
                for( j = 0; j < L; j++ )
                    dcda[i1*L + j] = b[j*bstep];
            }

            if( dABdB )
            {
                double* dcdb = (double*)(dABdB->data.ptr + dABdB->step*i);
                const double* a = (const double*)(A->data.ptr + A->step*i1);

                for( j = 0; j < L*N; j++ )
                    dcdb[j] = 0;
                for( j = 0; j < L; j++ )
                    dcdb[j*N + i2] = a[j];
            }
        }
    }

    __END__;
}

// reimplementation of compose_motion.m
CV_IMPL void
cvComposeRT( const CvMat* _rvec1, const CvMat* _tvec1,
             const CvMat* _rvec2, const CvMat* _tvec2,
             CvMat* _rvec3, CvMat* _tvec3,
             CvMat* dr3dr1, CvMat* dr3dt1,
             CvMat* dr3dr2, CvMat* dr3dt2,
             CvMat* dt3dr1, CvMat* dt3dt1,
             CvMat* dt3dr2, CvMat* dt3dt2 )
{
    CV_FUNCNAME( "cvComposeRT" );

    __BEGIN__;

    double _r1[3], _r2[3];
    double _R1[9], _d1[9*3], _R2[9], _d2[9*3];
    CvMat r1 = cvMat(3,1,CV_64F,_r1), r2 = cvMat(3,1,CV_64F,_r2);
    CvMat R1 = cvMat(3,3,CV_64F,_R1), R2 = cvMat(3,3,CV_64F,_R2);
    CvMat dR1dr1 = cvMat(9,3,CV_64F,_d1), dR2dr2 = cvMat(9,3,CV_64F,_d2);

    CV_ASSERT( CV_IS_MAT(_rvec1) && CV_IS_MAT(_rvec2) );

    CV_ASSERT( CV_MAT_TYPE(_rvec1->type) == CV_32F ||
               CV_MAT_TYPE(_rvec1->type) == CV_64F );

    CV_ASSERT( _rvec1->rows == 3 && _rvec1->cols == 1 && CV_ARE_SIZES_EQ(_rvec1, _rvec2) );

    cvConvert( _rvec1, &r1 );
    cvConvert( _rvec2, &r2 );

    cvRodrigues2( &r1, &R1, &dR1dr1 );
    cvRodrigues2( &r2, &R2, &dR2dr2 );

    if( _rvec3 || dr3dr1 || dr3dr1 )
    {
        double _r3[3], _R3[9], _dR3dR1[9*9], _dR3dR2[9*9], _dr3dR3[9*3];
        double _W1[9*3], _W2[3*3];
        CvMat r3 = cvMat(3,1,CV_64F,_r3), R3 = cvMat(3,3,CV_64F,_R3);
        CvMat dR3dR1 = cvMat(9,9,CV_64F,_dR3dR1), dR3dR2 = cvMat(9,9,CV_64F,_dR3dR2);
        CvMat dr3dR3 = cvMat(3,9,CV_64F,_dr3dR3);
        CvMat W1 = cvMat(3,9,CV_64F,_W1), W2 = cvMat(3,3,CV_64F,_W2);

        cvMatMul( &R2, &R1, &R3 );
        cvCalcMatMulDeriv( &R2, &R1, &dR3dR2, &dR3dR1 );

        cvRodrigues2( &R3, &r3, &dr3dR3 );

        if( _rvec3 )
            cvConvert( &r3, _rvec3 );

        if( dr3dr1 )
        {
            cvMatMul( &dr3dR3, &dR3dR1, &W1 );
            cvMatMul( &W1, &dR1dr1, &W2 );
            cvConvert( &W2, dr3dr1 );
        }

        if( dr3dr2 )
        {
            cvMatMul( &dr3dR3, &dR3dR2, &W1 );
            cvMatMul( &W1, &dR2dr2, &W2 );
            cvConvert( &W2, dr3dr2 );
        }
    }

    if( dr3dt1 )
        cvZero( dr3dt1 );
    if( dr3dt2 )
        cvZero( dr3dt2 );

    if( _tvec3 || dt3dr2 || dt3dt1 )
    {
        double _t1[3], _t2[3], _t3[3], _dxdR2[3*9], _dxdt1[3*3], _W3[3*3];
        CvMat t1 = cvMat(3,1,CV_64F,_t1), t2 = cvMat(3,1,CV_64F,_t2);
        CvMat t3 = cvMat(3,1,CV_64F,_t3);
        CvMat dxdR2 = cvMat(3, 9, CV_64F, _dxdR2);
        CvMat dxdt1 = cvMat(3, 3, CV_64F, _dxdt1);
        CvMat W3 = cvMat(3, 3, CV_64F, _W3);

        CV_ASSERT( CV_IS_MAT(_tvec1) && CV_IS_MAT(_tvec2) );
        CV_ASSERT( CV_ARE_SIZES_EQ(_tvec1, _tvec2) && CV_ARE_SIZES_EQ(_tvec1, _rvec1) );

        cvConvert( _tvec1, &t1 );
        cvConvert( _tvec2, &t2 );
        cvMatMulAdd( &R2, &t1, &t2, &t3 );

        if( _tvec3 )
            cvConvert( &t3, _tvec3 );

        if( dt3dr2 || dt3dt1 )
        {
            cvCalcMatMulDeriv( &R2, &t1, &dxdR2, &dxdt1 );
            if( dt3dr2 )
            {
                cvMatMul( &dxdR2, &dR2dr2, &W3 );
                cvConvert( &W3, dt3dr2 );
            }
            if( dt3dt1 )
                cvConvert( &dxdt1, dt3dt1 );
        }
    }

    if( dt3dt2 )
        cvSetIdentity( dt3dt2 );
    if( dt3dr1 )
        cvZero( dt3dr1 );

    __END__;
}

CV_IMPL int
cvRodrigues2( const CvMat* src, CvMat* dst, CvMat* jacobian )
{
    int result = 0;

    CV_FUNCNAME( "cvRogrigues2" );

    __BEGIN__;

    int depth, elem_size;
    int i, k;
    double J[27];
    CvMat _J = cvMat( 3, 9, CV_64F, J );

    if( !CV_IS_MAT(src) )
        CV_ERROR( !src ? CV_StsNullPtr : CV_StsBadArg, "Input argument is not a valid matrix" );

    if( !CV_IS_MAT(dst) )
        CV_ERROR( !dst ? CV_StsNullPtr : CV_StsBadArg,
        "The first output argument is not a valid matrix" );

    depth = CV_MAT_DEPTH(src->type);
    elem_size = CV_ELEM_SIZE(depth);

    if( depth != CV_32F && depth != CV_64F )
        CV_ERROR( CV_StsUnsupportedFormat, "The matrices must have 32f or 64f data type" );

    if( !CV_ARE_DEPTHS_EQ(src, dst) )
        CV_ERROR( CV_StsUnmatchedFormats, "All the matrices must have the same data type" );

    if( jacobian )
    {
        if( !CV_IS_MAT(jacobian) )
            CV_ERROR( CV_StsBadArg, "Jacobian is not a valid matrix" );

        if( !CV_ARE_DEPTHS_EQ(src, jacobian) || CV_MAT_CN(jacobian->type) != 1 )
            CV_ERROR( CV_StsUnmatchedFormats, "Jacobian must have 32fC1 or 64fC1 datatype" );

        if( (jacobian->rows != 9 || jacobian->cols != 3) &&
            (jacobian->rows != 3 || jacobian->cols != 9))
            CV_ERROR( CV_StsBadSize, "Jacobian must be 3x9 or 9x3" );
    }

    if( src->cols == 1 || src->rows == 1 )
    {
        double rx, ry, rz, theta;
        int step = src->rows > 1 ? src->step / elem_size : 1;

        if( src->rows + src->cols*CV_MAT_CN(src->type) - 1 != 3 )
            CV_ERROR( CV_StsBadSize, "Input matrix must be 1x3, 3x1 or 3x3" );

        if( dst->rows != 3 || dst->cols != 3 || CV_MAT_CN(dst->type) != 1 )
            CV_ERROR( CV_StsBadSize, "Output matrix must be 3x3, single-channel floating point matrix" );

        if( depth == CV_32F )
        {
            rx = src->data.fl[0];
            ry = src->data.fl[step];
            rz = src->data.fl[step*2];
        }
        else
        {
            rx = src->data.db[0];
            ry = src->data.db[step];
            rz = src->data.db[step*2];
        }
        theta = sqrt(rx*rx + ry*ry + rz*rz);

        if( theta < DBL_EPSILON )
        {
            cvSetIdentity( dst );

            if( jacobian )
            {
                memset( J, 0, sizeof(J) );
                J[5] = J[15] = J[19] = -1;
                J[7] = J[11] = J[21] = 1;
            }
        }
        else
        {
            const double I[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

            double c = cos(theta);
            double s = sin(theta);
            double c1 = 1. - c;
            double itheta = theta ? 1./theta : 0.;

            rx *= itheta; ry *= itheta; rz *= itheta;

            double rrt[] = { rx*rx, rx*ry, rx*rz, rx*ry, ry*ry, ry*rz, rx*rz, ry*rz, rz*rz };
            double _r_x_[] = { 0, -rz, ry, rz, 0, -rx, -ry, rx, 0 };
            double R[9];
            CvMat _R = cvMat( 3, 3, CV_64F, R );

            // R = cos(theta)*I + (1 - cos(theta))*r*rT + sin(theta)*[r_x]
            // where [r_x] is [0 -rz ry; rz 0 -rx; -ry rx 0]
            for( k = 0; k < 9; k++ )
                R[k] = c*I[k] + c1*rrt[k] + s*_r_x_[k];

            cvConvert( &_R, dst );

            if( jacobian )
            {
                double drrt[] = { rx+rx, ry, rz, ry, 0, 0, rz, 0, 0,
                                  0, rx, 0, rx, ry+ry, rz, 0, rz, 0,
                                  0, 0, rx, 0, 0, ry, rx, ry, rz+rz };
                double d_r_x_[] = { 0, 0, 0, 0, 0, -1, 0, 1, 0,
                                    0, 0, 1, 0, 0, 0, -1, 0, 0,
                                    0, -1, 0, 1, 0, 0, 0, 0, 0 };
                for( i = 0; i < 3; i++ )
                {
                    double ri = i == 0 ? rx : i == 1 ? ry : rz;
                    double a0 = -s*ri, a1 = (s - 2*c1*itheta)*ri, a2 = c1*itheta;
                    double a3 = (c - s*itheta)*ri, a4 = s*itheta;
                    for( k = 0; k < 9; k++ )
                        J[i*9+k] = a0*I[k] + a1*rrt[k] + a2*drrt[i*9+k] +
                                   a3*_r_x_[k] + a4*d_r_x_[i*9+k];
                }
            }
        }
    }
    else if( src->cols == 3 && src->rows == 3 )
    {
        double R[9], U[9], V[9], W[3], rx, ry, rz;
        CvMat _R = cvMat( 3, 3, CV_64F, R );
        CvMat _U = cvMat( 3, 3, CV_64F, U );
        CvMat _V = cvMat( 3, 3, CV_64F, V );
        CvMat _W = cvMat( 3, 1, CV_64F, W );
        double theta, s, c;
        int step = dst->rows > 1 ? dst->step / elem_size : 1;

        if( (dst->rows != 1 || dst->cols*CV_MAT_CN(dst->type) != 3) &&
            (dst->rows != 3 || dst->cols != 1 || CV_MAT_CN(dst->type) != 1))
            CV_ERROR( CV_StsBadSize, "Output matrix must be 1x3 or 3x1" );

        cvConvert( src, &_R );
        if( !cvCheckArr( &_R, CV_CHECK_RANGE+CV_CHECK_QUIET, -100, 100 ) )
        {
            cvZero(dst);
            if( jacobian )
                cvZero(jacobian);
            EXIT;
        }

        cvSVD( &_R, &_W, &_U, &_V, CV_SVD_MODIFY_A + CV_SVD_U_T + CV_SVD_V_T );
        cvGEMM( &_U, &_V, 1, 0, 0, &_R, CV_GEMM_A_T );

        rx = R[7] - R[5];
        ry = R[2] - R[6];
        rz = R[3] - R[1];

        s = sqrt((rx*rx + ry*ry + rz*rz)*0.25);
        c = (R[0] + R[4] + R[8] - 1)*0.5;
        c = c > 1. ? 1. : c < -1. ? -1. : c;
        theta = acos(c);

        if( s < 1e-5 )
        {
            double t;

            if( c > 0 )
                rx = ry = rz = 0;
            else
            {
                t = (R[0] + 1)*0.5;
                rx = theta*sqrt(MAX(t,0.));
                t = (R[4] + 1)*0.5;
                ry = theta*sqrt(MAX(t,0.))*(R[1] < 0 ? -1. : 1.);
                t = (R[8] + 1)*0.5;
                rz = theta*sqrt(MAX(t,0.))*(R[2] < 0 ? -1. : 1.);
            }

            if( jacobian )
            {
                memset( J, 0, sizeof(J) );
                if( c > 0 )
                {
                    J[5] = J[15] = J[19] = -0.5;
                    J[7] = J[11] = J[21] = 0.5;
                }
            }
        }
        else
        {
            double vth = 1/(2*s);

            if( jacobian )
            {
                double t, dtheta_dtr = -1./s;
                // var1 = [vth;theta]
                // var = [om1;var1] = [om1;vth;theta]
                double dvth_dtheta = -vth*c/s;
                double d1 = 0.5*dvth_dtheta*dtheta_dtr;
                double d2 = 0.5*dtheta_dtr;
                // dvar1/dR = dvar1/dtheta*dtheta/dR = [dvth/dtheta; 1] * dtheta/dtr * dtr/dR
                double dvardR[5*9] =
                {
                    0, 0, 0, 0, 0, 1, 0, -1, 0,
                    0, 0, -1, 0, 0, 0, 1, 0, 0,
                    0, 1, 0, -1, 0, 0, 0, 0, 0,
                    d1, 0, 0, 0, d1, 0, 0, 0, d1,
                    d2, 0, 0, 0, d2, 0, 0, 0, d2
                };
                // var2 = [om;theta]
                double dvar2dvar[] =
                {
                    vth, 0, 0, rx, 0,
                    0, vth, 0, ry, 0,
                    0, 0, vth, rz, 0,
                    0, 0, 0, 0, 1
                };
                double domegadvar2[] =
                {
                    theta, 0, 0, rx*vth,
                    0, theta, 0, ry*vth,
                    0, 0, theta, rz*vth
                };

                CvMat _dvardR = cvMat( 5, 9, CV_64FC1, dvardR );
                CvMat _dvar2dvar = cvMat( 4, 5, CV_64FC1, dvar2dvar );
                CvMat _domegadvar2 = cvMat( 3, 4, CV_64FC1, domegadvar2 );
                double t0[3*5];
                CvMat _t0 = cvMat( 3, 5, CV_64FC1, t0 );

                cvMatMul( &_domegadvar2, &_dvar2dvar, &_t0 );
                cvMatMul( &_t0, &_dvardR, &_J );

                // transpose every row of _J (treat the rows as 3x3 matrices)
                CV_SWAP(J[1], J[3], t); CV_SWAP(J[2], J[6], t); CV_SWAP(J[5], J[7], t);
                CV_SWAP(J[10], J[12], t); CV_SWAP(J[11], J[15], t); CV_SWAP(J[14], J[16], t);
                CV_SWAP(J[19], J[21], t); CV_SWAP(J[20], J[24], t); CV_SWAP(J[23], J[25], t);
            }

            vth *= theta;
            rx *= vth; ry *= vth; rz *= vth;
        }

        if( depth == CV_32F )
        {
            dst->data.fl[0] = (float)rx;
            dst->data.fl[step] = (float)ry;
            dst->data.fl[step*2] = (float)rz;
        }
        else
        {
            dst->data.db[0] = rx;
            dst->data.db[step] = ry;
            dst->data.db[step*2] = rz;
        }
    }

    if( jacobian )
    {
        if( depth == CV_32F )
        {
            if( jacobian->rows == _J.rows )
                cvConvert( &_J, jacobian );
            else
            {
                float Jf[3*9];
                CvMat _Jf = cvMat( _J.rows, _J.cols, CV_32FC1, Jf );
                cvConvert( &_J, &_Jf );
                cvTranspose( &_Jf, jacobian );
            }
        }
        else if( jacobian->rows == _J.rows )
            cvCopy( &_J, jacobian );
        else
            cvTranspose( &_J, jacobian );
    }

    result = 1;

    __END__;

    return result;
}


CV_IMPL void
cvProjectPoints2( const CvMat* objectPoints,
                  const CvMat* r_vec,
                  const CvMat* t_vec,
                  const CvMat* A,
                  const CvMat* distCoeffs,
                  CvMat* imagePoints, CvMat* dpdr,
                  CvMat* dpdt, CvMat* dpdf,
                  CvMat* dpdc, CvMat* dpdk,
                  double aspectRatio )
{
    CvMat *_M = 0, *_m = 0;
    CvMat *_dpdr = 0, *_dpdt = 0, *_dpdc = 0, *_dpdf = 0, *_dpdk = 0;

    CV_FUNCNAME( "cvProjectPoints2" );

    __BEGIN__;

    int i, j, count;
    int calc_derivatives;
    const CvPoint3D64f* M;
    CvPoint2D64f* m;
    double r[3], R[9], dRdr[27], t[3], a[9], k[5] = {0,0,0,0,0}, fx, fy, cx, cy;
    CvMat _r, _t, _a = cvMat( 3, 3, CV_64F, a ), _k;
    CvMat _R = cvMat( 3, 3, CV_64F, R ), _dRdr = cvMat( 3, 9, CV_64F, dRdr );
    double *dpdr_p = 0, *dpdt_p = 0, *dpdk_p = 0, *dpdf_p = 0, *dpdc_p = 0;
    int dpdr_step = 0, dpdt_step = 0, dpdk_step = 0, dpdf_step = 0, dpdc_step = 0;
    bool fixedAspectRatio = aspectRatio > FLT_EPSILON;

    if( !CV_IS_MAT(objectPoints) || !CV_IS_MAT(r_vec) ||
        !CV_IS_MAT(t_vec) || !CV_IS_MAT(A) ||
        /*!CV_IS_MAT(distCoeffs) ||*/ !CV_IS_MAT(imagePoints) )
        CV_ERROR( CV_StsBadArg, "One of required arguments is not a valid matrix" );

    count = MAX(objectPoints->rows, objectPoints->cols);

    if( CV_IS_CONT_MAT(objectPoints->type) && CV_MAT_DEPTH(objectPoints->type) == CV_64F &&
        ((objectPoints->rows == 1 && CV_MAT_CN(objectPoints->type) == 3) ||
        (objectPoints->rows == count && CV_MAT_CN(objectPoints->type)*objectPoints->cols == 3)))
        _M = (CvMat*)objectPoints;
    else
    {
        CV_CALL( _M = cvCreateMat( 1, count, CV_64FC3 ));
        CV_CALL( cvConvertPointsHomogeneous( objectPoints, _M ));
    }

    if( CV_IS_CONT_MAT(imagePoints->type) && CV_MAT_DEPTH(imagePoints->type) == CV_64F &&
        ((imagePoints->rows == 1 && CV_MAT_CN(imagePoints->type) == 2) ||
        (imagePoints->rows == count && CV_MAT_CN(imagePoints->type)*imagePoints->cols == 2)))
        _m = imagePoints;
    else
        CV_CALL( _m = cvCreateMat( 1, count, CV_64FC2 ));

    M = (CvPoint3D64f*)_M->data.db;
    m = (CvPoint2D64f*)_m->data.db;

    if( (CV_MAT_DEPTH(r_vec->type) != CV_64F && CV_MAT_DEPTH(r_vec->type) != CV_32F) ||
        (((r_vec->rows != 1 && r_vec->cols != 1) ||
        r_vec->rows*r_vec->cols*CV_MAT_CN(r_vec->type) != 3) &&
        ((r_vec->rows != 3 && r_vec->cols != 3) || CV_MAT_CN(r_vec->type) != 1)))
        CV_ERROR( CV_StsBadArg, "Rotation must be represented by 1x3 or 3x1 "
                  "floating-point rotation vector, or 3x3 rotation matrix" );

    if( r_vec->rows == 3 && r_vec->cols == 3 )
    {
        _r = cvMat( 3, 1, CV_64FC1, r );
        CV_CALL( cvRodrigues2( r_vec, &_r ));
        CV_CALL( cvRodrigues2( &_r, &_R, &_dRdr ));
        cvCopy( r_vec, &_R );
    }
    else
    {
        _r = cvMat( r_vec->rows, r_vec->cols, CV_MAKETYPE(CV_64F,CV_MAT_CN(r_vec->type)), r );
        CV_CALL( cvConvert( r_vec, &_r ));
        CV_CALL( cvRodrigues2( &_r, &_R, &_dRdr ) );
    }

    if( (CV_MAT_DEPTH(t_vec->type) != CV_64F && CV_MAT_DEPTH(t_vec->type) != CV_32F) ||
        (t_vec->rows != 1 && t_vec->cols != 1) ||
        t_vec->rows*t_vec->cols*CV_MAT_CN(t_vec->type) != 3 )
        CV_ERROR( CV_StsBadArg,
            "Translation vector must be 1x3 or 3x1 floating-point vector" );

    _t = cvMat( t_vec->rows, t_vec->cols, CV_MAKETYPE(CV_64F,CV_MAT_CN(t_vec->type)), t );
    CV_CALL( cvConvert( t_vec, &_t ));

    if( (CV_MAT_TYPE(A->type) != CV_64FC1 && CV_MAT_TYPE(A->type) != CV_32FC1) ||
        A->rows != 3 || A->cols != 3 )
        CV_ERROR( CV_StsBadArg, "Instrinsic parameters must be 3x3 floating-point matrix" );

    CV_CALL( cvConvert( A, &_a ));
    fx = a[0]; fy = a[4];
    cx = a[2]; cy = a[5];

    if( fixedAspectRatio )
        fx = fy*aspectRatio;

    if( distCoeffs )
    {
        if( !CV_IS_MAT(distCoeffs) ||
            (CV_MAT_DEPTH(distCoeffs->type) != CV_64F &&
            CV_MAT_DEPTH(distCoeffs->type) != CV_32F) ||
            (distCoeffs->rows != 1 && distCoeffs->cols != 1) ||
            (distCoeffs->rows*distCoeffs->cols*CV_MAT_CN(distCoeffs->type) != 4 &&
            distCoeffs->rows*distCoeffs->cols*CV_MAT_CN(distCoeffs->type) != 5) )
            CV_ERROR( CV_StsBadArg,
                "Distortion coefficients must be 1x4, 4x1, 1x5 or 5x1 floating-point vector" );

        _k = cvMat( distCoeffs->rows, distCoeffs->cols,
                    CV_MAKETYPE(CV_64F,CV_MAT_CN(distCoeffs->type)), k );
        CV_CALL( cvConvert( distCoeffs, &_k ));
    }

    if( dpdr )
    {
        if( !CV_IS_MAT(dpdr) ||
            (CV_MAT_TYPE(dpdr->type) != CV_32FC1 &&
            CV_MAT_TYPE(dpdr->type) != CV_64FC1) ||
            dpdr->rows != count*2 || dpdr->cols != 3 )
            CV_ERROR( CV_StsBadArg, "dp/drot must be 2Nx3 floating-point matrix" );

        if( CV_MAT_TYPE(dpdr->type) == CV_64FC1 )
            _dpdr = dpdr;
        else
            CV_CALL( _dpdr = cvCreateMat( 2*count, 3, CV_64FC1 ));
        dpdr_p = _dpdr->data.db;
        dpdr_step = _dpdr->step/sizeof(dpdr_p[0]);
    }

    if( dpdt )
    {
        if( !CV_IS_MAT(dpdt) ||
            (CV_MAT_TYPE(dpdt->type) != CV_32FC1 &&
            CV_MAT_TYPE(dpdt->type) != CV_64FC1) ||
            dpdt->rows != count*2 || dpdt->cols != 3 )
            CV_ERROR( CV_StsBadArg, "dp/dT must be 2Nx3 floating-point matrix" );

        if( CV_MAT_TYPE(dpdt->type) == CV_64FC1 )
            _dpdt = dpdt;
        else
            CV_CALL( _dpdt = cvCreateMat( 2*count, 3, CV_64FC1 ));
        dpdt_p = _dpdt->data.db;
        dpdt_step = _dpdt->step/sizeof(dpdt_p[0]);
    }

    if( dpdf )
    {
        if( !CV_IS_MAT(dpdf) ||
            (CV_MAT_TYPE(dpdf->type) != CV_32FC1 && CV_MAT_TYPE(dpdf->type) != CV_64FC1) ||
            dpdf->rows != count*2 || dpdf->cols != 2 )
            CV_ERROR( CV_StsBadArg, "dp/df must be 2Nx2 floating-point matrix" );

        if( CV_MAT_TYPE(dpdf->type) == CV_64FC1 )
            _dpdf = dpdf;
        else
            CV_CALL( _dpdf = cvCreateMat( 2*count, 2, CV_64FC1 ));
        dpdf_p = _dpdf->data.db;
        dpdf_step = _dpdf->step/sizeof(dpdf_p[0]);
    }

    if( dpdc )
    {
        if( !CV_IS_MAT(dpdc) ||
            (CV_MAT_TYPE(dpdc->type) != CV_32FC1 && CV_MAT_TYPE(dpdc->type) != CV_64FC1) ||
            dpdc->rows != count*2 || dpdc->cols != 2 )
            CV_ERROR( CV_StsBadArg, "dp/dc must be 2Nx2 floating-point matrix" );

        if( CV_MAT_TYPE(dpdc->type) == CV_64FC1 )
            _dpdc = dpdc;
        else
            CV_CALL( _dpdc = cvCreateMat( 2*count, 2, CV_64FC1 ));
        dpdc_p = _dpdc->data.db;
        dpdc_step = _dpdc->step/sizeof(dpdc_p[0]);
    }

    if( dpdk )
    {
        if( !CV_IS_MAT(dpdk) ||
            (CV_MAT_TYPE(dpdk->type) != CV_32FC1 && CV_MAT_TYPE(dpdk->type) != CV_64FC1) ||
            dpdk->rows != count*2 || (dpdk->cols != 5 && dpdk->cols != 4 && dpdk->cols != 2) )
            CV_ERROR( CV_StsBadArg, "dp/df must be 2Nx5, 2Nx4 or 2Nx2 floating-point matrix" );

        if( !distCoeffs )
            CV_ERROR( CV_StsNullPtr, "distCoeffs is NULL while dpdk is not" );

        if( CV_MAT_TYPE(dpdk->type) == CV_64FC1 )
            _dpdk = dpdk;
        else
            CV_CALL( _dpdk = cvCreateMat( dpdk->rows, dpdk->cols, CV_64FC1 ));
        dpdk_p = _dpdk->data.db;
        dpdk_step = _dpdk->step/sizeof(dpdk_p[0]);
    }

    calc_derivatives = dpdr || dpdt || dpdf || dpdc || dpdk;

    for( i = 0; i < count; i++ )
    {
        double X = M[i].x, Y = M[i].y, Z = M[i].z;
        double x = R[0]*X + R[1]*Y + R[2]*Z + t[0];
        double y = R[3]*X + R[4]*Y + R[5]*Z + t[1];
        double z = R[6]*X + R[7]*Y + R[8]*Z + t[2];
        double r2, r4, r6, a1, a2, a3, cdist;
        double xd, yd;

        z = z ? 1./z : 1;
        x *= z; y *= z;

        r2 = x*x + y*y;
        r4 = r2*r2;
        r6 = r4*r2;
        a1 = 2*x*y;
        a2 = r2 + 2*x*x;
        a3 = r2 + 2*y*y;
        cdist = 1 + k[0]*r2 + k[1]*r4 + k[4]*r6;
        xd = x*cdist + k[2]*a1 + k[3]*a2;
        yd = y*cdist + k[2]*a3 + k[3]*a1;

        m[i].x = xd*fx + cx;
        m[i].y = yd*fy + cy;

        if( calc_derivatives )
        {
            if( dpdc_p )
            {
                dpdc_p[0] = 1; dpdc_p[1] = 0;
                dpdc_p[dpdc_step] = 0;
                dpdc_p[dpdc_step+1] = 1;
                dpdc_p += dpdc_step*2;
            }

            if( dpdf_p )
            {
                if( fixedAspectRatio )
                {
                    dpdf_p[0] = 0; dpdf_p[1] = xd*aspectRatio;
                    dpdf_p[dpdf_step] = 0;
                    dpdf_p[dpdf_step+1] = yd;
                }
                else
                {
                    dpdf_p[0] = xd; dpdf_p[1] = 0;
                    dpdf_p[dpdf_step] = 0;
                    dpdf_p[dpdf_step+1] = yd;
                }
                dpdf_p += dpdf_step*2;
            }

            if( dpdk_p )
            {
                dpdk_p[0] = fx*x*r2;
                dpdk_p[1] = fx*x*r4;
                dpdk_p[dpdk_step] = fy*y*r2;
                dpdk_p[dpdk_step+1] = fy*y*r4;
                if( _dpdk->cols > 2 )
                {
                    dpdk_p[2] = fx*a1;
                    dpdk_p[3] = fx*a2;
                    dpdk_p[dpdk_step+2] = fy*a3;
                    dpdk_p[dpdk_step+3] = fy*a1;
                    if( _dpdk->cols > 4 )
                    {
                        dpdk_p[4] = fx*x*r6;
                        dpdk_p[dpdk_step+4] = fy*y*r6;
                    }
                }
                dpdk_p += dpdk_step*2;
            }

            if( dpdt_p )
            {
                double dxdt[] = { z, 0, -x*z }, dydt[] = { 0, z, -y*z };
                for( j = 0; j < 3; j++ )
                {
                    double dr2dt = 2*x*dxdt[j] + 2*y*dydt[j];
                    double dcdist_dt = k[0]*dr2dt + 2*k[1]*r2*dr2dt + 3*k[4]*r4*dr2dt;
                    double da1dt = 2*(x*dydt[j] + y*dxdt[j]);
                    double dmxdt = fx*(dxdt[j]*cdist + x*dcdist_dt +
                                k[2]*da1dt + k[3]*(dr2dt + 2*x*dxdt[j]));
                    double dmydt = fy*(dydt[j]*cdist + y*dcdist_dt +
                                k[2]*(dr2dt + 2*y*dydt[j]) + k[3]*da1dt);
                    dpdt_p[j] = dmxdt;
                    dpdt_p[dpdt_step+j] = dmydt;
                }
                dpdt_p += dpdt_step*2;
            }

            if( dpdr_p )
            {
                double dx0dr[] =
                {
                    X*dRdr[0] + Y*dRdr[1] + Z*dRdr[2],
                    X*dRdr[9] + Y*dRdr[10] + Z*dRdr[11],
                    X*dRdr[18] + Y*dRdr[19] + Z*dRdr[20]
                };
                double dy0dr[] =
                {
                    X*dRdr[3] + Y*dRdr[4] + Z*dRdr[5],
                    X*dRdr[12] + Y*dRdr[13] + Z*dRdr[14],
                    X*dRdr[21] + Y*dRdr[22] + Z*dRdr[23]
                };
                double dz0dr[] =
                {
                    X*dRdr[6] + Y*dRdr[7] + Z*dRdr[8],
                    X*dRdr[15] + Y*dRdr[16] + Z*dRdr[17],
                    X*dRdr[24] + Y*dRdr[25] + Z*dRdr[26]
                };
                for( j = 0; j < 3; j++ )
                {
                    double dxdr = z*(dx0dr[j] - x*dz0dr[j]);
                    double dydr = z*(dy0dr[j] - y*dz0dr[j]);
                    double dr2dr = 2*x*dxdr + 2*y*dydr;
                    double dcdist_dr = k[0]*dr2dr + 2*k[1]*r2*dr2dr + 3*k[4]*r4*dr2dr;
                    double da1dr = 2*(x*dydr + y*dxdr);
                    double dmxdr = fx*(dxdr*cdist + x*dcdist_dr +
                                k[2]*da1dr + k[3]*(dr2dr + 2*x*dxdr));
                    double dmydr = fy*(dydr*cdist + y*dcdist_dr +
                                k[2]*(dr2dr + 2*y*dydr) + k[3]*da1dr);
                    dpdr_p[j] = dmxdr;
                    dpdr_p[dpdr_step+j] = dmydr;
                }
                dpdr_p += dpdr_step*2;
            }
        }
    }

    if( _m != imagePoints )
        cvConvertPointsHomogeneous( _m, imagePoints );
    if( _dpdr != dpdr )
        cvConvert( _dpdr, dpdr );
    if( _dpdt != dpdt )
        cvConvert( _dpdt, dpdt );
    if( _dpdf != dpdf )
        cvConvert( _dpdf, dpdf );
    if( _dpdc != dpdc )
        cvConvert( _dpdc, dpdc );
    if( _dpdk != dpdk )
        cvConvert( _dpdk, dpdk );

    __END__;

    if( _M != objectPoints )
        cvReleaseMat( &_M );
    if( _m != imagePoints )
        cvReleaseMat( &_m );
    if( _dpdr != dpdr )
        cvReleaseMat( &_dpdr );
    if( _dpdt != dpdt )
        cvReleaseMat( &_dpdt );
    if( _dpdf != dpdf )
        cvReleaseMat( &_dpdf );
    if( _dpdc != dpdc )
        cvReleaseMat( &_dpdc );
    if( _dpdk != dpdk )
        cvReleaseMat( &_dpdk );
}


CV_IMPL void
cvFindExtrinsicCameraParams2( const CvMat* objectPoints,
                  const CvMat* imagePoints, const CvMat* A,
                  const CvMat* distCoeffs,
                  CvMat* rvec, CvMat* tvec )
{
    const int max_iter = 20;
    CvMat *_M = 0, *_Mxy = 0, *_m = 0, *_mn = 0, *_L = 0, *_J = 0;

    CV_FUNCNAME( "cvFindExtrinsicCameraParams2" );

    __BEGIN__;

    int i, count;
    double a[9], ar[9]={1,0,0,0,1,0,0,0,1}, R[9];
    double MM[9], U[9], V[9], W[3];
    CvScalar Mc;
    double JtJ[6*6], JtErr[6], JtJW[6], JtJV[6*6], delta[6], param[6];
    CvMat _A = cvMat( 3, 3, CV_64F, a );
    CvMat _Ar = cvMat( 3, 3, CV_64F, ar );
    CvMat _R = cvMat( 3, 3, CV_64F, R );
    CvMat _r = cvMat( 3, 1, CV_64F, param );
    CvMat _t = cvMat( 3, 1, CV_64F, param + 3 );
    CvMat _Mc = cvMat( 1, 3, CV_64F, Mc.val );
    CvMat _MM = cvMat( 3, 3, CV_64F, MM );
    CvMat _U = cvMat( 3, 3, CV_64F, U );
    CvMat _V = cvMat( 3, 3, CV_64F, V );
    CvMat _W = cvMat( 3, 1, CV_64F, W );
    CvMat _JtJ = cvMat( 6, 6, CV_64F, JtJ );
    CvMat _JtErr = cvMat( 6, 1, CV_64F, JtErr );
    CvMat _JtJW = cvMat( 6, 1, CV_64F, JtJW );
    CvMat _JtJV = cvMat( 6, 6, CV_64F, JtJV );
    CvMat _delta = cvMat( 6, 1, CV_64F, delta );
    CvMat _param = cvMat( 6, 1, CV_64F, param );
    CvMat _dpdr, _dpdt;

    CV_ASSERT( CV_IS_MAT(objectPoints) && CV_IS_MAT(imagePoints) &&
        CV_IS_MAT(A) && CV_IS_MAT(rvec) && CV_IS_MAT(tvec) );

    count = MAX(objectPoints->cols, objectPoints->rows);
    CV_CALL( _M = cvCreateMat( 1, count, CV_64FC3 ));
    CV_CALL( _m = cvCreateMat( 1, count, CV_64FC2 ));

    CV_CALL( cvConvertPointsHomogeneous( objectPoints, _M ));
    CV_CALL( cvConvertPointsHomogeneous( imagePoints, _m ));
    CV_CALL( cvConvert( A, &_A ));

    CV_ASSERT( (CV_MAT_DEPTH(rvec->type) == CV_64F || CV_MAT_DEPTH(rvec->type) == CV_32F) &&
        (rvec->rows == 1 || rvec->cols == 1) && rvec->rows*rvec->cols*CV_MAT_CN(rvec->type) == 3 );

    CV_ASSERT( (CV_MAT_DEPTH(tvec->type) == CV_64F || CV_MAT_DEPTH(tvec->type) == CV_32F) &&
        (tvec->rows == 1 || tvec->cols == 1) && tvec->rows*tvec->cols*CV_MAT_CN(tvec->type) == 3 );

    CV_CALL( _mn = cvCreateMat( 1, count, CV_64FC2 ));
    CV_CALL( _Mxy = cvCreateMat( 1, count, CV_64FC2 ));

    // normalize image points
    // (unapply the intrinsic matrix transformation and distortion)
    cvUndistortPoints( _m, _mn, &_A, distCoeffs, 0, &_Ar );

    Mc = cvAvg(_M);
    cvReshape( _M, _M, 1, count );
    cvMulTransposed( _M, &_MM, 1, &_Mc );
    cvSVD( &_MM, &_W, 0, &_V, CV_SVD_MODIFY_A + CV_SVD_V_T );

    // initialize extrinsic parameters
    if( W[2]/W[1] < 1e-3 || count < 4 )
    {
        // a planar structure case (all M's lie in the same plane)
        double tt[3], h[9], h1_norm, h2_norm;
        CvMat* R_transform = &_V;
        CvMat T_transform = cvMat( 3, 1, CV_64F, tt );
        CvMat _H = cvMat( 3, 3, CV_64F, h );
        CvMat _h1, _h2, _h3;

        if( V[2]*V[2] + V[5]*V[5] < 1e-10 )
            cvSetIdentity( R_transform );

        if( cvDet(R_transform) < 0 )
            cvScale( R_transform, R_transform, -1 );

        cvGEMM( R_transform, &_Mc, -1, 0, 0, &T_transform, CV_GEMM_B_T );

        for( i = 0; i < count; i++ )
        {
            const double* Rp = R_transform->data.db;
            const double* Tp = T_transform.data.db;
            const double* src = _M->data.db + i*3;
            double* dst = _Mxy->data.db + i*2;

            dst[0] = Rp[0]*src[0] + Rp[1]*src[1] + Rp[2]*src[2] + Tp[0];
            dst[1] = Rp[3]*src[0] + Rp[4]*src[1] + Rp[5]*src[2] + Tp[1];
        }

        cvFindHomography( _Mxy, _mn, &_H );

        cvGetCol( &_H, &_h1, 0 );
        _h2 = _h1; _h2.data.db++;
        _h3 = _h2; _h3.data.db++;
        h1_norm = sqrt(h[0]*h[0] + h[3]*h[3] + h[6]*h[6]);
        h2_norm = sqrt(h[1]*h[1] + h[4]*h[4] + h[7]*h[7]);

        cvScale( &_h1, &_h1, 1./h1_norm );
        cvScale( &_h2, &_h2, 1./h2_norm );
        cvScale( &_h3, &_t, 2./(h1_norm + h2_norm));
        cvCrossProduct( &_h1, &_h2, &_h3 );

        cvRodrigues2( &_H, &_r );
        cvRodrigues2( &_r, &_H );
        cvMatMulAdd( &_H, &T_transform, &_t, &_t );
        cvMatMul( &_H, R_transform, &_R );
        cvRodrigues2( &_R, &_r );
    }
    else
    {
        // non-planar structure. Use DLT method
        double* L;
        double LL[12*12], LW[12], LV[12*12], sc;
        CvMat _LL = cvMat( 12, 12, CV_64F, LL );
        CvMat _LW = cvMat( 12, 1, CV_64F, LW );
        CvMat _LV = cvMat( 12, 12, CV_64F, LV );
        CvMat _RRt, _RR, _tt;
        CvPoint3D64f* M = (CvPoint3D64f*)_M->data.db;
        CvPoint2D64f* mn = (CvPoint2D64f*)_mn->data.db;

        CV_CALL( _L = cvCreateMat( 2*count, 12, CV_64F ));
        L = _L->data.db;

        for( i = 0; i < count; i++, L += 24 )
        {
            double x = -mn[i].x, y = -mn[i].y;
            L[0] = L[16] = M[i].x;
            L[1] = L[17] = M[i].y;
            L[2] = L[18] = M[i].z;
            L[3] = L[19] = 1.;
            L[4] = L[5] = L[6] = L[7] = 0.;
            L[12] = L[13] = L[14] = L[15] = 0.;
            L[8] = x*M[i].x;
            L[9] = x*M[i].y;
            L[10] = x*M[i].z;
            L[11] = x;
            L[20] = y*M[i].x;
            L[21] = y*M[i].y;
            L[22] = y*M[i].z;
            L[23] = y;
        }

        cvMulTransposed( _L, &_LL, 1 );
        cvSVD( &_LL, &_LW, 0, &_LV, CV_SVD_MODIFY_A + CV_SVD_V_T );
        _RRt = cvMat( 3, 4, CV_64F, LV + 11*12 );
        cvGetCols( &_RRt, &_RR, 0, 3 );
        cvGetCol( &_RRt, &_tt, 3 );
        if( cvDet(&_RR) < 0 )
            cvScale( &_RRt, &_RRt, -1 );
        sc = cvNorm(&_RR);
        cvSVD( &_RR, &_W, &_U, &_V, CV_SVD_MODIFY_A + CV_SVD_U_T + CV_SVD_V_T );
        cvGEMM( &_U, &_V, 1, 0, 0, &_R, CV_GEMM_A_T );
        cvScale( &_tt, &_t, cvNorm(&_R)/sc );
        cvRodrigues2( &_R, &_r );
        cvReleaseMat( &_L );
    }

    cvReshape( _M, _M, 3, 1 );
    cvReshape( _mn, _mn, 2, 1 );

    CV_CALL( _J = cvCreateMat( 2*count, 6, CV_64FC1 ));
    cvGetCols( _J, &_dpdr, 0, 3 );
    cvGetCols( _J, &_dpdt, 3, 6 );

    // refine extrinsic parameters using iterative algorithm
    for( i = 0; i < max_iter; i++ )
    {
        double n1, n2;
        cvReshape( _mn, _mn, 2, 1 );
        cvProjectPoints2( _M, &_r, &_t, &_A, distCoeffs,
                          _mn, &_dpdr, &_dpdt, 0, 0, 0 );
        cvSub( _m, _mn, _mn );
        cvReshape( _mn, _mn, 1, 2*count );

        cvMulTransposed( _J, &_JtJ, 1 );
        cvGEMM( _J, _mn, 1, 0, 0, &_JtErr, CV_GEMM_A_T );
        cvSVD( &_JtJ, &_JtJW, 0, &_JtJV, CV_SVD_MODIFY_A + CV_SVD_V_T );
        if( JtJW[5]/JtJW[0] < 1e-12 )
            break;
        cvSVBkSb( &_JtJW, &_JtJV, &_JtJV, &_JtErr,
                  &_delta, CV_SVD_U_T + CV_SVD_V_T );
        cvAdd( &_delta, &_param, &_param );
        n1 = cvNorm( &_delta );
        n2 = cvNorm( &_param );
        if( n1/n2 < 1e-10 )
            break;
    }

    _r = cvMat( rvec->rows, rvec->cols,
        CV_MAKETYPE(CV_64F,CV_MAT_CN(rvec->type)), param );
    _t = cvMat( tvec->rows, tvec->cols,
        CV_MAKETYPE(CV_64F,CV_MAT_CN(tvec->type)), param + 3 );

    cvConvert( &_r, rvec );
    cvConvert( &_t, tvec );

    __END__;

    cvReleaseMat( &_M );
    cvReleaseMat( &_Mxy );
    cvReleaseMat( &_m );
    cvReleaseMat( &_mn );
    cvReleaseMat( &_L );
    cvReleaseMat( &_J );
}


CV_IMPL void
cvInitIntrinsicParams2D( const CvMat* objectPoints,
                         const CvMat* imagePoints,
                         const CvMat* npoints,
                         CvSize imageSize,
                         CvMat* cameraMatrix,
                         double aspectRatio )
{
    CvMat *_A = 0, *_b = 0, *_allH = 0, *_allK = 0;

    CV_FUNCNAME( "cvInitIntrinsicParams2D" );

    __BEGIN__;

    int i, j, pos, nimages, total, ni = 0;
    double a[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    double H[9], f[2];
    CvMat _a = cvMat( 3, 3, CV_64F, a );
    CvMat _H = cvMat( 3, 3, CV_64F, H );
    CvMat _f = cvMat( 2, 1, CV_64F, f );

    assert( CV_MAT_TYPE(npoints->type) == CV_32SC1 &&
            CV_IS_MAT_CONT(npoints->type) );
    nimages = npoints->rows + npoints->cols - 1;

    if( (CV_MAT_TYPE(objectPoints->type) != CV_32FC3 &&
        CV_MAT_TYPE(objectPoints->type) != CV_64FC3) ||
        (CV_MAT_TYPE(imagePoints->type) != CV_32FC2 &&
        CV_MAT_TYPE(imagePoints->type) != CV_64FC2) )
        CV_ERROR( CV_StsUnsupportedFormat, "Both object points and image points must be 2D" );

    if( objectPoints->rows != 1 || imagePoints->rows != 1 )
        CV_ERROR( CV_StsBadSize, "object points and image points must be a single-row matrices" );

    _A = cvCreateMat( 2*nimages, 2, CV_64F );
    _b = cvCreateMat( 2*nimages, 1, CV_64F );
    a[2] = (imageSize.width - 1)*0.5;
    a[5] = (imageSize.height - 1)*0.5;
    _allH = cvCreateMat( nimages, 9, CV_64F );

    total = cvRound(cvSum(npoints).val[0]);

    // extract vanishing points in order to obtain initial value for the focal length
    for( i = 0, pos = 0; i < nimages; i++, pos += ni )
    {
        double* Ap = _A->data.db + i*4;
        double* bp = _b->data.db + i*2;
        ni = npoints->data.i[i];
        double h[3], v[3], d1[3], d2[3];
        double n[4] = {0,0,0,0};
        CvMat _m, _M;
        cvGetCols( objectPoints, &_M, pos, pos + ni );
        cvGetCols( imagePoints, &_m, pos, pos + ni );

        cvFindHomography( &_M, &_m, &_H );
        memcpy( _allH->data.db + i*9, H, sizeof(H) );

        H[0] -= H[6]*a[2]; H[1] -= H[7]*a[2]; H[2] -= H[8]*a[2];
        H[3] -= H[6]*a[5]; H[4] -= H[7]*a[5]; H[5] -= H[8]*a[5];

        for( j = 0; j < 3; j++ )
        {
            double t0 = H[j*3], t1 = H[j*3+1];
            h[j] = t0; v[j] = t1;
            d1[j] = (t0 + t1)*0.5;
            d2[j] = (t0 - t1)*0.5;
            n[0] += t0*t0; n[1] += t1*t1;
            n[2] += d1[j]*d1[j]; n[3] += d2[j]*d2[j];
        }

        for( j = 0; j < 4; j++ )
            n[j] = 1./sqrt(n[j]);

        for( j = 0; j < 3; j++ )
        {
            h[j] *= n[0]; v[j] *= n[1];
            d1[j] *= n[2]; d2[j] *= n[3];
        }

        Ap[0] = h[0]*v[0]; Ap[1] = h[1]*v[1];
        Ap[2] = d1[0]*d2[0]; Ap[3] = d1[1]*d2[1];
        bp[0] = -h[2]*v[2]; bp[1] = -d1[2]*d2[2];
    }

    cvSolve( _A, _b, &_f, CV_LSQ | CV_SVD );
    a[0] = sqrt(fabs(1./f[0]));
    a[4] = sqrt(fabs(1./f[1]));
    if( aspectRatio != 0 )
    {
        double tf = (a[0] + a[4])/(aspectRatio + 1.);
        a[0] = aspectRatio*tf;
        a[4] = tf;
    }

    cvConvert( &_a, cameraMatrix );

    __END__;

    cvReleaseMat( &_A );
    cvReleaseMat( &_b );
    cvReleaseMat( &_allH );
    cvReleaseMat( &_allK );
}


/* finds intrinsic and extrinsic camera parameters
   from a few views of known calibration pattern */
CV_IMPL void
cvCalibrateCamera2( const CvMat* objectPoints,
                    const CvMat* imagePoints,
                    const CvMat* npoints,
                    CvSize imageSize,
                    CvMat* cameraMatrix, CvMat* distCoeffs,
                    CvMat* rvecs, CvMat* tvecs,
                    int flags )
{
    const int NINTRINSIC = 9;
    CvMat *_M = 0, *_m = 0, *_Ji = 0, *_Je = 0, *_err = 0;
    CvLevMarq solver;

    CV_FUNCNAME( "cvCalibrateCamera2" );

    __BEGIN__;

    double A[9], k[5] = {0,0,0,0,0};
    CvMat _A = cvMat(3, 3, CV_64F, A), _k;
    int i, nimages, maxPoints = 0, ni = 0, pos, total = 0, nparams, npstep, cn;
    double aspectRatio = 0.;

    // 0. check the parameters & allocate buffers
    if( !CV_IS_MAT(objectPoints) || !CV_IS_MAT(imagePoints) ||
        !CV_IS_MAT(npoints) || !CV_IS_MAT(cameraMatrix) || !CV_IS_MAT(distCoeffs) )
        CV_ERROR( CV_StsBadArg, "One of required vector arguments is not a valid matrix" );

    if( imageSize.width <= 0 || imageSize.height <= 0 )
        CV_ERROR( CV_StsOutOfRange, "image width and height must be positive" );

    if( CV_MAT_TYPE(npoints->type) != CV_32SC1 ||
        (npoints->rows != 1 && npoints->cols != 1) )
        CV_ERROR( CV_StsUnsupportedFormat,
            "the array of point counters must be 1-dimensional integer vector" );

    nimages = npoints->rows*npoints->cols;
    npstep = npoints->rows == 1 ? 1 : npoints->step/CV_ELEM_SIZE(npoints->type);

    if( rvecs )
    {
        cn = CV_MAT_CN(rvecs->type);
        if( !CV_IS_MAT(rvecs) ||
            (CV_MAT_DEPTH(rvecs->type) != CV_32F && CV_MAT_DEPTH(rvecs->type) != CV_64F) ||
            ((rvecs->rows != nimages || (rvecs->cols*cn != 3 && rvecs->cols*cn != 9)) &&
            (rvecs->rows != 1 || rvecs->cols != nimages || cn != 3)) )
            CV_ERROR( CV_StsBadArg, "the output array of rotation vectors must be 3-channel "
                "1xn or nx1 array or 1-channel nx3 or nx9 array, where n is the number of views" );
    }

    if( tvecs )
    {
        cn = CV_MAT_CN(tvecs->type);
        if( !CV_IS_MAT(tvecs) ||
            (CV_MAT_DEPTH(tvecs->type) != CV_32F && CV_MAT_DEPTH(tvecs->type) != CV_64F) ||
            ((tvecs->rows != nimages || tvecs->cols*cn != 3) &&
            (tvecs->rows != 1 || tvecs->cols != nimages || cn != 3)) )
            CV_ERROR( CV_StsBadArg, "the output array of translation vectors must be 3-channel "
                "1xn or nx1 array or 1-channel nx3 array, where n is the number of views" );
    }

    if( (CV_MAT_TYPE(cameraMatrix->type) != CV_32FC1 &&
        CV_MAT_TYPE(cameraMatrix->type) != CV_64FC1) ||
        cameraMatrix->rows != 3 || cameraMatrix->cols != 3 )
        CV_ERROR( CV_StsBadArg,
            "Intrinsic parameters must be 3x3 floating-point matrix" );

    if( (CV_MAT_TYPE(distCoeffs->type) != CV_32FC1 &&
        CV_MAT_TYPE(distCoeffs->type) != CV_64FC1) ||
        (distCoeffs->cols != 1 && distCoeffs->rows != 1) ||
        (distCoeffs->cols*distCoeffs->rows != 4 &&
        distCoeffs->cols*distCoeffs->rows != 5) )
        CV_ERROR( CV_StsBadArg,
            "Distortion coefficients must be 4x1, 1x4, 5x1 or 1x5 floating-point matrix" );

    for( i = 0; i < nimages; i++ )
    {
        ni = npoints->data.i[i*npstep];
        if( ni < 4 )
        {
            char buf[100];
            sprintf( buf, "The number of points in the view #%d is < 4", i );
            CV_ERROR( CV_StsOutOfRange, buf );
        }
        maxPoints = MAX( maxPoints, ni );
        total += ni;
    }

    CV_CALL( _M = cvCreateMat( 1, total, CV_64FC3 ));
    CV_CALL( _m = cvCreateMat( 1, total, CV_64FC2 ));

    CV_CALL( cvConvertPointsHomogeneous( objectPoints, _M ));
    CV_CALL( cvConvertPointsHomogeneous( imagePoints, _m ));

    nparams = NINTRINSIC + nimages*6;
    CV_CALL( _Ji = cvCreateMat( maxPoints*2, NINTRINSIC, CV_64FC1 ));
    CV_CALL( _Je = cvCreateMat( maxPoints*2, 6, CV_64FC1 ));
    CV_CALL( _err = cvCreateMat( maxPoints*2, 1, CV_64FC1 ));
    cvZero( _Ji );

    _k = cvMat( distCoeffs->rows, distCoeffs->cols, CV_MAKETYPE(CV_64F,CV_MAT_CN(distCoeffs->type)), k);
    if( distCoeffs->rows*distCoeffs->cols*CV_MAT_CN(distCoeffs->type) == 4 )
        flags |= CV_CALIB_FIX_K3;

    // 1. initialize intrinsic parameters & LM solver
    if( flags & CV_CALIB_USE_INTRINSIC_GUESS )
    {
        cvConvert( cameraMatrix, &_A );
        if( A[0] <= 0 || A[4] <= 0 )
            CV_ERROR( CV_StsOutOfRange, "Focal length (fx and fy) must be positive" );
        if( A[2] < 0 || A[2] >= imageSize.width ||
            A[5] < 0 || A[5] >= imageSize.height )
            CV_ERROR( CV_StsOutOfRange, "Principal point must be within the image" );
        if( fabs(A[1]) > 1e-5 )
            CV_ERROR( CV_StsOutOfRange, "Non-zero skew is not supported by the function" );
        if( fabs(A[3]) > 1e-5 || fabs(A[6]) > 1e-5 ||
            fabs(A[7]) > 1e-5 || fabs(A[8]-1) > 1e-5 )
            CV_ERROR( CV_StsOutOfRange,
                "The intrinsic matrix must have [fx 0 cx; 0 fy cy; 0 0 1] shape" );
        A[1] = A[3] = A[6] = A[7] = 0.;
        A[8] = 1.;

        if( flags & CV_CALIB_FIX_ASPECT_RATIO )
            aspectRatio = A[0]/A[4];
        cvConvert( distCoeffs, &_k );
    }
    else
    {
        CvScalar mean, sdv;
        cvAvgSdv( _M, &mean, &sdv );
        if( (fabs(mean.val[2]) > 1e-5 && fabs(mean.val[2] - 1) > 1e-5) || fabs(sdv.val[2]) > 1e-5 )
            CV_ERROR( CV_StsBadArg,
            "For non-planar calibration rigs the initial intrinsic matrix must be specified" );
        for( i = 0; i < total; i++ )
            ((CvPoint3D64f*)_M->data.db)[i].z = 0.;

        if( flags & CV_CALIB_FIX_ASPECT_RATIO )
        {
            aspectRatio = cvmGet(cameraMatrix,0,0);
            aspectRatio /= cvmGet(cameraMatrix,1,1);
            if( aspectRatio < 0.01 || aspectRatio > 100 )
                CV_ERROR( CV_StsOutOfRange,
                    "The specified aspect ratio (=A[0][0]/A[1][1]) is incorrect" );
        }
        cvInitIntrinsicParams2D( _M, _m, npoints, imageSize, &_A, aspectRatio );
    }

    solver.init( nparams, 0, cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,30,DBL_EPSILON) );

    {
    double* param = solver.param->data.db;
    uchar* mask = solver.mask->data.ptr;

    param[0] = A[0]; param[1] = A[4]; param[2] = A[2]; param[3] = A[5];
    param[4] = k[0]; param[5] = k[1]; param[6] = k[2]; param[7] = k[3];
    param[8] = k[4];

    if( flags & CV_CALIB_FIX_FOCAL_LENGTH )
        mask[0] = mask[1] = 0;
    if( flags & CV_CALIB_FIX_PRINCIPAL_POINT )
        mask[2] = mask[3] = 0;
    if( flags & CV_CALIB_ZERO_TANGENT_DIST )
    {
        param[6] = param[7] = 0;
        mask[6] = mask[7] = 0;
    }
    if( flags & CV_CALIB_FIX_K1 )
        mask[4] = 0;
    if( flags & CV_CALIB_FIX_K2 )
        mask[5] = 0;
    if( flags & CV_CALIB_FIX_K3 )
        mask[8] = 0;
    }

    // 2. initialize extrinsic parameters
    for( i = 0, pos = 0; i < nimages; i++, pos += ni )
    {
        CvMat _Mi, _mi, _ri, _ti;
        ni = npoints->data.i[i*npstep];

        cvGetRows( solver.param, &_ri, NINTRINSIC + i*6, NINTRINSIC + i*6 + 3 );
        cvGetRows( solver.param, &_ti, NINTRINSIC + i*6 + 3, NINTRINSIC + i*6 + 6 );

        cvGetCols( _M, &_Mi, pos, pos + ni );
        cvGetCols( _m, &_mi, pos, pos + ni );

        cvFindExtrinsicCameraParams2( &_Mi, &_mi, &_A, &_k, &_ri, &_ti );
    }

    // 3. run the optimization
    for(;;)
    {
        const CvMat* _param = 0;
        CvMat *_JtJ = 0, *_JtErr = 0;
        double* _errNorm = 0;
        bool proceed = solver.updateAlt( _param, _JtJ, _JtErr, _errNorm );
        double *param = solver.param->data.db, *pparam = solver.prevParam->data.db;

        if( flags & CV_CALIB_FIX_ASPECT_RATIO )
        {
            param[0] = param[1]*aspectRatio;
            pparam[0] = pparam[1]*aspectRatio;
        }

        A[0] = param[0]; A[4] = param[1];
        A[2] = param[2]; A[5] = param[3];
        k[0] = param[4]; k[1] = param[5]; k[2] = param[6];
        k[3] = param[7];
        k[4] = param[8];

        if( !proceed )
            break;

        for( i = 0, pos = 0; i < nimages; i++, pos += ni )
        {
            CvMat _Mi, _mi, _ri, _ti, _dpdr, _dpdt, _dpdf, _dpdc, _dpdk, _mp, _part;
            ni = npoints->data.i[i*npstep];

            cvGetRows( solver.param, &_ri, NINTRINSIC + i*6, NINTRINSIC + i*6 + 3 );
            cvGetRows( solver.param, &_ti, NINTRINSIC + i*6 + 3, NINTRINSIC + i*6 + 6 );

            cvGetCols( _M, &_Mi, pos, pos + ni );
            cvGetCols( _m, &_mi, pos, pos + ni );

            _Je->rows = _Ji->rows = _err->rows = ni*2;
            cvGetCols( _Je, &_dpdr, 0, 3 );
            cvGetCols( _Je, &_dpdt, 3, 6 );
            cvGetCols( _Ji, &_dpdf, 0, 2 );
            cvGetCols( _Ji, &_dpdc, 2, 4 );
            cvGetCols( _Ji, &_dpdk, 4, NINTRINSIC );
            cvReshape( _err, &_mp, 2, 1 );

            if( _JtJ || _JtErr )
            {
                cvProjectPoints2( &_Mi, &_ri, &_ti, &_A, &_k, &_mp, &_dpdr, &_dpdt,
                                  (flags & CV_CALIB_FIX_FOCAL_LENGTH) ? 0 : &_dpdf,
                                  (flags & CV_CALIB_FIX_PRINCIPAL_POINT) ? 0 : &_dpdc, &_dpdk,
                                  (flags & CV_CALIB_FIX_ASPECT_RATIO) ? aspectRatio : 0);
            }
            else
                cvProjectPoints2( &_Mi, &_ri, &_ti, &_A, &_k, &_mp );

            cvSub( &_mp, &_mi, &_mp );

            if( _JtJ || _JtErr )
            {
                cvGetSubRect( _JtJ, &_part, cvRect(0,0,NINTRINSIC,NINTRINSIC) );
                cvGEMM( _Ji, _Ji, 1, &_part, 1, &_part, CV_GEMM_A_T );

                cvGetSubRect( _JtJ, &_part, cvRect(NINTRINSIC+i*6,NINTRINSIC+i*6,6,6) );
                cvGEMM( _Je, _Je, 1, 0, 0, &_part, CV_GEMM_A_T );

                cvGetSubRect( _JtJ, &_part, cvRect(NINTRINSIC+i*6,0,6,NINTRINSIC) );
                cvGEMM( _Ji, _Je, 1, 0, 0, &_part, CV_GEMM_A_T );

                cvGetRows( _JtErr, &_part, 0, NINTRINSIC );
                cvGEMM( _Ji, _err, 1, &_part, 1, &_part, CV_GEMM_A_T );

                cvGetRows( _JtErr, &_part, NINTRINSIC + i*6, NINTRINSIC + (i+1)*6 );
                cvGEMM( _Je, _err, 1, 0, 0, &_part, CV_GEMM_A_T );
            }

            if( _errNorm )
            {
                double errNorm = cvNorm( &_mp, 0, CV_L2 );
                *_errNorm += errNorm*errNorm;
            }
        }
    }

    // 4. store the results
    cvConvert( &_A, cameraMatrix );
    cvConvert( &_k, distCoeffs );

    for( i = 0; i < nimages; i++ )
    {
        CvMat src, dst;
        if( rvecs )
        {
            src = cvMat( 3, 1, CV_64F, solver.param->data.db + NINTRINSIC + i*6 );
            if( rvecs->rows == nimages && rvecs->cols*CV_MAT_CN(rvecs->type) == 9 )
            {
                dst = cvMat( 3, 3, CV_MAT_DEPTH(rvecs->type),
                    rvecs->data.ptr + rvecs->step*i );
                cvRodrigues2( &src, &_A );
                cvConvert( &_A, &dst );
            }
            else
            {
                dst = cvMat( 3, 1, CV_MAT_DEPTH(rvecs->type), rvecs->rows == 1 ?
                    rvecs->data.ptr + i*CV_ELEM_SIZE(rvecs->type) :
                    rvecs->data.ptr + rvecs->step*i );
                cvConvert( &src, &dst );
            }
        }
        if( tvecs )
        {
            src = cvMat( 3, 1, CV_64F, solver.param->data.db + NINTRINSIC + i*6 + 3 );
            dst = cvMat( 3, 1, CV_MAT_TYPE(tvecs->type), tvecs->rows == 1 ?
                    tvecs->data.ptr + i*CV_ELEM_SIZE(tvecs->type) :
                    tvecs->data.ptr + tvecs->step*i );
            cvConvert( &src, &dst );
         }
    }

    __END__;

    cvReleaseMat( &_M );
    cvReleaseMat( &_m );
    cvReleaseMat( &_Ji );
    cvReleaseMat( &_Je );
    cvReleaseMat( &_err );
}


void cvCalibrationMatrixValues( const CvMat *calibMatr, CvSize imgSize,
    double apertureWidth, double apertureHeight, double *fovx, double *fovy,
    double *focalLength, CvPoint2D64f *principalPoint, double *pasp )
{
    double alphax, alphay, mx, my;
    int imgWidth = imgSize.width, imgHeight = imgSize.height;

    CV_FUNCNAME("cvCalibrationMatrixValues");
    __BEGIN__;

    /* Validate parameters. */

    if(calibMatr == 0)
        CV_ERROR(CV_StsNullPtr, "Some of parameters is a NULL pointer!");

    if(!CV_IS_MAT(calibMatr))
        CV_ERROR(CV_StsUnsupportedFormat, "Input parameters must be a matrices!");

    if(calibMatr->cols != 3 || calibMatr->rows != 3)
        CV_ERROR(CV_StsUnmatchedSizes, "Size of matrices must be 3x3!");

    alphax = cvmGet(calibMatr, 0, 0);
    alphay = cvmGet(calibMatr, 1, 1);
    assert(imgWidth != 0 && imgHeight != 0 && alphax != 0.0 && alphay != 0.0);

    /* Calculate pixel aspect ratio. */
    if(pasp)
        *pasp = alphay / alphax;

    /* Calculate number of pixel per realworld unit. */

    if(apertureWidth != 0.0 && apertureHeight != 0.0) {
        mx = imgWidth / apertureWidth;
        my = imgHeight / apertureHeight;
    } else {
        mx = 1.0;
        my = *pasp;
    }

    /* Calculate fovx and fovy. */

    if(fovx)
        *fovx = 2 * atan(imgWidth / (2 * alphax)) * 180.0 / CV_PI;

    if(fovy)
        *fovy = 2 * atan(imgHeight / (2 * alphay)) * 180.0 / CV_PI;

    /* Calculate focal length. */

    if(focalLength)
        *focalLength = alphax / mx;

    /* Calculate principle point. */

    if(principalPoint)
        *principalPoint = cvPoint2D64f(cvmGet(calibMatr, 0, 2) / mx, cvmGet(calibMatr, 1, 2) / my);

    __END__;
}


//////////////////////////////// Stereo Calibration ///////////////////////////////////

static int dbCmp( const void* _a, const void* _b )
{
    double a = *(const double*)_a;
    double b = *(const double*)_b;

    return (a > b) - (a < b);
}


void cvStereoCalibrate( const CvMat* _objectPoints, const CvMat* _imagePoints1,
                        const CvMat* _imagePoints2, const CvMat* _npoints,
                        CvMat* _cameraMatrix1, CvMat* _distCoeffs1,
                        CvMat* _cameraMatrix2, CvMat* _distCoeffs2,
                        CvSize imageSize, CvMat* _R, CvMat* _T,
                        CvMat* _E, CvMat* _F,
                        CvTermCriteria termCrit, int flags )
{
    const int NINTRINSIC = 9;
    CvMat* npoints = 0;
    CvMat* err = 0;
    CvMat* J_LR = 0;
    CvMat* Je = 0;
    CvMat* Ji = 0;
    CvMat* imagePoints[2] = {0,0};
    CvMat* objectPoints = 0;
    CvMat* RT0 = 0;
    CvLevMarq solver;

    CV_FUNCNAME( "cvStereoCalibrate" );

    __BEGIN__;

    double A[2][9], dk[2][5]={{0,0,0,0,0},{0,0,0,0,0}}, rlr[9];
    CvMat K[2], Dist[2], om_LR, T_LR;
    CvMat R_LR = cvMat(3, 3, CV_64F, rlr);
    int i, k, p, ni = 0, ofs, nimages, pointsTotal, maxPoints = 0;
    int nparams;
    bool recomputeIntrinsics = false;
    double aspectRatio[2] = {0,0};

    CV_ASSERT( CV_IS_MAT(_imagePoints1) && CV_IS_MAT(_imagePoints2) &&
               CV_IS_MAT(_objectPoints) && CV_IS_MAT(_npoints) &&
               CV_IS_MAT(_R) && CV_IS_MAT(_T) );

    CV_ASSERT( CV_ARE_TYPES_EQ(_imagePoints1, _imagePoints2) &&
               CV_ARE_DEPTHS_EQ(_imagePoints1, _objectPoints) );

    CV_ASSERT( (_npoints->cols == 1 || _npoints->rows == 1) &&
               CV_MAT_TYPE(_npoints->type) == CV_32SC1 );

    nimages = _npoints->cols + _npoints->rows - 1;
    npoints = cvCreateMat( _npoints->rows, _npoints->cols, _npoints->type );
    cvCopy( _npoints, npoints );

    for( i = 0, pointsTotal = 0; i < nimages; i++ )
    {
        maxPoints = MAX(maxPoints, npoints->data.i[i]);
        pointsTotal += npoints->data.i[i];
    }

    objectPoints = cvCreateMat( _objectPoints->rows, _objectPoints->cols,
                                CV_64FC(CV_MAT_CN(_objectPoints->type)));
    cvConvert( _objectPoints, objectPoints );
    cvReshape( objectPoints, objectPoints, 3, 1 );

    for( k = 0; k < 2; k++ )
    {
        const CvMat* points = k == 0 ? _imagePoints1 : _imagePoints2;
        const CvMat* cameraMatrix = k == 0 ? _cameraMatrix1 : _cameraMatrix2;
        const CvMat* distCoeffs = k == 0 ? _distCoeffs1 : _distCoeffs2;

        int cn = CV_MAT_CN(_imagePoints1->type);
        CV_ASSERT( (CV_MAT_DEPTH(_imagePoints1->type) == CV_32F ||
                CV_MAT_DEPTH(_imagePoints1->type) == CV_64F) &&
               ((_imagePoints1->rows == pointsTotal && _imagePoints1->cols*cn == 2) ||
                (_imagePoints1->rows == 1 && _imagePoints1->cols == pointsTotal && cn == 2)) );

        K[k] = cvMat(3,3,CV_64F,A[k]);
        Dist[k] = cvMat(1,5,CV_64F,dk[k]);

        imagePoints[k] = cvCreateMat( points->rows, points->cols, CV_64FC(CV_MAT_CN(points->type)));
        cvConvert( points, imagePoints[k] );
        cvReshape( imagePoints[k], imagePoints[k], 2, 1 );

        if( flags & (CV_CALIB_FIX_INTRINSIC|CV_CALIB_USE_INTRINSIC_GUESS|
            CV_CALIB_FIX_ASPECT_RATIO|CV_CALIB_FIX_FOCAL_LENGTH) )
            cvConvert( cameraMatrix, &K[k] );

        if( flags & (CV_CALIB_FIX_INTRINSIC|CV_CALIB_USE_INTRINSIC_GUESS|
            CV_CALIB_FIX_K1|CV_CALIB_FIX_K2|CV_CALIB_FIX_K3) )
        {
            CvMat tdist = cvMat( distCoeffs->rows, distCoeffs->cols,
                CV_MAKETYPE(CV_64F,CV_MAT_CN(distCoeffs->type)), Dist[k].data.db );
            cvConvert( distCoeffs, &tdist );
        }

        if( !(flags & (CV_CALIB_FIX_INTRINSIC|CV_CALIB_USE_INTRINSIC_GUESS)))
        {
            cvCalibrateCamera2( objectPoints, imagePoints[k],
                npoints, imageSize, &K[k], &Dist[k], 0, 0, flags );
        }
    }

    if( flags & CV_CALIB_SAME_FOCAL_LENGTH )
    {
        static const int avg_idx[] = { 0, 4, 2, 5, -1 };
        for( k = 0; avg_idx[k] >= 0; k++ )
            A[0][avg_idx[k]] = A[1][avg_idx[k]] = (A[0][avg_idx[k]] + A[1][avg_idx[k]])*0.5;
    }

    if( flags & CV_CALIB_FIX_ASPECT_RATIO )
    {
        for( k = 0; k < 2; k++ )
            aspectRatio[k] = A[k][0]/A[k][4];
    }

    recomputeIntrinsics = (flags & CV_CALIB_FIX_INTRINSIC) == 0;

    err = cvCreateMat( maxPoints*2, 1, CV_64F );
    Je = cvCreateMat( maxPoints*2, 6, CV_64F );
    J_LR = cvCreateMat( maxPoints*2, 6, CV_64F );
    Ji = cvCreateMat( maxPoints*2, NINTRINSIC, CV_64F );
    cvZero( Ji );

    // we optimize for the inter-camera R(3),t(3), then, optionally,
    // for intrinisic parameters of each camera ((fx,fy,cx,cy,k1,k2,p1,p2) ~ 8 parameters).
    nparams = 6*(nimages+1) + (recomputeIntrinsics ? NINTRINSIC*2 : 0);

    // storage for initial [om(R){i}|t{i}] (in order to compute the median for each component)
    RT0 = cvCreateMat( 6, nimages, CV_64F );

    solver.init( nparams, 0, termCrit );
    if( recomputeIntrinsics )
    {
        uchar* imask = solver.mask->data.ptr + nparams - NINTRINSIC*2;
        if( flags & CV_CALIB_FIX_ASPECT_RATIO )
            imask[0] = imask[NINTRINSIC] = 0;
        if( flags & CV_CALIB_FIX_FOCAL_LENGTH )
            imask[0] = imask[1] = imask[NINTRINSIC] = imask[NINTRINSIC+1] = 0;
        if( flags & CV_CALIB_FIX_PRINCIPAL_POINT )
            imask[2] = imask[3] = imask[NINTRINSIC+2] = imask[NINTRINSIC+3] = 0;
        if( flags & CV_CALIB_ZERO_TANGENT_DIST )
            imask[6] = imask[7] = imask[NINTRINSIC+6] = imask[NINTRINSIC+7] = 0;
        if( flags & CV_CALIB_FIX_K1 )
            imask[4] = imask[NINTRINSIC+4] = 0;
        if( flags & CV_CALIB_FIX_K2 )
            imask[5] = imask[NINTRINSIC+5] = 0;
        if( flags & CV_CALIB_FIX_K3 )
            imask[8] = imask[NINTRINSIC+8] = 0;
    }

    /*
       Compute initial estimate of pose

       For each image, compute:
          R(om) is the rotation matrix of om
          om(R) is the rotation vector of R
          R_ref = R(om_right) * R(om_left)'
          T_ref_list = [T_ref_list; T_right - R_ref * T_left]
          om_ref_list = {om_ref_list; om(R_ref)]

       om = median(om_ref_list)
       T = median(T_ref_list)
    */
    for( i = ofs = 0; i < nimages; ofs += ni, i++ )
    {
        ni = npoints->data.i[i];
        CvMat objpt_i;
        double _om[2][3], r[2][9], t[2][3];
        CvMat om[2], R[2], T[2], imgpt_i[2];

        objpt_i = cvMat(1, ni, CV_64FC3, objectPoints->data.db + ofs*3);
        for( k = 0; k < 2; k++ )
        {
            imgpt_i[k] = cvMat(1, ni, CV_64FC2, imagePoints[k]->data.db + ofs*2);
            om[k] = cvMat(3, 1, CV_64F, _om[k]);
            R[k] = cvMat(3, 3, CV_64F, r[k]);
            T[k] = cvMat(3, 1, CV_64F, t[k]);

            // FIXME: here we ignore activePoints[k] because of
            // the limited API of cvFindExtrnisicCameraParams2
            cvFindExtrinsicCameraParams2( &objpt_i, &imgpt_i[k], &K[k], &Dist[k], &om[k], &T[k] );
            cvRodrigues2( &om[k], &R[k] );
            if( k == 0 )
            {
                // save initial om_left and T_left
                solver.param->data.db[(i+1)*6] = _om[0][0];
                solver.param->data.db[(i+1)*6 + 1] = _om[0][1];
                solver.param->data.db[(i+1)*6 + 2] = _om[0][2];
                solver.param->data.db[(i+1)*6 + 3] = t[0][0];
                solver.param->data.db[(i+1)*6 + 4] = t[0][1];
                solver.param->data.db[(i+1)*6 + 5] = t[0][2];
            }
        }
        cvGEMM( &R[1], &R[0], 1, 0, 0, &R[0], CV_GEMM_B_T );
        cvGEMM( &R[0], &T[0], -1, &T[1], 1, &T[1] );
        cvRodrigues2( &R[0], &T[0] );
        RT0->data.db[i] = t[0][0];
        RT0->data.db[i + nimages] = t[0][1];
        RT0->data.db[i + nimages*2] = t[0][2];
        RT0->data.db[i + nimages*3] = t[1][0];
        RT0->data.db[i + nimages*4] = t[1][1];
        RT0->data.db[i + nimages*5] = t[1][2];
    }

    // find the medians and save the first 6 parameters
    for( i = 0; i < 6; i++ )
    {
        qsort( RT0->data.db + i*nimages, nimages, CV_ELEM_SIZE(RT0->type), dbCmp );
        solver.param->data.db[i] = nimages % 2 != 0 ? RT0->data.db[i*nimages + nimages/2] :
            (RT0->data.db[i*nimages + nimages/2 - 1] + RT0->data.db[i*nimages + nimages/2])*0.5;
    }

    if( recomputeIntrinsics )
        for( k = 0; k < 2; k++ )
        {
            double* iparam = solver.param->data.db + (nimages+1)*6 + k*NINTRINSIC;
            if( flags & CV_CALIB_ZERO_TANGENT_DIST )
                dk[k][2] = dk[k][3] = 0;
            iparam[0] = A[k][0]; iparam[1] = A[k][4]; iparam[2] = A[k][2]; iparam[3] = A[k][5];
            iparam[4] = dk[k][0]; iparam[5] = dk[k][1]; iparam[6] = dk[k][2];
            iparam[7] = dk[k][3]; iparam[8] = dk[k][4];
        }

    om_LR = cvMat(3, 1, CV_64F, solver.param->data.db);
    T_LR = cvMat(3, 1, CV_64F, solver.param->data.db + 3);

    for(;;)
    {
        const CvMat* param = 0;
        CvMat tmpimagePoints;
        CvMat *JtJ = 0, *JtErr = 0;
        double* errNorm = 0;
        double _omR[3], _tR[3];
        double _dr3dr1[9], _dr3dr2[9], /*_dt3dr1[9],*/ _dt3dr2[9], _dt3dt1[9], _dt3dt2[9];
        CvMat dr3dr1 = cvMat(3, 3, CV_64F, _dr3dr1);
        CvMat dr3dr2 = cvMat(3, 3, CV_64F, _dr3dr2);
        //CvMat dt3dr1 = cvMat(3, 3, CV_64F, _dt3dr1);
        CvMat dt3dr2 = cvMat(3, 3, CV_64F, _dt3dr2);
        CvMat dt3dt1 = cvMat(3, 3, CV_64F, _dt3dt1);
        CvMat dt3dt2 = cvMat(3, 3, CV_64F, _dt3dt2);
        CvMat om[2], T[2], imgpt_i[2];
        CvMat dpdrot_hdr, dpdt_hdr, dpdf_hdr, dpdc_hdr, dpdk_hdr;
        CvMat *dpdrot = &dpdrot_hdr, *dpdt = &dpdt_hdr, *dpdf = 0, *dpdc = 0, *dpdk = 0;

        if( !solver.updateAlt( param, JtJ, JtErr, errNorm ))
            break;

        cvRodrigues2( &om_LR, &R_LR );
        om[1] = cvMat(3,1,CV_64F,_omR);
        T[1] = cvMat(3,1,CV_64F,_tR);

        if( recomputeIntrinsics )
        {
            double* iparam = solver.param->data.db + (nimages+1)*6;
            double* ipparam = solver.prevParam->data.db + (nimages+1)*6;
            dpdf = &dpdf_hdr;
            dpdc = &dpdc_hdr;
            dpdk = &dpdk_hdr;
            if( flags & CV_CALIB_SAME_FOCAL_LENGTH )
            {
                iparam[NINTRINSIC] = iparam[0];
                iparam[NINTRINSIC+1] = iparam[1];
                ipparam[NINTRINSIC] = ipparam[0];
                ipparam[NINTRINSIC+1] = ipparam[1];
            }
            if( flags & CV_CALIB_FIX_ASPECT_RATIO )
            {
                iparam[0] = iparam[1]*aspectRatio[0];
                iparam[NINTRINSIC] = iparam[NINTRINSIC+1]*aspectRatio[1];
                ipparam[0] = ipparam[1]*aspectRatio[0];
                ipparam[NINTRINSIC] = ipparam[NINTRINSIC+1]*aspectRatio[1];
            }
            for( k = 0; k < 2; k++ )
            {
                A[k][0] = iparam[k*NINTRINSIC+0];
                A[k][4] = iparam[k*NINTRINSIC+1];
                A[k][2] = iparam[k*NINTRINSIC+2];
                A[k][5] = iparam[k*NINTRINSIC+3];
                dk[k][0] = iparam[k*NINTRINSIC+4];
                dk[k][1] = iparam[k*NINTRINSIC+5];
                dk[k][2] = iparam[k*NINTRINSIC+6];
                dk[k][3] = iparam[k*NINTRINSIC+7];
                dk[k][4] = iparam[k*NINTRINSIC+8];
            }
        }

        for( i = ofs = 0; i < nimages; ofs += ni, i++ )
        {
            ni = npoints->data.i[i];
            CvMat objpt_i, _part;

            om[0] = cvMat(3,1,CV_64F,solver.param->data.db+(i+1)*6);
            T[0] = cvMat(3,1,CV_64F,solver.param->data.db+(i+1)*6+3);

            if( JtJ || JtErr )
                cvComposeRT( &om[0], &T[0], &om_LR, &T_LR, &om[1], &T[1], &dr3dr1, 0,
                             &dr3dr2, 0, 0, &dt3dt1, &dt3dr2, &dt3dt2 );
            else
                cvComposeRT( &om[0], &T[0], &om_LR, &T_LR, &om[1], &T[1] );

            objpt_i = cvMat(1, ni, CV_64FC3, objectPoints->data.db + ofs*3);
            err->rows = Je->rows = J_LR->rows = Ji->rows = ni*2;
            cvReshape( err, &tmpimagePoints, 2, 1 );

            cvGetCols( Ji, &dpdf_hdr, 0, 2 );
            cvGetCols( Ji, &dpdc_hdr, 2, 4 );
            cvGetCols( Ji, &dpdk_hdr, 4, NINTRINSIC );
            cvGetCols( Je, &dpdrot_hdr, 0, 3 );
            cvGetCols( Je, &dpdt_hdr, 3, 6 );

            for( k = 0; k < 2; k++ )
            {
                double maxErr, l2err;
                imgpt_i[k] = cvMat(1, ni, CV_64FC2, imagePoints[k]->data.db + ofs*2);

                if( JtJ || JtErr )
                    cvProjectPoints2( &objpt_i, &om[k], &T[k], &K[k], &Dist[k],
                            &tmpimagePoints, dpdrot, dpdt, dpdf, dpdc, dpdk,
                            (flags & CV_CALIB_FIX_ASPECT_RATIO) ? aspectRatio[k] : 0);
                else
                    cvProjectPoints2( &objpt_i, &om[k], &T[k], &K[k], &Dist[k], &tmpimagePoints );
                cvSub( &tmpimagePoints, &imgpt_i[k], &tmpimagePoints );

                l2err = cvNorm( &tmpimagePoints, 0, CV_L2 );
                maxErr = cvNorm( &tmpimagePoints, 0, CV_C );

                if( JtJ || JtErr )
                {
                    int iofs = (nimages+1)*6 + k*NINTRINSIC, eofs = (i+1)*6;
                    assert( JtJ && JtErr );

                    if( k == 1 )
                    {
                        // d(err_{x|y}R) ~ de3
                        // convert de3/{dr3,dt3} => de3{dr1,dt1} & de3{dr2,dt2}
                        for( p = 0; p < ni*2; p++ )
                        {
                            CvMat de3dr3 = cvMat( 1, 3, CV_64F, Je->data.ptr + Je->step*p );
                            CvMat de3dt3 = cvMat( 1, 3, CV_64F, de3dr3.data.db + 3 );
                            CvMat de3dr2 = cvMat( 1, 3, CV_64F, J_LR->data.ptr + J_LR->step*p );
                            CvMat de3dt2 = cvMat( 1, 3, CV_64F, de3dr2.data.db + 3 );
                            double _de3dr1[3], _de3dt1[3];
                            CvMat de3dr1 = cvMat( 1, 3, CV_64F, _de3dr1 );
                            CvMat de3dt1 = cvMat( 1, 3, CV_64F, _de3dt1 );

                            cvMatMul( &de3dr3, &dr3dr1, &de3dr1 );
                            cvMatMul( &de3dt3, &dt3dt1, &de3dt1 );

                            cvMatMul( &de3dr3, &dr3dr2, &de3dr2 );
                            cvMatMulAdd( &de3dt3, &dt3dr2, &de3dr2, &de3dr2 );

                            cvMatMul( &de3dt3, &dt3dt2, &de3dt2 );

                            cvCopy( &de3dr1, &de3dr3 );
                            cvCopy( &de3dt1, &de3dt3 );
                        }

                        cvGetSubRect( JtJ, &_part, cvRect(0, 0, 6, 6) );
                        cvGEMM( J_LR, J_LR, 1, &_part, 1, &_part, CV_GEMM_A_T );

                        cvGetSubRect( JtJ, &_part, cvRect(eofs, 0, 6, 6) );
                        cvGEMM( J_LR, Je, 1, 0, 0, &_part, CV_GEMM_A_T );

                        cvGetRows( JtErr, &_part, 0, 6 );
                        cvGEMM( J_LR, err, 1, &_part, 1, &_part, CV_GEMM_A_T );
                    }

                    cvGetSubRect( JtJ, &_part, cvRect(eofs, eofs, 6, 6) );
                    cvGEMM( Je, Je, 1, &_part, 1, &_part, CV_GEMM_A_T );

                    cvGetRows( JtErr, &_part, eofs, eofs + 6 );
                    cvGEMM( Je, err, 1, &_part, 1, &_part, CV_GEMM_A_T );

                    if( recomputeIntrinsics )
                    {
                        cvGetSubRect( JtJ, &_part, cvRect(iofs, iofs, NINTRINSIC, NINTRINSIC) );
                        cvGEMM( Ji, Ji, 1, &_part, 1, &_part, CV_GEMM_A_T );
                        cvGetSubRect( JtJ, &_part, cvRect(iofs, eofs, NINTRINSIC, 6) );
                        cvGEMM( Je, Ji, 1, &_part, 1, &_part, CV_GEMM_A_T );
                        if( k == 1 )
                        {
                            cvGetSubRect( JtJ, &_part, cvRect(iofs, 0, NINTRINSIC, 6) );
                            cvGEMM( J_LR, Ji, 1, &_part, 1, &_part, CV_GEMM_A_T );
                        }
                        cvGetRows( JtErr, &_part, iofs, iofs + NINTRINSIC );
                        cvGEMM( Ji, err, 1, &_part, 1, &_part, CV_GEMM_A_T );
                    }
                }

                if( errNorm )
                    *errNorm += l2err*l2err;
            }
        }
    }

    cvRodrigues2( &om_LR, &R_LR );
    if( _R->rows == 1 || _R->cols == 1 )
        cvConvert( &om_LR, _R );
    else
        cvConvert( &R_LR, _R );
    cvConvert( &T_LR, _T );

    if( recomputeIntrinsics )
    {
        cvConvert( &K[0], _cameraMatrix1 );
        cvConvert( &K[1], _cameraMatrix2 );

        for( k = 0; k < 2; k++ )
        {
            CvMat* distCoeffs = k == 0 ? _distCoeffs1 : _distCoeffs2;
            CvMat tdist = cvMat( distCoeffs->rows, distCoeffs->cols,
                CV_MAKETYPE(CV_64F,CV_MAT_CN(distCoeffs->type)), Dist[k].data.db );
            cvConvert( &tdist, distCoeffs );
        }
    }

    if( _E || _F )
    {
        double* t = T_LR.data.db;
        double tx[] =
        {
            0, -t[2], t[1],
            t[2], 0, -t[0],
            -t[1], t[0], 0
        };
        CvMat Tx = cvMat(3, 3, CV_64F, tx);
        double e[9], f[9];
        CvMat E = cvMat(3, 3, CV_64F, e);
        CvMat F = cvMat(3, 3, CV_64F, f);
        cvMatMul( &Tx, &R_LR, &E );
        if( _E )
            cvConvert( &E, _E );
        if( _F )
        {
            double ik[9];
            CvMat iK = cvMat(3, 3, CV_64F, ik);
            cvInvert(&K[1], &iK);
            cvGEMM( &iK, &E, 1, 0, 0, &E, CV_GEMM_A_T );
            cvInvert(&K[0], &iK);
            cvMatMul(&E, &iK, &F);
            cvConvertScale( &F, _F, fabs(f[8]) > 0 ? 1./f[8] : 1 );
        }
    }

    __END__;

    cvReleaseMat( &npoints );
    cvReleaseMat( &err );
    cvReleaseMat( &J_LR );
    cvReleaseMat( &Je );
    cvReleaseMat( &Ji );
    cvReleaseMat( &RT0 );
    cvReleaseMat( &objectPoints );
    cvReleaseMat( &imagePoints[0] );
    cvReleaseMat( &imagePoints[1] );
}


void cvStereoRectify( const CvMat* _cameraMatrix1, const CvMat* _cameraMatrix2,
                      const CvMat* _distCoeffs1, const CvMat* _distCoeffs2,
                      CvSize imageSize, const CvMat* _R, const CvMat* _T,
                      CvMat* _R1, CvMat* _R2, CvMat* _P1, CvMat* _P2,
                      CvMat* _Q, int flags )
{
    double _om[3], _t[3], _uu[3]={0,0,0}, _r_r[3][3], _pp[3][4];
    double _ww[3], _wr[3][3], _z[3] = {0,0,0}, _ri[3][3];
    CvMat om  = cvMat(3, 1, CV_64F, _om);
    CvMat t   = cvMat(3, 1, CV_64F, _t);
    CvMat uu  = cvMat(3, 1, CV_64F, _uu);
    CvMat r_r = cvMat(3, 3, CV_64F, _r_r);
    CvMat pp  = cvMat(3, 4, CV_64F, _pp);
    CvMat ww  = cvMat(3, 1, CV_64F, _ww); // temps
    CvMat wR  = cvMat(3, 3, CV_64F, _wr);
    CvMat Z   = cvMat(3, 1, CV_64F, _z);
    CvMat Ri  = cvMat(3, 3, CV_64F, _ri);
    double nx = imageSize.width, ny = imageSize.height;
    int i, k;

    if( _R->rows == 3 && _R->cols == 3 )
        cvRodrigues2(_R, &om);          // get vector rotation
    else
        cvConvert(_R, &om); // it's already a rotation vector
    cvConvertScale(&om, &om, -0.5); // get average rotation
    cvRodrigues2(&om, &r_r);        // rotate cameras to same orientation by averaging
    cvMatMul(&r_r, _T, &t);

    int idx = fabs(_t[0]) > fabs(_t[1]) ? 0 : 1;
    double c = _t[idx], nt = cvNorm(&t, 0, CV_L2);
    _uu[idx] = c > 0 ? 1 : -1;

    // calculate global Z rotation
    cvCrossProduct(&t,&uu,&ww);
    double nw = cvNorm(&ww, 0, CV_L2);
    cvConvertScale(&ww, &ww, acos(fabs(c)/nt)/nw);
    cvRodrigues2(&ww, &wR);

    // apply to both views
    cvGEMM(&wR, &r_r, 1, 0, 0, &Ri, CV_GEMM_B_T);
    cvConvert( &Ri, _R1 );
    cvGEMM(&wR, &r_r, 1, 0, 0, &Ri, 0);
    cvConvert( &Ri, _R2 );
    cvMatMul(&r_r, _T, &t);

    // calculate projection/camera matrices
    // these contain the relevant rectified image internal params (fx, fy=fx, cx, cy)
    double fc_new = DBL_MAX;
    CvPoint2D64f cc_new[2] = {{0,0}, {0,0}};

    for( k = 0; k < 2; k++ )
    {
        const CvMat* A = k == 0 ? _cameraMatrix1 : _cameraMatrix2;
        const CvMat* Dk = k == 0 ? _distCoeffs1 : _distCoeffs2;
        CvPoint2D32f _pts[4];
        CvPoint3D32f _pts_3[4];
        CvMat pts = cvMat(1, 4, CV_32FC2, _pts);
        CvMat pts_3 = cvMat(1, 4, CV_32FC3, _pts_3);
        double fc, dk1 = Dk ? cvmGet(Dk, 0, 0) : 0;

        fc = cvmGet(A,idx^1,idx^1);
        if( dk1 < 0 )
            fc *= 1 + 0.2*dk1*(nx*nx + ny*ny)/(8*fc*fc);
        fc_new = MIN(fc_new, fc);

        for( i = 0; i < 4; i++ )
        {
            _pts[i].x = (float)(((i % 2) + 0.5)*nx*0.5);
            _pts[i].y = (float)(((i / 2) + 0.5)*ny*0.5);
        }
        cvUndistortPoints( &pts, &pts, A, Dk, 0, 0 );
        cvConvertPointsHomogeneous( &pts, &pts_3 );
        cvProjectPoints2( &pts_3, k == 0 ? _R1 : _R2, &Z, A, 0, &pts );
        CvScalar avg = cvAvg(&pts);
        cc_new[k].x = avg.val[0];
        cc_new[k].y = avg.val[1];
    }

    // vertical focal length must be the same for both images to keep the epipolar constraint
    // (for horizontal epipolar lines -- TBD: check for vertical epipolar lines)
    // use fy for fx also, for simplicity

    // For simplicity, set the principal points for both cameras to be the average
    // of the two principal points (either one of or both x- and y- coordinates)
    if( flags & CV_CALIB_ZERO_DISPARITY )
    {
        cc_new[0].x = cc_new[1].x = (cc_new[0].x + cc_new[1].x)*0.5;
        cc_new[0].y = cc_new[1].y = (cc_new[0].y + cc_new[1].y)*0.5;
    }
    else if( idx == 0 ) // horizontal stereo
        cc_new[0].y = cc_new[1].y = (cc_new[0].y + cc_new[1].y)*0.5;
    else // vertical stereo
        cc_new[0].x = cc_new[1].x = (cc_new[0].x + cc_new[1].x)*0.5;

    cvZero( &pp );
    _pp[0][0] = _pp[1][1] = fc_new;
    _pp[0][2] = cc_new[0].x;
    _pp[1][2] = cc_new[0].y;
    _pp[2][2] = 1;
    cvConvert(&pp, _P1);

    _pp[0][2] = cc_new[1].x;
    _pp[1][2] = cc_new[1].y;
    _pp[idx][3] = _t[idx]*fc_new; // baseline * focal length
    cvConvert(&pp, _P2);

    if( _Q )
    {
        double q[] =
        {
            1, 0, 0, -cc_new[0].x,
            0, 1, 0, -cc_new[0].y,
            0, 0, 0, fc_new,
            0, 0, 1./_t[idx],
            (idx == 0 ? cc_new[0].x - cc_new[1].x : cc_new[0].y - cc_new[1].y)/_t[idx]
        };
        CvMat Q = cvMat(4, 4, CV_64F, q);
        cvConvert( &Q, _Q );
    }
}


CV_IMPL int
cvStereoRectifyUncalibrated(
    const CvMat* _points1, const CvMat* _points2,
    const CvMat* F0, CvSize imgSize, CvMat* _H1, CvMat* _H2, double threshold )
{
    int result = 0;
    CvMat* _m1 = 0;
    CvMat* _m2 = 0;
    CvMat* _lines1 = 0;
    CvMat* _lines2 = 0;

    CV_FUNCNAME( "cvStereoCalcHomographiesFromF" );

    __BEGIN__;

    int i, j, npoints;
    double cx, cy;
    double u[9], v[9], w[9], f[9], h1[9], h2[9], h0[9], e2[3];
    CvMat E2 = cvMat( 3, 1, CV_64F, e2 );
    CvMat U = cvMat( 3, 3, CV_64F, u );
    CvMat V = cvMat( 3, 3, CV_64F, v );
    CvMat W = cvMat( 3, 3, CV_64F, w );
    CvMat F = cvMat( 3, 3, CV_64F, f );
    CvMat H1 = cvMat( 3, 3, CV_64F, h1 );
    CvMat H2 = cvMat( 3, 3, CV_64F, h2 );
    CvMat H0 = cvMat( 3, 3, CV_64F, h0 );

    CvPoint2D64f* m1;
    CvPoint2D64f* m2;
    CvPoint3D64f* lines1;
    CvPoint3D64f* lines2;

    CV_ASSERT( CV_IS_MAT(_points1) && CV_IS_MAT(_points2) &&
        (_points1->rows == 1 || _points1->cols == 1) &&
        (_points2->rows == 1 || _points2->cols == 1) &&
        CV_ARE_SIZES_EQ(_points1, _points2) );

    npoints = _points1->rows * _points1->cols * CV_MAT_CN(_points1->type) / 2;

    _m1 = cvCreateMat( _points1->rows, _points1->cols, CV_64FC(CV_MAT_CN(_points1->type)) );
    _m2 = cvCreateMat( _points2->rows, _points2->cols, CV_64FC(CV_MAT_CN(_points2->type)) );
    _lines1 = cvCreateMat( 1, npoints, CV_64FC3 );
    _lines2 = cvCreateMat( 1, npoints, CV_64FC3 );

    cvConvert( F0, &F );

    cvSVD( (CvMat*)&F, &W, &U, &V, CV_SVD_U_T + CV_SVD_V_T );
    W.data.db[8] = 0.;
    cvGEMM( &U, &W, 1, 0, 0, &W, CV_GEMM_A_T );
    cvMatMul( &W, &V, &F );

    cx = cvRound( (imgSize.width-1)*0.5 );
    cy = cvRound( (imgSize.height-1)*0.5 );

    cvZero( _H1 );
    cvZero( _H2 );

    cvConvert( _points1, _m1 );
    cvConvert( _points2, _m2 );
    cvReshape( _m1, _m1, 2, 1 );
    cvReshape( _m1, _m1, 2, 1 );

    m1 = (CvPoint2D64f*)_m1->data.ptr;
    m2 = (CvPoint2D64f*)_m2->data.ptr;
    lines1 = (CvPoint3D64f*)_lines1->data.ptr;
    lines2 = (CvPoint3D64f*)_lines2->data.ptr;

    if( threshold > 0 )
    {
        cvComputeCorrespondEpilines( _m1, 1, &F, _lines1 );
        cvComputeCorrespondEpilines( _m2, 2, &F, _lines2 );

        // measure distance from points to the corresponding epilines, mark outliers
        for( i = j = 0; i < npoints; i++ )
        {
            if( fabs(m1[i].x*lines2[i].x +
                     m1[i].y*lines2[i].y +
                     lines2[i].z) <= threshold &&
                fabs(m2[i].x*lines1[i].x +
                     m2[i].y*lines1[i].y +
                     lines1[i].z) <= threshold )
            {
                if( j > i )
                {
                    m1[j] = m1[i];
                    m2[j] = m2[i];
                }
                j++;
            }
        }

        npoints = j;
        if( npoints == 0 )
            EXIT;
    }

    {
    _m1->cols = _m2->cols = npoints;
    memcpy( E2.data.db, U.data.db + 6, sizeof(e2));
    cvScale( &E2, &E2, e2[2] > 0 ? 1 : -1 );

    double t[] =
    {
        1, 0, -cx,
        0, 1, -cy,
        0, 0, 1
    };
    CvMat T = cvMat(3, 3, CV_64F, t);
    cvMatMul( &T, &E2, &E2 );

    int mirror = e2[0] < 0;
    double d = MAX(sqrt(e2[0]*e2[0] + e2[1]*e2[1]),DBL_EPSILON);
    double alpha = e2[0]/d;
    double beta = e2[1]/d;
    double r[] =
    {
        alpha, beta, 0,
        -beta, alpha, 0,
        0, 0, 1
    };
    CvMat R = cvMat(3, 3, CV_64F, r);
    cvMatMul( &R, &T, &T );
    cvMatMul( &R, &E2, &E2 );
    double invf = fabs(e2[2]) < 1e-6*fabs(e2[0]) ? 0 : -e2[2]/e2[0];
    double k[] =
    {
        1, 0, 0,
        0, 1, 0,
        invf, 0, 1
    };
    CvMat K = cvMat(3, 3, CV_64F, k);
    cvMatMul( &K, &T, &H2 );
    cvMatMul( &K, &E2, &E2 );

    double it[] =
    {
        1, 0, cx,
        0, 1, cy,
        0, 0, 1
    };
    CvMat iT = cvMat( 3, 3, CV_64F, it );
    cvMatMul( &iT, &H2, &H2 );

    memcpy( E2.data.db, U.data.db + 6, sizeof(e2));
    cvScale( &E2, &E2, e2[2] > 0 ? 1 : -1 );

    double e2_x[] =
    {
        0, -e2[2], e2[1],
       e2[2], 0, -e2[0],
       -e2[1], e2[0], 0
    };
    double e2_111[] =
    {
        e2[0], e2[0], e2[0],
        e2[1], e2[1], e2[1],
        e2[2], e2[2], e2[2],
    };
    CvMat E2_x = cvMat(3, 3, CV_64F, e2_x);
    CvMat E2_111 = cvMat(3, 3, CV_64F, e2_111);
    cvMatMulAdd(&E2_x, &F, &E2_111, &H0 );
    cvMatMul(&H2, &H0, &H0);
    CvMat E1=cvMat(3, 1, CV_64F, V.data.db+6);
    cvMatMul(&H0, &E1, &E1);

    cvPerspectiveTransform( _m1, _m1, &H0 );
    cvPerspectiveTransform( _m2, _m2, &H2 );
    CvMat A = cvMat( 1, npoints, CV_64FC3, lines1 ), BxBy, B;
    double a[9], atb[3], x[3];
    CvMat AtA = cvMat( 3, 3, CV_64F, a );
    CvMat AtB = cvMat( 3, 1, CV_64F, atb );
    CvMat X = cvMat( 3, 1, CV_64F, x );
    cvConvertPointsHomogeneous( _m1, &A );
    cvReshape( &A, &A, 1, npoints );
    cvReshape( _m2, &BxBy, 1, npoints );
    cvGetCol( &BxBy, &B, 0 );
    cvGEMM( &A, &A, 1, 0, 0, &AtA, CV_GEMM_A_T );
    cvGEMM( &A, &B, 1, 0, 0, &AtB, CV_GEMM_A_T );
    cvSolve( &AtA, &AtB, &X, CV_SVD_SYM );

    double ha[] =
    {
        x[0], x[1], x[2],
        0, 1, 0,
        0, 0, 1
    };
    CvMat Ha = cvMat(3, 3, CV_64F, ha);
    cvMatMul( &Ha, &H0, &H1 );
    cvPerspectiveTransform( _m1, _m1, &Ha );

    if( mirror )
    {
        double mm[] = { -1, 0, cx*2, 0, -1, cy*2, 0, 0, 1 };
        CvMat MM = cvMat(3, 3, CV_64F, mm);
        cvMatMul( &MM, &H1, &H1 );
        cvMatMul( &MM, &H2, &H2 );
    }

    cvConvert( &H1, _H1 );
    cvConvert( &H2, _H2 );

    result = 1;
    }

    __END__;

    cvReleaseMat( &_m1 );
    cvReleaseMat( &_m2 );
    cvReleaseMat( &_lines1 );
    cvReleaseMat( &_lines2 );

    return result;
}


CV_IMPL void
cvReprojectImageTo3D(
    const CvArr* disparityImage,
    CvArr* _3dImage, const CvMat* _Q )
{
    CV_FUNCNAME( "cvReprojectImageTo3D" );

    __BEGIN__;

    double q[4][4];
    CvMat Q = cvMat(4, 4, CV_64F, q);
    CvMat sstub, *src = cvGetMat( disparityImage, &sstub );
    CvMat dstub, *dst = cvGetMat( _3dImage, &dstub );
    int stype = CV_MAT_TYPE(src->type), dtype = CV_MAT_TYPE(dst->type);
    int x, y, rows = src->rows, cols = src->cols;
    float* sbuf = (float*)cvStackAlloc( cols*sizeof(sbuf[0]) );
    float* dbuf = (float*)cvStackAlloc( cols*3*sizeof(dbuf[0]) );

    CV_ASSERT( CV_ARE_SIZES_EQ(src, dst) &&
        (CV_MAT_TYPE(stype) == CV_16SC1 || CV_MAT_TYPE(stype) == CV_32FC1) &&
        (CV_MAT_TYPE(dtype) == CV_16SC3 || CV_MAT_TYPE(dtype) == CV_32FC3) );

    cvConvert( _Q, &Q );

    for( y = 0; y < rows; y++ )
    {
        const float* sptr = (const float*)(src->data.ptr + src->step*y);
        float* dptr0 = (float*)(dst->data.ptr + dst->step*y), *dptr = dptr0;
        double qx = q[0][1]*y + q[0][3], qy = q[1][1]*y + q[1][3];
        double qz = q[2][1]*y + q[2][3], qw = q[3][1]*y + q[3][3];

        if( stype == CV_16SC1 )
        {
            const short* sptr0 = (const short*)sptr;
            for( x = 0; x < cols; x++ )
                sbuf[x] = (float)sptr0[x];
            sptr = sbuf;
        }
        if( dtype != CV_32FC3 )
            dptr = dbuf;

        for( x = 0; x < cols; x++, qx += q[0][0], qy += q[1][0], qz += q[2][0], qw += q[3][0] )
        {
            double d = sptr[x];
            double iW = 1./(qw + q[3][2]*d);
            double X = (qx + q[0][2]*d)*iW;
            double Y = (qy + q[1][2]*d)*iW;
            double Z = (qz + q[2][2]*d)*iW;

            dptr[x*3] = (float)X;
            dptr[x*3+1] = (float)Y;
            dptr[x*3+2] = (float)Z;
        }

        if( dtype == CV_16SC3 )
        {
            for( x = 0; x < cols*3; x++ )
            {
                int ival = cvRound(dptr[x]);
                ((short*)dptr0)[x] = CV_CAST_16S(ival);
            }
        }
    }

    __END__;
}


/* End of file. */
