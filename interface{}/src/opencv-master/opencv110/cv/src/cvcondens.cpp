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

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvCreateConDensation
//    Purpose: Creating CvConDensation structure and allocating memory for it
//    Context:
//    Parameters:
//      Kalman     - double pointer to CvConDensation structure
//      DP         - dimension of the dynamical vector
//      MP         - dimension of the measurement vector
//      SamplesNum - number of samples in sample set used in algorithm 
//    Returns:
//    Notes:
//      
//F*/

CV_IMPL CvConDensation* cvCreateConDensation( int DP, int MP, int SamplesNum )
{
    int i;
    CvConDensation *CD = 0;

    CV_FUNCNAME( "cvCreateConDensation" );
    __BEGIN__;
    
    if( DP < 0 || MP < 0 || SamplesNum < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );
    
    /* allocating memory for the structure */
    CV_CALL( CD = (CvConDensation *) cvAlloc( sizeof( CvConDensation )));
    /* setting structure params */
    CD->SamplesNum = SamplesNum;
    CD->DP = DP;
    CD->MP = MP;
    /* allocating memory for structure fields */
    CV_CALL( CD->flSamples = (float **) cvAlloc( sizeof( float * ) * SamplesNum ));
    CV_CALL( CD->flNewSamples = (float **) cvAlloc( sizeof( float * ) * SamplesNum ));
    CV_CALL( CD->flSamples[0] = (float *) cvAlloc( sizeof( float ) * SamplesNum * DP ));
    CV_CALL( CD->flNewSamples[0] = (float *) cvAlloc( sizeof( float ) * SamplesNum * DP ));

    /* setting pointers in pointer's arrays */
    for( i = 1; i < SamplesNum; i++ )
    {
        CD->flSamples[i] = CD->flSamples[i - 1] + DP;
        CD->flNewSamples[i] = CD->flNewSamples[i - 1] + DP;
    }

    CV_CALL( CD->State = (float *) cvAlloc( sizeof( float ) * DP ));
    CV_CALL( CD->DynamMatr = (float *) cvAlloc( sizeof( float ) * DP * DP ));
    CV_CALL( CD->flConfidence = (float *) cvAlloc( sizeof( float ) * SamplesNum ));
    CV_CALL( CD->flCumulative = (float *) cvAlloc( sizeof( float ) * SamplesNum ));

    CV_CALL( CD->RandS = (CvRandState *) cvAlloc( sizeof( CvRandState ) * DP ));
    CV_CALL( CD->Temp = (float *) cvAlloc( sizeof( float ) * DP ));
    CV_CALL( CD->RandomSample = (float *) cvAlloc( sizeof( float ) * DP ));

    /* Returning created structure */
    __END__;

    return CD;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvReleaseConDensation
//    Purpose: Releases CvConDensation structure and frees memory allocated for it
//    Context:
//    Parameters:
//      Kalman     - double pointer to CvConDensation structure
//      DP         - dimension of the dynamical vector
//      MP         - dimension of the measurement vector
//      SamplesNum - number of samples in sample set used in algorithm 
//    Returns:
//    Notes:
//      
//F*/
CV_IMPL void
cvReleaseConDensation( CvConDensation ** ConDensation )
{
    CV_FUNCNAME( "cvReleaseConDensation" );
    __BEGIN__;
    
    CvConDensation *CD = *ConDensation;
    
    if( !ConDensation )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !CD )
        EXIT;

    /* freeing the memory */
	cvFree( &CD->State );
    cvFree( &CD->DynamMatr);
    cvFree( &CD->flConfidence );
    cvFree( &CD->flCumulative );
    cvFree( &CD->flSamples[0] );
    cvFree( &CD->flNewSamples[0] );
    cvFree( &CD->flSamples );
    cvFree( &CD->flNewSamples );
    cvFree( &CD->Temp );
    cvFree( &CD->RandS );
    cvFree( &CD->RandomSample );
    /* release structure */
    cvFree( ConDensation );
    
    __END__;

}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvConDensUpdateByTime
//    Purpose: Performing Time Update routine for ConDensation algorithm
//    Context:
//    Parameters:
//      Kalman     - pointer to CvConDensation structure
//    Returns:
//    Notes:
//      
//F*/
CV_IMPL void
cvConDensUpdateByTime( CvConDensation * ConDens )
{
    int i, j;
    float Sum = 0;

    CV_FUNCNAME( "cvConDensUpdateByTime" );
    __BEGIN__;
    
    if( !ConDens )
        CV_ERROR( CV_StsNullPtr, "" );

    /* Sets Temp to Zero */
    icvSetZero_32f( ConDens->Temp, ConDens->DP, 1 );

    /* Calculating the Mean */
    for( i = 0; i < ConDens->SamplesNum; i++ )
    {
        icvScaleVector_32f( ConDens->flSamples[i], ConDens->State, ConDens->DP,
                             ConDens->flConfidence[i] );
        icvAddVector_32f( ConDens->Temp, ConDens->State, ConDens->Temp, ConDens->DP );
        Sum += ConDens->flConfidence[i];
        ConDens->flCumulative[i] = Sum;
    }

    /* Taking the new vector from transformation of mean by dynamics matrix */

    icvScaleVector_32f( ConDens->Temp, ConDens->Temp, ConDens->DP, 1.f / Sum );
    icvTransformVector_32f( ConDens->DynamMatr, ConDens->Temp, ConDens->State, ConDens->DP,
                             ConDens->DP );
    Sum = Sum / ConDens->SamplesNum;

    /* Updating the set of random samples */
    for( i = 0; i < ConDens->SamplesNum; i++ )
    {
        j = 0;
        while( (ConDens->flCumulative[j] <= (float) i * Sum)&&(j<ConDens->SamplesNum-1))
        {
            j++;
        }
        icvCopyVector_32f( ConDens->flSamples[j], ConDens->DP, ConDens->flNewSamples[i] );
    }

    /* Adding the random-generated vector to every vector in sample set */
    for( i = 0; i < ConDens->SamplesNum; i++ )
    {
        for( j = 0; j < ConDens->DP; j++ )
        {
            cvbRand( ConDens->RandS + j, ConDens->RandomSample + j, 1 );
        }

        icvTransformVector_32f( ConDens->DynamMatr, ConDens->flNewSamples[i],
                                 ConDens->flSamples[i], ConDens->DP, ConDens->DP );
        icvAddVector_32f( ConDens->flSamples[i], ConDens->RandomSample, ConDens->flSamples[i],
                           ConDens->DP );
    }

    __END__;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvConDensInitSamplSet
//    Purpose: Performing Time Update routine for ConDensation algorithm
//    Context:
//    Parameters:
//    conDens     - pointer to CvConDensation structure
//    lowerBound  - vector of lower bounds used to random update of sample set
//    lowerBound  - vector of upper bounds used to random update of sample set
//    Returns:
//    Notes:
//      
//F*/

CV_IMPL void
cvConDensInitSampleSet( CvConDensation * conDens, CvMat * lowerBound, CvMat * upperBound )
{
    int i, j;
    float *LBound;
    float *UBound;
    float Prob = 1.f / conDens->SamplesNum;

    CV_FUNCNAME( "cvConDensInitSampleSet" );
    __BEGIN__;
    
    if( !conDens || !lowerBound || !upperBound )
        CV_ERROR( CV_StsNullPtr, "" );

    if( CV_MAT_TYPE(lowerBound->type) != CV_32FC1 ||
        !CV_ARE_TYPES_EQ(lowerBound,upperBound) )
        CV_ERROR( CV_StsBadArg, "source  has not appropriate format" );

    if( (lowerBound->cols != 1) || (upperBound->cols != 1) )
        CV_ERROR( CV_StsBadArg, "source  has not appropriate size" );

    if( (lowerBound->rows != conDens->DP) || (upperBound->rows != conDens->DP) )
        CV_ERROR( CV_StsBadArg, "source  has not appropriate size" );

    LBound = lowerBound->data.fl;
    UBound = upperBound->data.fl;
    /* Initializing the structures to create initial Sample set */
    for( i = 0; i < conDens->DP; i++ )
    {
        cvRandInit( &(conDens->RandS[i]),
                    LBound[i],
                    UBound[i],
                    i );
    }
    /* Generating the samples */
    for( j = 0; j < conDens->SamplesNum; j++ )
    {
        for( i = 0; i < conDens->DP; i++ )
        {
            cvbRand( conDens->RandS + i, conDens->flSamples[j] + i, 1 );
        }
        conDens->flConfidence[j] = Prob;
    }
    /* Reinitializes the structures to update samples randomly */
    for( i = 0; i < conDens->DP; i++ )
    {
        cvRandInit( &(conDens->RandS[i]),
                    (LBound[i] - UBound[i]) / 5,
                    (UBound[i] - LBound[i]) / 5,
                    i);
    }

    __END__;
}
