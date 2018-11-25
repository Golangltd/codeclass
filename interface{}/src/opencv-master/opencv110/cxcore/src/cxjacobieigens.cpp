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

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Names:      icvJacobiEigens_32f, icvJacobiEigens_64d
//    Purpose:    Eigenvalues & eigenvectors calculation of a symmetric matrix:
//                A Vi  =  Ei Vi
//    Context:   
//    Parameters: A(n, n) - source symmetric matrix (n - rows & columns number),
//                V(n, n) - matrix of its eigenvectors 
//                          (i-th row is an eigenvector Vi),
//                E(n)    - vector of its eigenvalues
//                          (i-th element is an eigenvalue Ei),
//                eps     - accuracy of diagonalization.
//               
//    Returns:
//    CV_NO_ERROR or error code     
//    Notes:
//        1. The functions destroy source matrix A, so if you need it further, you
//           have to copy it before the processing.
//        2. Eigenvalies and eigenvectors are sorted in Ei absolute value descending.
//        3. Calculation time depends on eps value. If the time isn't very important,
//           we recommend to set eps = 0.
//F*/

/*=========================== Single precision function ================================*/

static CvStatus CV_STDCALL
icvJacobiEigens_32f(float *A, float *V, float *E, int n, float eps)
{
    int i, j, k, ind, iters = 0;
    float *AA = A, *VV = V;
    double Amax, anorm = 0, ax;

    if( A == NULL || V == NULL || E == NULL )
        return CV_NULLPTR_ERR;
    if( n <= 0 )
        return CV_BADSIZE_ERR;
    if( eps < DBL_EPSILON )
        eps = DBL_EPSILON;

    /*-------- Prepare --------*/
    for( i = 0; i < n; i++, VV += n, AA += n )
    {
        for( j = 0; j < i; j++ )
        {
            double Am = AA[j];

            anorm += Am * Am;
        }
        for( j = 0; j < n; j++ )
            VV[j] = 0.f;
        VV[i] = 1.f;
    }

    anorm = sqrt( anorm + anorm );
    ax = anorm * eps / n;
    Amax = anorm;

    while( Amax > ax && iters++ < 100 )
    {
        Amax /= n;
        do                      /* while (ind) */
        {
            int p, q;
            float *V1 = V, *A1 = A;

            ind = 0;
            for( p = 0; p < n - 1; p++, A1 += n, V1 += n )
            {
                float *A2 = A + n * (p + 1), *V2 = V + n * (p + 1);

                for( q = p + 1; q < n; q++, A2 += n, V2 += n )
                {
                    double x, y, c, s, c2, s2, a;
                    float *A3, Apq = A1[q], App, Aqq, Aip, Aiq, Vpi, Vqi;

                    if( fabs( Apq ) < Amax )
                        continue;

                    ind = 1;

                    /*---- Calculation of rotation angle's sine & cosine ----*/
                    App = A1[p];
                    Aqq = A2[q];
                    y = 5.0e-1 * (App - Aqq);
                    x = -Apq / sqrt( (double)Apq * Apq + (double)y * y );
                    if( y < 0.0 )
                        x = -x;
                    s = x / sqrt( 2.0 * (1.0 + sqrt( 1.0 - (double)x * x )));
                    s2 = s * s;
                    c = sqrt( 1.0 - s2 );
                    c2 = c * c;
                    a = 2.0 * Apq * c * s;

                    /*---- Apq annulation ----*/
                    A3 = A;
                    for( i = 0; i < p; i++, A3 += n )
                    {
                        Aip = A3[p];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A3[p] = (float) (Aip * c - Aiq * s);
                        A3[q] = (float) (Aiq * c + Aip * s);
                        V1[i] = (float) (Vpi * c - Vqi * s);
                        V2[i] = (float) (Vqi * c + Vpi * s);
                    }
                    for( ; i < q; i++, A3 += n )
                    {
                        Aip = A1[i];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = (float) (Aip * c - Aiq * s);
                        A3[q] = (float) (Aiq * c + Aip * s);
                        V1[i] = (float) (Vpi * c - Vqi * s);
                        V2[i] = (float) (Vqi * c + Vpi * s);
                    }
                    for( ; i < n; i++ )
                    {
                        Aip = A1[i];
                        Aiq = A2[i];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = (float) (Aip * c - Aiq * s);
                        A2[i] = (float) (Aiq * c + Aip * s);
                        V1[i] = (float) (Vpi * c - Vqi * s);
                        V2[i] = (float) (Vqi * c + Vpi * s);
                    }
                    A1[p] = (float) (App * c2 + Aqq * s2 - a);
                    A2[q] = (float) (App * s2 + Aqq * c2 + a);
                    A1[q] = A2[p] = 0.0f;
                }               /*q */
            }                   /*p */
        }
        while( ind );
        Amax /= n;
    }                           /* while ( Amax > ax ) */

    for( i = 0, k = 0; i < n; i++, k += n + 1 )
        E[i] = A[k];
    /*printf(" M = %d\n", M); */

    /* -------- ordering -------- */
    for( i = 0; i < n; i++ )
    {
        int m = i;
        float Em = (float) fabs( E[i] );

        for( j = i + 1; j < n; j++ )
        {
            float Ej = (float) fabs( E[j] );

            m = (Em < Ej) ? j : m;
            Em = (Em < Ej) ? Ej : Em;
        }
        if( m != i )
        {
            int l;
            float b = E[i];

            E[i] = E[m];
            E[m] = b;
            for( j = 0, k = i * n, l = m * n; j < n; j++, k++, l++ )
            {
                b = V[k];
                V[k] = V[l];
                V[l] = b;
            }
        }
    }

    return CV_NO_ERR;
}

/*=========================== Double precision function ================================*/

static CvStatus CV_STDCALL
icvJacobiEigens_64d(double *A, double *V, double *E, int n, double eps)
{
    int i, j, k, p, q, ind, iters = 0;
    double *A1 = A, *V1 = V, *A2 = A, *V2 = V;
    double Amax = 0.0, anorm = 0.0, ax;

    if( A == NULL || V == NULL || E == NULL )
        return CV_NULLPTR_ERR;
    if( n <= 0 )
        return CV_BADSIZE_ERR;
    if( eps < DBL_EPSILON )
        eps = DBL_EPSILON;

    /*-------- Prepare --------*/
    for( i = 0; i < n; i++, V1 += n, A1 += n )
    {
        for( j = 0; j < i; j++ )
        {
            double Am = A1[j];

            anorm += Am * Am;
        }
        for( j = 0; j < n; j++ )
            V1[j] = 0.0;
        V1[i] = 1.0;
    }

    anorm = sqrt( anorm + anorm );
    ax = anorm * eps / n;
    Amax = anorm;

    while( Amax > ax && iters++ < 100 )
    {
        Amax /= n;
        do                      /* while (ind) */
        {
            ind = 0;
            A1 = A;
            V1 = V;
            for( p = 0; p < n - 1; p++, A1 += n, V1 += n )
            {
                A2 = A + n * (p + 1);
                V2 = V + n * (p + 1);
                for( q = p + 1; q < n; q++, A2 += n, V2 += n )
                {
                    double x, y, c, s, c2, s2, a;
                    double *A3, Apq, App, Aqq, App2, Aqq2, Aip, Aiq, Vpi, Vqi;

                    if( fabs( A1[q] ) < Amax )
                        continue;
                    Apq = A1[q];

                    ind = 1;

                    /*---- Calculation of rotation angle's sine & cosine ----*/
                    App = A1[p];
                    Aqq = A2[q];
                    y = 5.0e-1 * (App - Aqq);
                    x = -Apq / sqrt( Apq * Apq + (double)y * y );
                    if( y < 0.0 )
                        x = -x;
                    s = x / sqrt( 2.0 * (1.0 + sqrt( 1.0 - (double)x * x )));
                    s2 = s * s;
                    c = sqrt( 1.0 - s2 );
                    c2 = c * c;
                    a = 2.0 * Apq * c * s;

                    /*---- Apq annulation ----*/
                    A3 = A;
                    for( i = 0; i < p; i++, A3 += n )
                    {
                        Aip = A3[p];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A3[p] = Aip * c - Aiq * s;
                        A3[q] = Aiq * c + Aip * s;
                        V1[i] = Vpi * c - Vqi * s;
                        V2[i] = Vqi * c + Vpi * s;
                    }
                    for( ; i < q; i++, A3 += n )
                    {
                        Aip = A1[i];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = Aip * c - Aiq * s;
                        A3[q] = Aiq * c + Aip * s;
                        V1[i] = Vpi * c - Vqi * s;
                        V2[i] = Vqi * c + Vpi * s;
                    }
                    for( ; i < n; i++ )
                    {
                        Aip = A1[i];
                        Aiq = A2[i];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = Aip * c - Aiq * s;
                        A2[i] = Aiq * c + Aip * s;
                        V1[i] = Vpi * c - Vqi * s;
                        V2[i] = Vqi * c + Vpi * s;
                    }
                    App2 = App * c2 + Aqq * s2 - a;
                    Aqq2 = App * s2 + Aqq * c2 + a;
                    A1[p] = App2;
                    A2[q] = Aqq2;
                    A1[q] = A2[p] = 0.0;
                }               /*q */
            }                   /*p */
        }
        while( ind );
    }                           /* while ( Amax > ax ) */

    for( i = 0, k = 0; i < n; i++, k += n + 1 )
        E[i] = A[k];

    /* -------- ordering -------- */
    for( i = 0; i < n; i++ )
    {
        int m = i;
        double Em = fabs( E[i] );

        for( j = i + 1; j < n; j++ )
        {
            double Ej = fabs( E[j] );

            m = (Em < Ej) ? j : m;
            Em = (Em < Ej) ? Ej : Em;
        }
        if( m != i )
        {
            int l;
            double b = E[i];

            E[i] = E[m];
            E[m] = b;
            for( j = 0, k = i * n, l = m * n; j < n; j++, k++, l++ )
            {
                b = V[k];
                V[k] = V[l];
                V[l] = b;
            }
        }
    }

    return CV_NO_ERR;
}


CV_IMPL void
cvEigenVV( CvArr* srcarr, CvArr* evectsarr, CvArr* evalsarr, double eps )
{

    CV_FUNCNAME( "cvEigenVV" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat estub1, *evects = (CvMat*)evectsarr;
    CvMat estub2, *evals = (CvMat*)evalsarr;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub ));

    if( !CV_IS_MAT( evects ))
        CV_CALL( evects = cvGetMat( evects, &estub1 ));

    if( !CV_IS_MAT( evals ))
        CV_CALL( evals = cvGetMat( evals, &estub2 ));

    if( src->cols != src->rows )
        CV_ERROR( CV_StsUnmatchedSizes, "source is not quadratic matrix" );

    if( !CV_ARE_SIZES_EQ( src, evects) )
        CV_ERROR( CV_StsUnmatchedSizes, "eigenvectors matrix has inappropriate size" );

    if( (evals->rows != src->rows || evals->cols != 1) &&
        (evals->cols != src->rows || evals->rows != 1))
        CV_ERROR( CV_StsBadSize, "eigenvalues vector has inappropriate size" );

    if( !CV_ARE_TYPES_EQ( src, evects ) || !CV_ARE_TYPES_EQ( src, evals ))
        CV_ERROR( CV_StsUnmatchedFormats,
        "input matrix, eigenvalues and eigenvectors must have the same type" );

    if( !CV_IS_MAT_CONT( src->type & evals->type & evects->type ))
        CV_ERROR( CV_BadStep, "all the matrices must be continuous" );

    if( CV_MAT_TYPE(src->type) == CV_32FC1 )
    {
        IPPI_CALL( icvJacobiEigens_32f( src->data.fl,
                                        evects->data.fl,
                                        evals->data.fl, src->cols, (float)eps ));

    }
    else if( CV_MAT_TYPE(src->type) == CV_64FC1 )
    {
        IPPI_CALL( icvJacobiEigens_64d( src->data.db,
                                        evects->data.db,
                                        evals->data.db, src->cols, eps ));
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "Only 32fC1 and 64fC1 types are supported" );
    }

    CV_CHECK_NANS( evects );
    CV_CHECK_NANS( evals );

    __END__;
}

/* End of file */
