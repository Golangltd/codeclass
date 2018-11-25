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


CV_IMPL CvKalman*
cvCreateKalman( int DP, int MP, int CP )
{
    CvKalman *kalman = 0;

    CV_FUNCNAME( "cvCreateKalman" );
    
    __BEGIN__;

    if( DP <= 0 || MP <= 0 )
        CV_ERROR( CV_StsOutOfRange,
        "state and measurement vectors must have positive number of dimensions" );

    if( CP < 0 )
        CP = DP;
    
    /* allocating memory for the structure */
    CV_CALL( kalman = (CvKalman *)cvAlloc( sizeof( CvKalman )));
    memset( kalman, 0, sizeof(*kalman));
    
    kalman->DP = DP;
    kalman->MP = MP;
    kalman->CP = CP;

    CV_CALL( kalman->state_pre = cvCreateMat( DP, 1, CV_32FC1 ));
    cvZero( kalman->state_pre );
    
    CV_CALL( kalman->state_post = cvCreateMat( DP, 1, CV_32FC1 ));
    cvZero( kalman->state_post );
    
    CV_CALL( kalman->transition_matrix = cvCreateMat( DP, DP, CV_32FC1 ));
    cvSetIdentity( kalman->transition_matrix );

    CV_CALL( kalman->process_noise_cov = cvCreateMat( DP, DP, CV_32FC1 ));
    cvSetIdentity( kalman->process_noise_cov );
    
    CV_CALL( kalman->measurement_matrix = cvCreateMat( MP, DP, CV_32FC1 ));
    cvZero( kalman->measurement_matrix );

    CV_CALL( kalman->measurement_noise_cov = cvCreateMat( MP, MP, CV_32FC1 ));
    cvSetIdentity( kalman->measurement_noise_cov );

    CV_CALL( kalman->error_cov_pre = cvCreateMat( DP, DP, CV_32FC1 ));
    
    CV_CALL( kalman->error_cov_post = cvCreateMat( DP, DP, CV_32FC1 ));
    cvZero( kalman->error_cov_post );

    CV_CALL( kalman->gain = cvCreateMat( DP, MP, CV_32FC1 ));

    if( CP > 0 )
    {
        CV_CALL( kalman->control_matrix = cvCreateMat( DP, CP, CV_32FC1 ));
        cvZero( kalman->control_matrix );
    }

    CV_CALL( kalman->temp1 = cvCreateMat( DP, DP, CV_32FC1 ));
    CV_CALL( kalman->temp2 = cvCreateMat( MP, DP, CV_32FC1 ));
    CV_CALL( kalman->temp3 = cvCreateMat( MP, MP, CV_32FC1 ));
    CV_CALL( kalman->temp4 = cvCreateMat( MP, DP, CV_32FC1 ));
    CV_CALL( kalman->temp5 = cvCreateMat( MP, 1, CV_32FC1 ));

#if 1
    kalman->PosterState = kalman->state_pre->data.fl;
    kalman->PriorState = kalman->state_post->data.fl;
    kalman->DynamMatr = kalman->transition_matrix->data.fl;
    kalman->MeasurementMatr = kalman->measurement_matrix->data.fl;
    kalman->MNCovariance = kalman->measurement_noise_cov->data.fl;
    kalman->PNCovariance = kalman->process_noise_cov->data.fl;
    kalman->KalmGainMatr = kalman->gain->data.fl;
    kalman->PriorErrorCovariance = kalman->error_cov_pre->data.fl;
    kalman->PosterErrorCovariance = kalman->error_cov_post->data.fl;
#endif    

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseKalman( &kalman );

    return kalman;
}


CV_IMPL void
cvReleaseKalman( CvKalman** _kalman )
{
    CvKalman *kalman;

    CV_FUNCNAME( "cvReleaseKalman" );
    __BEGIN__;
    
    if( !_kalman )
        CV_ERROR( CV_StsNullPtr, "" );
    
    kalman = *_kalman;
    
    /* freeing the memory */
    cvReleaseMat( &kalman->state_pre );
    cvReleaseMat( &kalman->state_post );
    cvReleaseMat( &kalman->transition_matrix );
    cvReleaseMat( &kalman->control_matrix );
    cvReleaseMat( &kalman->measurement_matrix );
    cvReleaseMat( &kalman->process_noise_cov );
    cvReleaseMat( &kalman->measurement_noise_cov );
    cvReleaseMat( &kalman->error_cov_pre );
    cvReleaseMat( &kalman->gain );
    cvReleaseMat( &kalman->error_cov_post );
    cvReleaseMat( &kalman->temp1 );
    cvReleaseMat( &kalman->temp2 );
    cvReleaseMat( &kalman->temp3 );
    cvReleaseMat( &kalman->temp4 );
    cvReleaseMat( &kalman->temp5 );

    memset( kalman, 0, sizeof(*kalman));

    /* deallocating the structure */
    cvFree( _kalman );

    __END__;
}


CV_IMPL const CvMat*
cvKalmanPredict( CvKalman* kalman, const CvMat* control )
{
    CvMat* result = 0;
    
    CV_FUNCNAME( "cvKalmanPredict" );

    __BEGIN__;
    
    if( !kalman )
        CV_ERROR( CV_StsNullPtr, "" );

    /* update the state */
    /* x'(k) = A*x(k) */
    CV_CALL( cvMatMulAdd( kalman->transition_matrix, kalman->state_post, 0, kalman->state_pre ));

    if( control && kalman->CP > 0 )
        /* x'(k) = x'(k) + B*u(k) */
        CV_CALL( cvMatMulAdd( kalman->control_matrix, control, kalman->state_pre, kalman->state_pre ));
    
    /* update error covariance matrices */
    /* temp1 = A*P(k) */
    CV_CALL( cvMatMulAdd( kalman->transition_matrix, kalman->error_cov_post, 0, kalman->temp1 ));
    
    /* P'(k) = temp1*At + Q */
    CV_CALL( cvGEMM( kalman->temp1, kalman->transition_matrix, 1, kalman->process_noise_cov, 1,
                     kalman->error_cov_pre, CV_GEMM_B_T ));

    result = kalman->state_pre;

    __END__;

    return result;
}


CV_IMPL const CvMat*
cvKalmanCorrect( CvKalman* kalman, const CvMat* measurement )
{
    CvMat* result = 0;

    CV_FUNCNAME( "cvKalmanCorrect" );

    __BEGIN__;
    
    if( !kalman || !measurement )
        CV_ERROR( CV_StsNullPtr, "" );

    /* temp2 = H*P'(k) */
    CV_CALL( cvMatMulAdd( kalman->measurement_matrix,
                          kalman->error_cov_pre, 0, kalman->temp2 ));
    /* temp3 = temp2*Ht + R */
    CV_CALL( cvGEMM( kalman->temp2, kalman->measurement_matrix, 1,
                     kalman->measurement_noise_cov, 1, kalman->temp3, CV_GEMM_B_T ));

    /* temp4 = inv(temp3)*temp2 = Kt(k) */
    CV_CALL( cvSolve( kalman->temp3, kalman->temp2, kalman->temp4, CV_SVD ));

    /* K(k) */
    CV_CALL( cvTranspose( kalman->temp4, kalman->gain ));
    
    /* temp5 = z(k) - H*x'(k) */
    CV_CALL( cvGEMM( kalman->measurement_matrix, kalman->state_pre, -1, measurement, 1, kalman->temp5 ));

    /* x(k) = x'(k) + K(k)*temp5 */
    CV_CALL( cvMatMulAdd( kalman->gain, kalman->temp5, kalman->state_pre, kalman->state_post ));

    /* P(k) = P'(k) - K(k)*temp2 */
    CV_CALL( cvGEMM( kalman->gain, kalman->temp2, -1, kalman->error_cov_pre, 1,
                     kalman->error_cov_post, 0 ));

    result = kalman->state_post;

    __END__;

    return result;
}
