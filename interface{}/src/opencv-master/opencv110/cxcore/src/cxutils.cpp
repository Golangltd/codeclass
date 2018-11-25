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

CV_IMPL void
cvKMeans2( const CvArr* samples_arr, int cluster_count,
           CvArr* labels_arr, CvTermCriteria termcrit )
{
    CvMat* centers = 0;
    CvMat* old_centers = 0;
    CvMat* counters = 0;

    CV_FUNCNAME( "cvKMeans2" );

    __BEGIN__;

    CvMat samples_stub, labels_stub;
    CvMat* samples = (CvMat*)samples_arr;
    CvMat* labels = (CvMat*)labels_arr;
    CvMat* temp = 0;
    CvRNG rng = CvRNG(-1);
    int i, j, k, sample_count, dims;
    int ids_delta, iter;
    double max_dist;

    if( !CV_IS_MAT( samples ))
        CV_CALL( samples = cvGetMat( samples, &samples_stub ));

    if( !CV_IS_MAT( labels ))
        CV_CALL( labels = cvGetMat( labels, &labels_stub ));

    if( cluster_count < 1 )
        CV_ERROR( CV_StsOutOfRange, "Number of clusters should be positive" );

    if( CV_MAT_DEPTH(samples->type) != CV_32F || CV_MAT_TYPE(labels->type) != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "samples should be floating-point matrix, cluster_idx - integer vector" );

    if( (labels->rows != 1 && (labels->cols != 1 || !CV_IS_MAT_CONT(labels->type))) ||
        labels->rows + labels->cols - 1 != samples->rows )
        CV_ERROR( CV_StsUnmatchedSizes,
        "cluster_idx should be 1D vector of the same number of elements as samples' number of rows" );

    CV_CALL( termcrit = cvCheckTermCriteria( termcrit, 1e-6, 100 ));

    termcrit.epsilon *= termcrit.epsilon;
    sample_count = samples->rows;

    if( cluster_count > sample_count )
        cluster_count = sample_count;

    dims = samples->cols*CV_MAT_CN(samples->type);
    ids_delta = labels->step ? labels->step/(int)sizeof(int) : 1;

    CV_CALL( centers = cvCreateMat( cluster_count, dims, CV_64FC1 ));
    CV_CALL( old_centers = cvCreateMat( cluster_count, dims, CV_64FC1 ));
    CV_CALL( counters = cvCreateMat( 1, cluster_count, CV_32SC1 ));

    // init centers
    for( i = 0; i < sample_count; i++ )
        labels->data.i[i] = cvRandInt(&rng) % cluster_count;

    counters->cols = cluster_count; // cut down counters
    max_dist = termcrit.epsilon*2;

    for( iter = 0; iter < termcrit.max_iter; iter++ )
    {
        // computer centers
        cvZero( centers );
        cvZero( counters );

        for( i = 0; i < sample_count; i++ )
        {
            float* s = (float*)(samples->data.ptr + i*samples->step);
            k = labels->data.i[i*ids_delta];
            double* c = (double*)(centers->data.ptr + k*centers->step);
            for( j = 0; j <= dims - 4; j += 4 )
            {
                double t0 = c[j] + s[j];
                double t1 = c[j+1] + s[j+1];

                c[j] = t0;
                c[j+1] = t1;

                t0 = c[j+2] + s[j+2];
                t1 = c[j+3] + s[j+3];

                c[j+2] = t0;
                c[j+3] = t1;
            }
            for( ; j < dims; j++ )
                c[j] += s[j];
            counters->data.i[k]++;
        }

        if( iter > 0 )
            max_dist = 0;

        for( k = 0; k < cluster_count; k++ )
        {
            double* c = (double*)(centers->data.ptr + k*centers->step);
            if( counters->data.i[k] != 0 )
            {
                double scale = 1./counters->data.i[k];
                for( j = 0; j < dims; j++ )
                    c[j] *= scale;
            }
            else
            {
                i = cvRandInt( &rng ) % sample_count;
                float* s = (float*)(samples->data.ptr + i*samples->step);
                for( j = 0; j < dims; j++ )
                    c[j] = s[j];
            }
            
            if( iter > 0 )
            {
                double dist = 0;
                double* c_o = (double*)(old_centers->data.ptr + k*old_centers->step);
                for( j = 0; j < dims; j++ )
                {
                    double t = c[j] - c_o[j];
                    dist += t*t;
                }
                if( max_dist < dist )
                    max_dist = dist;
            }
        }

        // assign labels
        for( i = 0; i < sample_count; i++ )
        {
            float* s = (float*)(samples->data.ptr + i*samples->step);
            int k_best = 0;
            double min_dist = DBL_MAX;

            for( k = 0; k < cluster_count; k++ )
            {
                double* c = (double*)(centers->data.ptr + k*centers->step);
                double dist = 0;
                
                j = 0;
                for( ; j <= dims - 4; j += 4 )
                {
                    double t0 = c[j] - s[j];
                    double t1 = c[j+1] - s[j+1];
                    dist += t0*t0 + t1*t1;
                    t0 = c[j+2] - s[j+2];
                    t1 = c[j+3] - s[j+3];
                    dist += t0*t0 + t1*t1;
                }

                for( ; j < dims; j++ )
                {
                    double t = c[j] - s[j];
                    dist += t*t;
                }

                if( min_dist > dist )
                {
                    min_dist = dist;
                    k_best = k;
                }
            }

            labels->data.i[i*ids_delta] = k_best;
        }

        if( max_dist < termcrit.epsilon )
            break;

        CV_SWAP( centers, old_centers, temp );
    }

    cvZero( counters );
    for( i = 0; i < sample_count; i++ )
        counters->data.i[labels->data.i[i]]++;

    // ensure that we do not have empty clusters
    for( k = 0; k < cluster_count; k++ )
        if( counters->data.i[k] == 0 )
            for(;;)
            {
                i = cvRandInt(&rng) % sample_count;
                j = labels->data.i[i];
                if( counters->data.i[j] > 1 )
                {
                    labels->data.i[i] = k;
                    counters->data.i[j]--;
                    counters->data.i[k]++;
                    break;
                }
            }

    __END__;

    cvReleaseMat( &centers );
    cvReleaseMat( &old_centers );
    cvReleaseMat( &counters );
}


/*
  Finds real roots of cubic, quadratic or linear equation.
  The original code has been taken from Ken Turkowski web page
  (http://www.worldserver.com/turk/opensource/) and adopted for OpenCV.
  Here is the copyright notice.

  -----------------------------------------------------------------------
  Copyright (C) 1978-1999 Ken Turkowski. <turk@computer.org>
 
    All rights reserved.
 
    Warranty Information
      Even though I have reviewed this software, I make no warranty
      or representation, either express or implied, with respect to this
      software, its quality, accuracy, merchantability, or fitness for a
      particular purpose.  As a result, this software is provided "as is,"
      and you, its user, are assuming the entire risk as to its quality
      and accuracy.
 
    This code may be used and freely distributed as long as it includes
    this copyright notice and the above warranty information.
  -----------------------------------------------------------------------
*/
CV_IMPL int
cvSolveCubic( const CvMat* coeffs, CvMat* roots )
{
    int n = 0;
    
    CV_FUNCNAME( "cvSolveCubic" );

    __BEGIN__;

    double a0 = 1., a1, a2, a3;
    double x0 = 0., x1 = 0., x2 = 0.;
    int step = 1, coeff_count;

    if( !CV_IS_MAT(coeffs) )
        CV_ERROR( !coeffs ? CV_StsNullPtr : CV_StsBadArg, "Input parameter is not a valid matrix" );

    if( !CV_IS_MAT(roots) )
        CV_ERROR( !roots ? CV_StsNullPtr : CV_StsBadArg, "Output parameter is not a valid matrix" );

    if( (CV_MAT_TYPE(coeffs->type) != CV_32FC1 && CV_MAT_TYPE(coeffs->type) != CV_64FC1) ||
        (CV_MAT_TYPE(roots->type) != CV_32FC1 && CV_MAT_TYPE(roots->type) != CV_64FC1) )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Both matrices should be floating-point (single or double precision)" );

    coeff_count = coeffs->rows + coeffs->cols - 1;

    if( (coeffs->rows != 1 && coeffs->cols != 1) || (coeff_count != 3 && coeff_count != 4) )
        CV_ERROR( CV_StsBadSize,
        "The matrix of coefficients must be 1-dimensional vector of 3 or 4 elements" );

    if( (roots->rows != 1 && roots->cols != 1) ||
        roots->rows + roots->cols - 1 != 3 )
        CV_ERROR( CV_StsBadSize,
        "The matrix of roots must be 1-dimensional vector of 3 elements" );

    if( CV_MAT_TYPE(coeffs->type) == CV_32FC1 )
    {
        const float* c = coeffs->data.fl;
        if( coeffs->rows > 1 )
            step = coeffs->step/sizeof(c[0]);
        if( coeff_count == 4 )
            a0 = c[0], c += step;
        a1 = c[0];
        a2 = c[step];
        a3 = c[step*2];
    }
    else
    {
        const double* c = coeffs->data.db;
        if( coeffs->rows > 1 )
            step = coeffs->step/sizeof(c[0]);
        if( coeff_count == 4 )
            a0 = c[0], c += step;
        a1 = c[0];
        a2 = c[step];
        a3 = c[step*2];
    }

    if( a0 == 0 )
    {
        if( a1 == 0 )
        {
            if( a2 == 0 )
                n = a3 == 0 ? -1 : 0;
            else
            {
                // linear equation
                x0 = a3/a2;
                n = 1;
            }
        }
        else
        {
            // quadratic equation
            double d = a2*a2 - 4*a1*a3;
            if( d >= 0 )
            {
                d = sqrt(d);
                double q = (-a2 + (a2 < 0 ? -d : d)) * 0.5;
                x0 = q / a1;
                x1 = a3 / q;
                n = d > 0 ? 2 : 1;
            }
        }
    }
    else
    {
        a0 = 1./a0;
        a1 *= a0;
        a2 *= a0;
        a3 *= a0;

        double Q = (a1 * a1 - 3 * a2) * (1./9);
        double R = (2 * a1 * a1 * a1 - 9 * a1 * a2 + 27 * a3) * (1./54);
        double Qcubed = Q * Q * Q;
        double d = Qcubed - R * R;
    
        if( d >= 0 )
        {
            double theta = acos(R / sqrt(Qcubed));
            double sqrtQ = sqrt(Q);
            double t0 = -2 * sqrtQ;
            double t1 = theta * (1./3);
            double t2 = a1 * (1./3);
            x0 = t0 * cos(t1) - t2;
            x1 = t0 * cos(t1 + (2.*CV_PI/3)) - t2;
            x2 = t0 * cos(t1 + (4.*CV_PI/3)) - t2;
            n = 3;
        }
        else
        {
            double e;
            d = sqrt(-d);
            e = pow(d + fabs(R), 0.333333333333);
            if( R > 0 )
                e = -e;
            x0 = (e + Q / e) - a1 * (1./3);
            n = 1;
        }
    }

    step = 1;

    if( CV_MAT_TYPE(roots->type) == CV_32FC1 )
    {
        float* r = roots->data.fl;
        if( roots->rows > 1 )
            step = roots->step/sizeof(r[0]);
        r[0] = (float)x0;
        r[step] = (float)x1;
        r[step*2] = (float)x2;
    }
    else
    {
        double* r = roots->data.db;
        if( roots->rows > 1 )
            step = roots->step/sizeof(r[0]);
        r[0] = x0;
        r[step] = x1;
        r[step*2] = x2;
    }

    __END__;

    return n;
}


/*
  Finds real and complex roots of polynomials of any degree with real 
  coefficients. The original code has been taken from Ken Turkowski's web 
  page (http://www.worldserver.com/turk/opensource/) and adopted for OpenCV.
  Here is the copyright notice.

  -----------------------------------------------------------------------
  Copyright (C) 1981-1999 Ken Turkowski. <turk@computer.org>

  All rights reserved.

  Warranty Information
  Even though I have reviewed this software, I make no warranty
  or representation, either express or implied, with respect to this
  software, its quality, accuracy, merchantability, or fitness for a
  particular purpose.  As a result, this software is provided "as is,"
  and you, its user, are assuming the entire risk as to its quality
  and accuracy.

  This code may be used and freely distributed as long as it includes
  this copyright notice and the above warranty information.
*/


/*******************************************************************************
 * FindPolynomialRoots
 *
 * The Bairstow and Newton correction formulae are used for a simultaneous
 * linear and quadratic iterated synthetic division.  The coefficients of
 * a polynomial of degree n are given as a[i] (i=0,i,..., n) where a[0] is
 * the constant term.  The coefficients are scaled by dividing them by
 * their geometric mean.  The Bairstow or Newton iteration method will
 * nearly always converge to the number of figures carried, fig, either to
 * root values or to their reciprocals.  If the simultaneous Newton and
 * Bairstow iteration fails to converge on root values or their
 * reciprocals in maxiter iterations, the convergence requirement will be
 * successively reduced by one decimal figure.  This program anticipates
 * and protects against loss of significance in the quadratic synthetic
 * division.  (Refer to "On Programming the Numerical Solution of
 * Polynomial Equations," by K. W. Ellenberger, Commun. ACM 3 (Dec. 1960),
 * 644-647.)  The real and imaginary part of each root is stated as u[i]
 * and v[i], respectively.
 *
 * ACM algorithm #30 - Numerical Solution of the Polynomial Equation
 * K. W. Ellenberger
 * Missle Division, North American Aviation, Downey, California
 * Converted to C, modified, optimized, and structured by
 * Ken Turkowski
 * CADLINC, Inc., Palo Alto, California
 *******************************************************************************/

#define MAXN 16

static void icvFindPolynomialRoots(const double *a, double *u, int n, int maxiter, int fig)
{
  int i;
  int j;
  double h[MAXN + 3], b[MAXN + 3], c[MAXN + 3], d[MAXN + 3], e[MAXN + 3];
  // [-2 : n]
  double K, ps, qs, pt, qt, s, rev, r = 0;
  int t;
  double p = 0, q = 0, qq;

  // Zero elements with negative indices
  b[2 + -1] = b[2 + -2] =
    c[2 + -1] = c[2 + -2] =
    d[2 + -1] = d[2 + -2] =
    e[2 + -1] = e[2 + -2] =
    h[2 + -1] = h[2 + -2] = 0.0;

  // Copy polynomial coefficients to working storage
  for (j = n; j >= 0; j--)
    h[2 + j] = *a++; // Note reversal of coefficients

  t = 1;
  K = pow(10.0, (double)(fig)); // Relative accuracy

  for (; h[2 + n] == 0.0; n--) { // Look for zero high-order coeff.
    *u++ = 0.0;
    *u++ = 0.0;
  }

 INIT:
  if (n == 0)
    return;

  ps = qs = pt = qt = s = 0.0;
  rev = 1.0;
  K = pow(10.0, (double)(fig));

  if (n == 1) {
    r = -h[2 + 1] / h[2 + 0];
    goto LINEAR;
  }

  for (j = n; j >= 0; j--) // Find geometric mean of coeff's
    if (h[2 + j] != 0.0)
      s += log(fabs(h[2 + j]));
  s = exp(s / (n + 1));

  for (j = n; j >= 0; j--) // Normalize coeff's by mean
    h[2 + j] /= s;

  if (fabs(h[2 + 1] / h[2 + 0]) < fabs(h[2 + n - 1] / h[2 + n])) {
  REVERSE:
    t = -t;
    for (j = (n - 1) / 2; j >= 0; j--) {
      s = h[2 + j];
      h[2 + j] = h[2 + n - j];
      h[2 + n - j] = s;
    }
  }
  if (qs != 0.0) {
    p = ps;
    q = qs;
  } else {
    if (h[2 + n - 2] == 0.0) {
      q = 1.0;
      p = -2.0;
    } else {
      q = h[2 + n] / h[2 + n - 2];
      p = (h[2 + n - 1] - q * h[2 + n - 3]) / h[2 + n - 2];
    }
    if (n == 2)
      goto QADRTIC;
    r = 0.0;
  }
 ITERATE:
  for (i = maxiter; i > 0; i--) {

    for (j = 0; j <= n; j++) { // BAIRSTOW
      b[2 + j] = h[2 + j] - p * b[2 + j - 1] - q * b[2 + j - 2];
      c[2 + j] = b[2 + j] - p * c[2 + j - 1] - q * c[2 + j - 2];
    }
    if ((h[2 + n - 1] != 0.0) && (b[2 + n - 1] != 0.0)) {
      if (fabs(h[2 + n - 1] / b[2 + n - 1]) >= K) {
	b[2 + n] = h[2 + n] - q * b[2 + n - 2];
      }
      if (b[2 + n] == 0.0)
	goto QADRTIC;
      if (K < fabs(h[2 + n] / b[2 + n]))
	goto QADRTIC;
    }

    for (j = 0; j <= n; j++) { // NEWTON
      d[2 + j] = h[2 + j] + r * d[2 + j - 1]; // Calculate polynomial at r
      e[2 + j] = d[2 + j] + r * e[2 + j - 1]; // Calculate derivative at r
    }
    if (d[2 + n] == 0.0)
      goto LINEAR;
    if (K < fabs(h[2 + n] / d[2 + n]))
      goto LINEAR;

    c[2 + n - 1] = -p * c[2 + n - 2] - q * c[2 + n - 3];
    s = c[2 + n - 2] * c[2 + n - 2] - c[2 + n - 1] * c[2 + n - 3];
    if (s == 0.0) {
      p -= 2.0;
      q *= (q + 1.0);
    } else {
      p += (b[2 + n - 1] * c[2 + n - 2] - b[2 + n] * c[2 + n - 3]) / s;
      q += (-b[2 + n - 1] * c[2 + n - 1] + b[2 + n] * c[2 + n - 2]) / s;
    }
    if (e[2 + n - 1] == 0.0)
      r -= 1.0; // Minimum step
    else
      r -= d[2 + n] / e[2 + n - 1]; // Newton's iteration
  }
  ps = pt;
  qs = qt;
  pt = p;
  qt = q;
  if (rev < 0.0)
    K /= 10.0;
  rev = -rev;
  goto REVERSE;

 LINEAR:
  if (t < 0)
    r = 1.0 / r;
  n--;
  *u++ = r;
  *u++ = 0.0;

  for (j = n; j >= 0; j--) { // Polynomial deflation by lin-nomial
    if ((d[2 + j] != 0.0) && (fabs(h[2 + j] / d[2 + j]) < K))
      h[2 + j] = d[2 + j];
    else
      h[2 + j] = 0.0;
  }

  if (n == 0)
    return;
  goto ITERATE;

 QADRTIC:
  if (t < 0) {
    p /= q;
    q = 1.0 / q;
  }
  n -= 2;

  if (0.0 < (q - (p * p / 4.0))) { // Two complex roots
    s = sqrt(q - (p * p / 4.0));
    *u++ = -p / 2.0;
    *u++ = s;
    *u++ = -p / 2.0;
    *u++ = -s;
  } else { // Two real roots
    s = sqrt(((p * p / 4.0)) - q);
    if (p < 0.0)
      *u++ = qq = -p / 2.0 + s;
    else
      *u++ = qq = -p / 2.0 - s;
    *u++ = 0.0;
    *u++ = q / qq;
    *u++ = 0.0;
  }

  for (j = n; j >= 0; j--) { // Polynomial deflation by quadratic
    if ((b[2 + j] != 0.0) && (fabs(h[2 + j] / b[2 + j]) < K))
      h[2 + j] = b[2 + j];
    else
      h[2 + j] = 0.0;
  }
  goto INIT;
}

#undef MAXN

void cvSolvePoly(const CvMat* a, CvMat *r, int maxiter, int fig)
{
    __BEGIN__;

    int m, n;
    double *ad = 0, *rd = 0;

    CV_FUNCNAME("cvSolvePoly");

    if (CV_MAT_TYPE(a->type) != CV_32FC1 && 
        CV_MAT_TYPE(a->type) != CV_64FC1)
        CV_ERROR(CV_StsUnsupportedFormat, "coeffs must be either CV_32FC1 or CV_64FC1");
    if (CV_MAT_TYPE(r->type) != CV_32FC2 && 
        CV_MAT_TYPE(r->type) != CV_64FC2)
        CV_ERROR(CV_StsUnsupportedFormat, "roots must be either CV_32FC2 or CV_64FC2");
    m = a->rows * a->cols;
    n = r->rows * r->cols;

    if (m - 1 != n)
        CV_ERROR(CV_StsUnmatchedFormats, "must have n + 1 coefficients");

    if( CV_MAT_TYPE(a->type) == CV_32F || !CV_IS_MAT_CONT(a->type))
    {
        ad = (double*)cvStackAlloc(m*sizeof(ad[0]));
        CvMat _a = cvMat( a->rows, a->cols, CV_64F, ad );
        cvConvert( a, &_a );
    }
    else
        ad = a->data.db;

    if( CV_MAT_TYPE(r->type) == CV_32F || !CV_IS_MAT_CONT(r->type))
        rd = (double*)cvStackAlloc(n*sizeof(ad[0]));
    else
        rd = r->data.db;

    icvFindPolynomialRoots( ad, rd, n, maxiter, fig);
    if( rd != r->data.db )
    {
        CvMat _r = cvMat( r->rows, r->cols, CV_64F, rd );
        cvConvert( &_r, r );
    }

    __END__;
}


CV_IMPL void cvNormalize( const CvArr* src, CvArr* dst,
                          double a, double b, int norm_type, const CvArr* mask )
{
    CvMat* tmp = 0;

    CV_FUNCNAME( "cvNormalize" );

    __BEGIN__;

    double scale, shift;
    
    if( norm_type == CV_MINMAX )
    {
        double smin = 0, smax = 0;
        double dmin = MIN( a, b ), dmax = MAX( a, b );
        cvMinMaxLoc( src, &smin, &smax, 0, 0, mask );
        scale = (dmax - dmin)*(smax - smin > DBL_EPSILON ? 1./(smax - smin) : 0);
        shift = dmin - smin*scale;
    }
    else if( norm_type == CV_L2 || norm_type == CV_L1 || norm_type == CV_C )
    {
        CvMat *s = (CvMat*)src, *d = (CvMat*)dst;
        
        if( CV_IS_MAT(s) && CV_IS_MAT(d) && CV_IS_MAT_CONT(s->type & d->type) &&
            CV_ARE_TYPES_EQ(s,d) && CV_ARE_SIZES_EQ(s,d) && !mask &&
            s->cols*s->rows <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
        {
            int i, len = s->cols*s->rows;
            double norm = 0, v;

            if( CV_MAT_TYPE(s->type) == CV_32FC1 )
            {
                const float* sptr = s->data.fl;
                float* dptr = d->data.fl;
                
                if( norm_type == CV_L2 )
                {
                    for( i = 0; i < len; i++ )
                    {
                        v = sptr[i];
                        norm += v*v;
                    }
                    norm = sqrt(norm);
                }
                else if( norm_type == CV_L1 )
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs((double)sptr[i]);
                        norm += v;
                    }
                else
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs((double)sptr[i]);
                        norm = MAX(norm,v);
                    }

                norm = norm > DBL_EPSILON ? 1./norm : 0.;
                for( i = 0; i < len; i++ )
                    dptr[i] = (float)(sptr[i]*norm);
                EXIT;
            }

            if( CV_MAT_TYPE(s->type) == CV_64FC1 )
            {
                const double* sptr = s->data.db;
                double* dptr = d->data.db;
                
                if( norm_type == CV_L2 )
                {
                    for( i = 0; i < len; i++ )
                    {
                        v = sptr[i];
                        norm += v*v;
                    }
                    norm = sqrt(norm);
                }
                else if( norm_type == CV_L1 )
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs(sptr[i]);
                        norm += v;
                    }
                else
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs(sptr[i]);
                        norm = MAX(norm,v);
                    }

                norm = norm > DBL_EPSILON ? 1./norm : 0.;
                for( i = 0; i < len; i++ )
                    dptr[i] = sptr[i]*norm;
                EXIT;
            }
        }
        
        scale = cvNorm( src, 0, norm_type, mask );
        scale = scale > DBL_EPSILON ? 1./scale : 0.;
        shift = 0;
    }
    else
        CV_ERROR( CV_StsBadArg, "Unknown/unsupported norm type" );
    
    if( !mask )
        cvConvertScale( src, dst, scale, shift );
    else
    {
        CvMat stub, *dmat;
        CV_CALL( dmat = cvGetMat(dst, &stub));
        CV_CALL( tmp = cvCreateMat(dmat->rows, dmat->cols, dmat->type) );
        cvConvertScale( src, tmp, scale, shift );
        cvCopy( tmp, dst, mask );
    }

    __END__;

    if( tmp )
        cvReleaseMat( &tmp );
}


CV_IMPL void cvRandShuffle( CvArr* arr, CvRNG* rng, double iter_factor )
{
    CV_FUNCNAME( "cvRandShuffle" );

    __BEGIN__;

    const int sizeof_int = (int)sizeof(int);
    CvMat stub, *mat = (CvMat*)arr;
    int i, j, k, iters, delta = 0;
    int cont_flag, arr_size, elem_size, cols, step;
    const int pair_buf_sz = 100;
    int* pair_buf = (int*)cvStackAlloc( pair_buf_sz*sizeof(pair_buf[0])*2 );
    CvMat _pair_buf = cvMat( 1, pair_buf_sz*2, CV_32S, pair_buf );
    CvRNG _rng = cvRNG(-1);
    uchar* data = 0;
    int* idata = 0;
    
    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub ));

    if( !rng )
        rng = &_rng;

    cols = mat->cols;
    step = mat->step;
    arr_size = cols*mat->rows;
    iters = cvRound(iter_factor*arr_size)*2;
    cont_flag = CV_IS_MAT_CONT(mat->type);
    elem_size = CV_ELEM_SIZE(mat->type);
    if( elem_size % sizeof_int == 0 && (cont_flag || step % sizeof_int == 0) )
    {
        idata = mat->data.i;
        step /= sizeof_int;
        elem_size /= sizeof_int;
    }
    else
        data = mat->data.ptr;

    for( i = 0; i < iters; i += delta )
    {
        delta = MIN( iters - i, pair_buf_sz*2 );
        _pair_buf.cols = delta;
        cvRandArr( rng, &_pair_buf, CV_RAND_UNI, cvRealScalar(0), cvRealScalar(arr_size) );
        
        if( cont_flag )
        {
            if( idata )
                for( j = 0; j < delta; j += 2 )
                {
                    int* p = idata + pair_buf[j]*elem_size, *q = idata + pair_buf[j+1]*elem_size, t;
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
            else
                for( j = 0; j < delta; j += 2 )
                {
                    uchar* p = data + pair_buf[j]*elem_size, *q = data + pair_buf[j+1]*elem_size, t;
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
        }
        else
        {
            if( idata )
                for( j = 0; j < delta; j += 2 )
                {
                    int idx1 = pair_buf[j], idx2 = pair_buf[j+1], row1, row2;
                    int* p, *q, t;
                    row1 = idx1/step; row2 = idx2/step;
                    p = idata + row1*step + (idx1 - row1*cols)*elem_size;
                    q = idata + row2*step + (idx2 - row2*cols)*elem_size;
                    
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
            else
                for( j = 0; j < delta; j += 2 )
                {
                    int idx1 = pair_buf[j], idx2 = pair_buf[j+1], row1, row2;
                    uchar* p, *q, t;
                    row1 = idx1/step; row2 = idx2/step;
                    p = data + row1*step + (idx1 - row1*cols)*elem_size;
                    q = data + row2*step + (idx2 - row2*cols)*elem_size;
                    
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
        }
    }

    __END__;
}


CV_IMPL CvArr*
cvRange( CvArr* arr, double start, double end )
{
    int ok = 0;
    
    CV_FUNCNAME( "cvRange" );

    __BEGIN__;
    
    CvMat stub, *mat = (CvMat*)arr;
    double delta;
    int type, step;
    double val = start;
    int i, j;
    int rows, cols;
    
    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub) );

    rows = mat->rows;
    cols = mat->cols;
    type = CV_MAT_TYPE(mat->type);
    delta = (end-start)/(rows*cols);

    if( CV_IS_MAT_CONT(mat->type) )
    {
        cols *= rows;
        rows = 1;
        step = 1;
    }
    else
        step = mat->step / CV_ELEM_SIZE(type);

    if( type == CV_32SC1 )
    {
        int* idata = mat->data.i;
        int ival = cvRound(val), idelta = cvRound(delta);

        if( fabs(val - ival) < DBL_EPSILON &&
            fabs(delta - idelta) < DBL_EPSILON )
        {
            for( i = 0; i < rows; i++, idata += step )
                for( j = 0; j < cols; j++, ival += idelta )
                    idata[j] = ival;
        }
        else
        {
            for( i = 0; i < rows; i++, idata += step )
                for( j = 0; j < cols; j++, val += delta )
                    idata[j] = cvRound(val);
        }
    }
    else if( type == CV_32FC1 )
    {
        float* fdata = mat->data.fl;
        for( i = 0; i < rows; i++, fdata += step )
            for( j = 0; j < cols; j++, val += delta )
                fdata[j] = (float)val;
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "The function only supports 32sC1 and 32fC1 datatypes" );

    ok = 1;

    __END__;

    return ok ? arr : 0;
}


#define ICV_LT_BY_IDX(a, b) (aux[a] < aux[b])

static CV_IMPLEMENT_QSORT_EX( icvSortIdx64f, int, ICV_LT_BY_IDX, const double* )
static CV_IMPLEMENT_QSORT_EX( icvSortIdx32f, int, ICV_LT_BY_IDX, const float* )
static CV_IMPLEMENT_QSORT_EX( icvSortIdx32s, int, ICV_LT_BY_IDX, const int* )
static CV_IMPLEMENT_QSORT_EX( icvSortIdx16u, int, ICV_LT_BY_IDX, const ushort* )
static CV_IMPLEMENT_QSORT_EX( icvSortIdx16s, int, ICV_LT_BY_IDX, const short* )
static CV_IMPLEMENT_QSORT_EX( icvSortIdx8u, int, ICV_LT_BY_IDX, const uchar* )
static CV_IMPLEMENT_QSORT_EX( icvSortIdx8s, int, ICV_LT_BY_IDX, const schar* )

static CV_IMPLEMENT_QSORT_EX( icvSort64f, double, CV_LT, int )
static CV_IMPLEMENT_QSORT_EX( icvSort32f, float, CV_LT, int )
static CV_IMPLEMENT_QSORT_EX( icvSort32s, int, CV_LT, int )
static CV_IMPLEMENT_QSORT_EX( icvSort16u, ushort, CV_LT, int )
static CV_IMPLEMENT_QSORT_EX( icvSort16s, short, CV_LT, int )
static CV_IMPLEMENT_QSORT_EX( icvSort8u, uchar, CV_LT, int )
static CV_IMPLEMENT_QSORT_EX( icvSort8s, schar, CV_LT, int )

typedef void (*CvSortFunc)( void* arr, size_t n, int );
typedef void (*CvSortIdxFunc)( int* arr, size_t n, const void* );

static inline void
icvCopy1D( const void* src, int s, void* dst, int d, int n, int elemSize )
{
    int i;
    switch( elemSize )
    {
    case 1:
        for( i = 0; i < n; i++ )
            ((uchar*)dst)[i*d] = ((uchar*)src)[i*s];
        break;
    case 2:
        for( i = 0; i < n; i++ )
            ((ushort*)dst)[i*d] = ((ushort*)src)[i*s];
        break;
    case 4:
        for( i = 0; i < n; i++ )
            ((int*)dst)[i*d] = ((int*)src)[i*s];
        break;
    case 8:
        for( i = 0; i < n; i++ )
            ((int64*)dst)[i*d] = ((int64*)src)[i*s];
        break;
    default:
        assert(0);
    }
}

static void
icvShuffle1D( const uchar* src, const int* idx, uchar* dst, int d, int n, int elemSize )
{
    int i;
    switch( elemSize )
    {
    case 1:
        for( i = 0; i < n; i++ )
            dst[i*d] = src[idx[i]];
        break;
    case 2:
        for( i = 0; i < n; i++ )
            ((ushort*)dst)[i*d] = ((ushort*)src)[idx[i]];
        break;
    case 4:
        for( i = 0; i < n; i++ )
            ((int*)dst)[i*d] = ((int*)src)[idx[i]];
        break;
    case 8:
        for( i = 0; i < n; i++ )
            ((int64*)dst)[i*d] = ((int64*)src)[idx[i]];
        break;
    default:
        assert(0);
    }
}


CV_IMPL void
cvSort( const CvArr* _src, CvArr* _dst, CvArr* _idx, int flags )
{
    uchar *tsrc = 0;
    int* tidx = 0;
    
    CV_FUNCNAME("cvSort");

    __BEGIN__;

    CvMat sstub, *src = cvGetMat(_src, &sstub);
    CvMat dstub, *dst = _dst ? cvGetMat(_dst, &dstub) : 0;
    CvMat istub, *idx = _idx ? cvGetMat(_idx, &istub) : 0;
    int type = CV_MAT_TYPE(src->type), elemSize = CV_ELEM_SIZE(type);
    int sstep = src->step, dstep = dst ? dst->step : 0;
    int istep = idx ? idx->step/sizeof(int) : 0;
    int i, len = src->cols;
    CvSortFunc sortFunc = 0;
    CvSortIdxFunc sortIdxFunc = 0;

    if( CV_MAT_CN( src->type ) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "The input matrix should be a one-channel matrix." );
    if( idx )
    {
        if( CV_MAT_TYPE( idx->type ) != CV_32SC1)
            CV_ERROR( CV_StsUnsupportedFormat, "The index matrix must be CV_32SC1." );

        if( !CV_ARE_SIZES_EQ( idx, src ))
            CV_ERROR( CV_StsUnmatchedSizes, "The input matrix and index matrix must be of the same size" );
    }

    if( dst )
    {
        if( !CV_ARE_TYPES_EQ(src, dst) )
            CV_ERROR( CV_StsUnmatchedFormats, "The input and output matrix must be of the same type" );

        if( !CV_ARE_SIZES_EQ(src, dst) )
            CV_ERROR( CV_StsUnmatchedSizes, "The input and output matrix must be of the same size" );
    }

    if( !idx && !dst )
        CV_ERROR( CV_StsNullPtr, "At least one of index array or destination array must be non-NULL" );

    if( type == CV_8U )
        sortIdxFunc = (CvSortIdxFunc)icvSortIdx8u, sortFunc = (CvSortFunc)icvSort8u;
    else if( type == CV_8S )
        sortIdxFunc = (CvSortIdxFunc)icvSortIdx8s, sortFunc = (CvSortFunc)icvSort8s;
    else if( type == CV_16U )
        sortIdxFunc = (CvSortIdxFunc)icvSortIdx16u, sortFunc = (CvSortFunc)icvSort16u;
    else if( type == CV_16S )
        sortIdxFunc = (CvSortIdxFunc)icvSortIdx16s, sortFunc = (CvSortFunc)icvSort16s;
    else if( type == CV_32S )
        sortIdxFunc = (CvSortIdxFunc)icvSortIdx32s, sortFunc = (CvSortFunc)icvSort32s;
    else if( type == CV_32F )
        sortIdxFunc = (CvSortIdxFunc)icvSortIdx32f, sortFunc = (CvSortFunc)icvSort32f;
    else if( type == CV_64F )
        sortIdxFunc = (CvSortIdxFunc)icvSortIdx64f, sortFunc = (CvSortFunc)icvSort64f;
    else
        CV_ERROR( CV_StsUnsupportedFormat, "Unsupported format of the input array" );

    // single-column case, where all of src, idx & dst arrays are continuous, is
    // equivalent to "sort every row" mode.
    if( (flags & CV_SORT_EVERY_COLUMN) &&
        (src->cols > 1 || !CV_IS_MAT_CONT(src->type &
        (dst ? dst->type : -1) & (idx ? idx->type : -1))) )
    {
        uchar* dptr = dst ? dst->data.ptr : 0;
        int* idxptr = idx ? idx->data.i : 0;

        len = src->rows;
        tsrc = (uchar*)cvAlloc(len*elemSize);
        if( idx )
        {
            tidx = (int*)cvAlloc(len*sizeof(tidx[0]));
            for( i = 0; i < len; i++ )
                tidx[i] = i;
        }

        if( flags & CV_SORT_DESCENDING )
        {
            dptr += dstep*(src->rows - 1);
            dstep = -dstep;
            idxptr += istep*(src->rows - 1);
            istep = -istep;
        }

        sstep /= elemSize;
        dstep /= elemSize;

        for( i = 0; i < src->cols; i++ )
        {
            icvCopy1D( src->data.ptr + i*elemSize, sstep, tsrc, 1, len, elemSize );
            if( tidx )
            {
                sortIdxFunc( tidx, len, tsrc );
                if( dst )
                    icvShuffle1D( tsrc, tidx, dptr + i*elemSize, dstep, len, elemSize );
                icvCopy1D( tidx, 1, idxptr + i, istep, len, sizeof(int) );
            }
            else
            {
                sortFunc( tsrc, len, 0 );
                icvCopy1D( tsrc, 1, dptr + i*elemSize, dstep, len, elemSize );
            }
        }
    }
    else
    {
        int j, t, count = src->rows;
        if( flags & CV_SORT_EVERY_COLUMN )
            CV_SWAP( len, count, t );

        if( (flags & CV_SORT_DESCENDING) || (idx && dst && dst->data.ptr == src->data.ptr) )
            tsrc = (uchar*)cvAlloc(len*elemSize);

        for( i = 0; i < count; i++ )
        {
            if( !idx )
            {
                const uchar* sptr = src->data.ptr + i*sstep;
                uchar* dptr = dst->data.ptr + i*dstep;
                uchar* ptr = flags & CV_SORT_DESCENDING ? tsrc : dptr;
                if( ptr != sptr )
                    icvCopy1D( sptr, 1, ptr, 1, len, elemSize );
                sortFunc( ptr, len, 0 );
                if( flags & CV_SORT_DESCENDING )
                    icvCopy1D( ptr + (len-1)*elemSize, -1, dptr, 1, len, elemSize );
            }
            else
            {
                int* idx_ = idx->data.i + istep*i;
                const uchar* sptr = src->data.ptr + sstep*i;
                uchar* dptr = dst ? dst->data.ptr + dstep*i : 0;
                for( j = 0; j < len; j++ )
                    idx_[j] = j;
                if( dptr && dptr == sptr )
                {
                    icvCopy1D( sptr, 1, tsrc, 1, len, elemSize );
                    sptr = tsrc;
                }
                sortIdxFunc( idx_, len, sptr );
                if( flags & CV_SORT_DESCENDING )
                    for( j = 0; j < len/2; j++ )
                        CV_SWAP(idx_[j], idx_[len-j-1], t);
                if( dptr )
                    icvShuffle1D( sptr, idx_, dptr, 1, len, elemSize );
            }
        }
    }

    __END__;

    cvFree( &tsrc );
    cvFree( &tidx );
}

/* End of file. */
