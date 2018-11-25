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


// This is based on the "An Improved Adaptive Background Mixture Model for
// Real-time Tracking with Shadow Detection" by P. KaewTraKulPong and R. Bowden
// http://personal.ee.surrey.ac.uk/Personal/R.Bowden/publications/avbs01/avbs01.pdf
//
// The windowing method is used, but not the shadow detection. I make some of my
// own modifications which make more sense. There are some errors in some of their
// equations.
//
//IplImage values of image that are useful
//int  nSize;         /* sizeof(IplImage) */
//int  depth;         /* pixel depth in bits: IPL_DEPTH_8U ...*/
//int  nChannels;     /* OpenCV functions support 1,2,3 or 4 channels */
//int  width;         /* image width in pixels */
//int  height;        /* image height in pixels */
//int  imageSize;     /* image data size in bytes in case of interleaved data)*/
//char *imageData;    /* pointer to aligned image data */
//char *imageDataOrigin; /* pointer to very origin of image -deallocation */
//Values useful for gaussian integral
//0.5 - 0.19146 - 0.38292
//1.0 - 0.34134 - 0.68268
//1.5 - 0.43319 - 0.86638
//2.0 - 0.47725 - 0.95450
//2.5 - 0.49379 - 0.98758
//3.0 - 0.49865 - 0.99730
//3.5 - 0.4997674 - 0.9995348
//4.0 - 0.4999683 - 0.9999366

#include "_cvaux.h"


//internal functions for gaussian background detection
static void icvInsertionSortGaussians( CvGaussBGPoint* g_point, double* sort_key, CvGaussBGStatModelParams *bg_model_params );

/* 
   Test whether pixel can be explained by background model; 
   Return -1 if no match was found; otherwise the index in match[] is returned

   icvMatchTest(...) assumes what all color channels component exhibit the same variance
   icvMatchTest2(...) accounts for different variances per color channel
 */
static int icvMatchTest( double* src_pixel, int nChannels, int* match, 
                 const CvGaussBGPoint* g_point, const CvGaussBGStatModelParams *bg_model_params );
/*static int icvMatchTest2( double* src_pixel, int nChannels, int* match, 
                 const CvGaussBGPoint* g_point, const CvGaussBGStatModelParams *bg_model_params );*/


/* 
   The update procedure differs between  
      * the initialization phase (named *Partial* ) and
      * the normal phase (named *Full* )
   The initalization phase is defined as not having processed <win_size> frames yet
 */
static void icvUpdateFullWindow( double* src_pixel, int nChannels, 
                         int* match,
                         CvGaussBGPoint* g_point, 
                         const CvGaussBGStatModelParams *bg_model_params );
static void icvUpdateFullNoMatch( IplImage* gm_image, int p, 
                          int* match, 
                          CvGaussBGPoint* g_point, 
                          const CvGaussBGStatModelParams *bg_model_params);
static void icvUpdatePartialWindow( double* src_pixel, int nChannels, int* match, 
                            CvGaussBGPoint* g_point, const CvGaussBGStatModelParams *bg_model_params );
static void icvUpdatePartialNoMatch( double* src_pixel, int nChannels, 
                             int* match, 
                             CvGaussBGPoint* g_point, 
                             const CvGaussBGStatModelParams *bg_model_params);


static void icvGetSortKey( const int nChannels, double* sort_key, const CvGaussBGPoint* g_point, 
                    const CvGaussBGStatModelParams *bg_model_params );
static void icvBackgroundTest( const int nChannels, int n, int i, int j, int *match, CvGaussBGModel* bg_model );

static void CV_CDECL icvReleaseGaussianBGModel( CvGaussBGModel** bg_model );
static int CV_CDECL icvUpdateGaussianBGModel( IplImage* curr_frame, CvGaussBGModel*  bg_model );

//#define for if(0);else for

//g = 1 for first gaussian in list that matches else g = 0
//Rw is the learning rate for weight and Rg is leaning rate for mean and variance
//Ms is the match_sum which is the sum of matches for a particular gaussian
//Ms values are incremented until the sum of Ms values in the list equals window size L
//SMs is the sum of match_sums for gaussians in the list
//Rw = 1/SMs note the smallest Rw gets is 1/L
//Rg = g/Ms for SMs < L and Rg = g/(w*L) for SMs = L
//The list is maintained in sorted order using w/sqrt(variance) as a key
//If there is no match the last gaussian in the list is replaced by the new gaussian
//This will result in changes to SMs which results in changes in Rw and Rg.
//If a gaussian is replaced and SMs previously equaled L values of Ms are computed from w
//w[n+1] = w[n] + Rw*(g - w[n])   weight
//u[n+1] = u[n] + Rg*(x[n+1] - u[n]) mean value Sg is sum n values of g
//v[n+1] = v[n] + Rg*((x[n+1] - u[n])*(x[n+1] - u[n])) - v[n]) variance
//

CV_IMPL CvBGStatModel*
cvCreateGaussianBGModel( IplImage* first_frame, CvGaussBGStatModelParams* parameters )
{
    CvGaussBGModel* bg_model = 0;
    
    CV_FUNCNAME( "cvCreateGaussianBGModel" );
    
    __BEGIN__;
    
    double var_init;
    CvGaussBGStatModelParams params;
    int i, j, k, m, n;
    
    //init parameters
    if( parameters == NULL )
      {                        /* These constants are defined in cvaux/include/cvaux.h: */
        params.win_size      = CV_BGFG_MOG_WINDOW_SIZE;
        params.bg_threshold  = CV_BGFG_MOG_BACKGROUND_THRESHOLD;

        params.std_threshold = CV_BGFG_MOG_STD_THRESHOLD;
        params.weight_init   = CV_BGFG_MOG_WEIGHT_INIT;

        params.variance_init = CV_BGFG_MOG_SIGMA_INIT*CV_BGFG_MOG_SIGMA_INIT;
        params.minArea       = CV_BGFG_MOG_MINAREA;
        params.n_gauss       = CV_BGFG_MOG_NGAUSSIANS;
    }
    else
    {
        params = *parameters;
    }
    
    if( !CV_IS_IMAGE(first_frame) )
        CV_ERROR( CV_StsBadArg, "Invalid or NULL first_frame parameter" );
    
    CV_CALL( bg_model = (CvGaussBGModel*)cvAlloc( sizeof(*bg_model) ));
    memset( bg_model, 0, sizeof(*bg_model) );
    bg_model->type = CV_BG_MODEL_MOG;
    bg_model->release = (CvReleaseBGStatModel)icvReleaseGaussianBGModel;
    bg_model->update = (CvUpdateBGStatModel)icvUpdateGaussianBGModel;
    
    bg_model->params = params;
    
    //prepare storages
    CV_CALL( bg_model->g_point = (CvGaussBGPoint*)cvAlloc(sizeof(CvGaussBGPoint)*
        ((first_frame->width*first_frame->height) + 256)));
    
    CV_CALL( bg_model->background = cvCreateImage(cvSize(first_frame->width,
        first_frame->height), IPL_DEPTH_8U, first_frame->nChannels));
    CV_CALL( bg_model->foreground = cvCreateImage(cvSize(first_frame->width,
        first_frame->height), IPL_DEPTH_8U, 1));
    
    CV_CALL( bg_model->storage = cvCreateMemStorage());
    
    //initializing
    var_init = 2 * params.std_threshold * params.std_threshold;
    CV_CALL( bg_model->g_point[0].g_values =
        (CvGaussBGValues*)cvAlloc( sizeof(CvGaussBGValues)*params.n_gauss*
        (first_frame->width*first_frame->height + 128)));
    
    for( i = 0, n = 0; i < first_frame->height; i++ )
    {
        for( j = 0; j < first_frame->width; j++, n++ )
        {
            const int p = i*first_frame->widthStep+j*first_frame->nChannels;

            bg_model->g_point[n].g_values =
                bg_model->g_point[0].g_values + n*params.n_gauss;
            bg_model->g_point[n].g_values[0].weight = 1;    //the first value seen has weight one
            bg_model->g_point[n].g_values[0].match_sum = 1;
            for( m = 0; m < first_frame->nChannels; m++)
            {
                bg_model->g_point[n].g_values[0].variance[m] = var_init;
                bg_model->g_point[n].g_values[0].mean[m] = (unsigned char)first_frame->imageData[p + m];
            }
            for( k = 1; k < params.n_gauss; k++)
            {
                bg_model->g_point[n].g_values[k].weight = 0;
                bg_model->g_point[n].g_values[k].match_sum = 0;
                for( m = 0; m < first_frame->nChannels; m++){
                    bg_model->g_point[n].g_values[k].variance[m] = var_init;
                    bg_model->g_point[n].g_values[k].mean[m] = 0;
                }
            }
        }
    }
    
    bg_model->countFrames = 0;
    
    __END__;
    
    if( cvGetErrStatus() < 0 )
    {
        CvBGStatModel* base_ptr = (CvBGStatModel*)bg_model;
        
        if( bg_model && bg_model->release )
            bg_model->release( &base_ptr );
        else
            cvFree( &bg_model );
        bg_model = 0;
    }
    
    return (CvBGStatModel*)bg_model;
}


static void CV_CDECL
icvReleaseGaussianBGModel( CvGaussBGModel** _bg_model )
{
    CV_FUNCNAME( "icvReleaseGaussianBGModel" );

    __BEGIN__;
    
    if( !_bg_model )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *_bg_model )
    {
        CvGaussBGModel* bg_model = *_bg_model;
        if( bg_model->g_point )
        {
            cvFree( &bg_model->g_point[0].g_values );
            cvFree( &bg_model->g_point );
        }
        
        cvReleaseImage( &bg_model->background );
        cvReleaseImage( &bg_model->foreground );
        cvReleaseMemStorage(&bg_model->storage);
        memset( bg_model, 0, sizeof(*bg_model) );
        cvFree( _bg_model );
    }

    __END__;
}


static int CV_CDECL
icvUpdateGaussianBGModel( IplImage* curr_frame, CvGaussBGModel*  bg_model )
{
    int i, j, k, n;
    int region_count = 0;
    CvSeq *first_seq = NULL, *prev_seq = NULL, *seq = NULL;
    
    bg_model->countFrames++;
    
    for( i = 0, n = 0; i < curr_frame->height; i++ )
    {
        for( j = 0; j < curr_frame->width; j++, n++ )
        {
            int match[CV_BGFG_MOG_MAX_NGAUSSIANS];
            double sort_key[CV_BGFG_MOG_MAX_NGAUSSIANS];
            const int nChannels = curr_frame->nChannels;
            const int p = curr_frame->widthStep*i+j*nChannels;
            
            // A few short cuts
            CvGaussBGPoint* g_point = &bg_model->g_point[n];
            const CvGaussBGStatModelParams bg_model_params = bg_model->params;
            double pixel[4];
            int no_match;
            
            for( k = 0; k < nChannels; k++ )
                pixel[k] = (uchar)curr_frame->imageData[p+k];
            
            no_match = icvMatchTest( pixel, nChannels, match, g_point, &bg_model_params );
            if( bg_model->countFrames >= bg_model->params.win_size )
            {
                icvUpdateFullWindow( pixel, nChannels, match, g_point, &bg_model->params );
                if( no_match == -1)
                    icvUpdateFullNoMatch( curr_frame, p, match, g_point, &bg_model_params );
            }
            else
            {
                icvUpdatePartialWindow( pixel, nChannels, match, g_point, &bg_model_params );
                if( no_match == -1)
                    icvUpdatePartialNoMatch( pixel, nChannels, match, g_point, &bg_model_params );
            }
            icvGetSortKey( nChannels, sort_key, g_point, &bg_model_params );
            icvInsertionSortGaussians( g_point, sort_key, (CvGaussBGStatModelParams *)&bg_model_params );
            icvBackgroundTest( nChannels, n, i, j, match, bg_model );
        }
    }
    
    //foreground filtering
    
    //filter small regions
    cvClearMemStorage(bg_model->storage);
    
    //cvMorphologyEx( bg_model->foreground, bg_model->foreground, 0, 0, CV_MOP_OPEN, 1 );
    //cvMorphologyEx( bg_model->foreground, bg_model->foreground, 0, 0, CV_MOP_CLOSE, 1 );
    
    cvFindContours( bg_model->foreground, bg_model->storage, &first_seq, sizeof(CvContour), CV_RETR_LIST );
    for( seq = first_seq; seq; seq = seq->h_next )
    {
        CvContour* cnt = (CvContour*)seq;
        if( cnt->rect.width * cnt->rect.height < bg_model->params.minArea )
        {
            //delete small contour
            prev_seq = seq->h_prev;
            if( prev_seq )
            {
                prev_seq->h_next = seq->h_next;
                if( seq->h_next ) seq->h_next->h_prev = prev_seq;
            }
            else
            {
                first_seq = seq->h_next;
                if( seq->h_next ) seq->h_next->h_prev = NULL;
            }
        }
        else
        {
            region_count++;
        }
    }
    bg_model->foreground_regions = first_seq;
    cvZero(bg_model->foreground);
    cvDrawContours(bg_model->foreground, first_seq, CV_RGB(0, 0, 255), CV_RGB(0, 0, 255), 10, -1);
    
    return region_count;
}

static void icvInsertionSortGaussians( CvGaussBGPoint* g_point, double* sort_key, CvGaussBGStatModelParams *bg_model_params )
{
    int i, j;
    for( i = 1; i < bg_model_params->n_gauss; i++ )
    {
        double index = sort_key[i];
        for( j = i; j > 0 && sort_key[j-1] < index; j-- ) //sort decending order
        {
            double temp_sort_key = sort_key[j];
            sort_key[j] = sort_key[j-1];
            sort_key[j-1] = temp_sort_key;
            
            CvGaussBGValues temp_gauss_values = g_point->g_values[j];
            g_point->g_values[j] = g_point->g_values[j-1];
            g_point->g_values[j-1] = temp_gauss_values;
        }
//        sort_key[j] = index;
    }
}


static int icvMatchTest( double* src_pixel, int nChannels, int* match,
                         const CvGaussBGPoint* g_point,
                         const CvGaussBGStatModelParams *bg_model_params )
{
    int k;
    int matchPosition=-1;
    for ( k = 0; k < bg_model_params->n_gauss; k++) match[k]=0;
    
    for ( k = 0; k < bg_model_params->n_gauss; k++) {
        double sum_d2 = 0.0;
        double var_threshold = 0.0;
        for(int m = 0; m < nChannels; m++){
            double d = g_point->g_values[k].mean[m]- src_pixel[m];
            sum_d2 += (d*d);
            var_threshold += g_point->g_values[k].variance[m];
        }  //difference < STD_LIMIT*STD_LIMIT or difference**2 < STD_LIMIT*STD_LIMIT*VAR
        var_threshold = bg_model_params->std_threshold*bg_model_params->std_threshold*var_threshold;
        if(sum_d2 < var_threshold){
            match[k] = 1;
            matchPosition = k;
            break;
        }
    }
    
    return matchPosition;
}

/*
static int icvMatchTest2( double* src_pixel, int nChannels, int* match,
                          const CvGaussBGPoint* g_point,
                          const CvGaussBGStatModelParams *bg_model_params )
{
    int k, m;
    int matchPosition=-1;
    
    for( k = 0; k < bg_model_params->n_gauss; k++ )
        match[k] = 0;
    
    for( k = 0; k < bg_model_params->n_gauss; k++ )
    {
        double sum_d2 = 0.0, var_threshold;
        for( m = 0; m < nChannels; m++ )
        {
            double d = g_point->g_values[k].mean[m]- src_pixel[m];
            sum_d2 += (d*d) / (g_point->g_values[k].variance[m] * g_point->g_values[k].variance[m]);
        }  //difference < STD_LIMIT*STD_LIMIT or difference**2 < STD_LIMIT*STD_LIMIT*VAR
        
        var_threshold = bg_model_params->std_threshold*bg_model_params->std_threshold;
        if( sum_d2 < var_threshold )
        {
            match[k] = 1;
            matchPosition = k;
            break;
        }
    }
    
    return matchPosition;
}
*/

static void icvUpdateFullWindow( double* src_pixel, int nChannels, int* match,
                                 CvGaussBGPoint* g_point,
                                 const CvGaussBGStatModelParams *bg_model_params )
{
    const double learning_rate_weight = (1.0/(double)bg_model_params->win_size);
    for(int k = 0; k < bg_model_params->n_gauss; k++){
        g_point->g_values[k].weight = g_point->g_values[k].weight +
            (learning_rate_weight*((double)match[k] -
            g_point->g_values[k].weight));
        if(match[k]){
            double learning_rate_gaussian = (double)match[k]/(g_point->g_values[k].weight*
                (double)bg_model_params->win_size);
            for(int m = 0; m < nChannels; m++){
                const double tmpDiff = src_pixel[m] - g_point->g_values[k].mean[m];
                g_point->g_values[k].mean[m] = g_point->g_values[k].mean[m] +
                    (learning_rate_gaussian * tmpDiff);
                g_point->g_values[k].variance[m] = g_point->g_values[k].variance[m]+
                    (learning_rate_gaussian*((tmpDiff*tmpDiff) - g_point->g_values[k].variance[m]));
            }
        }
    }
}


static void icvUpdatePartialWindow( double* src_pixel, int nChannels, int* match, CvGaussBGPoint* g_point, const CvGaussBGStatModelParams *bg_model_params )
{
    int k, m;
    int window_current = 0;
    
    for( k = 0; k < bg_model_params->n_gauss; k++ )
        window_current += g_point->g_values[k].match_sum;
    
    for( k = 0; k < bg_model_params->n_gauss; k++ )
    {
        g_point->g_values[k].match_sum += match[k];
        double learning_rate_weight = (1.0/((double)window_current + 1.0)); //increased by one since sum
        g_point->g_values[k].weight = g_point->g_values[k].weight +
            (learning_rate_weight*((double)match[k] - g_point->g_values[k].weight));
        
        if( g_point->g_values[k].match_sum > 0 && match[k] )
        {
            double learning_rate_gaussian = (double)match[k]/((double)g_point->g_values[k].match_sum);
            for( m = 0; m < nChannels; m++ )
            {
                const double tmpDiff = src_pixel[m] - g_point->g_values[k].mean[m];
                g_point->g_values[k].mean[m] = g_point->g_values[k].mean[m] +
                    (learning_rate_gaussian*tmpDiff);
                g_point->g_values[k].variance[m] = g_point->g_values[k].variance[m]+
                    (learning_rate_gaussian*((tmpDiff*tmpDiff) - g_point->g_values[k].variance[m]));
            }
        }
    }
}

static void icvUpdateFullNoMatch( IplImage* gm_image, int p, int* match,
                                  CvGaussBGPoint* g_point,
                                  const CvGaussBGStatModelParams *bg_model_params)
{
    int k, m;
    double alpha;
    int match_sum_total = 0;

    //new value of last one
    g_point->g_values[bg_model_params->n_gauss - 1].match_sum = 1;
    
    //get sum of all but last value of match_sum
    
    for( k = 0; k < bg_model_params->n_gauss ; k++ )
        match_sum_total += g_point->g_values[k].match_sum;
    
    g_point->g_values[bg_model_params->n_gauss - 1].weight = 1./(double)match_sum_total;
    for( m = 0; m < gm_image->nChannels ; m++ )
    {
        // first pass mean is image value
        g_point->g_values[bg_model_params->n_gauss - 1].variance[m] = bg_model_params->variance_init;
        g_point->g_values[bg_model_params->n_gauss - 1].mean[m] = (unsigned char)gm_image->imageData[p + m];
    }
    
    alpha = 1.0 - (1.0/bg_model_params->win_size);
    for( k = 0; k < bg_model_params->n_gauss - 1; k++ )
    {
        g_point->g_values[k].weight *= alpha;
        if( match[k] )
            g_point->g_values[k].weight += alpha;
    }
}


static void
icvUpdatePartialNoMatch(double *pixel,
                        int nChannels,
                        int* /*match*/,
                        CvGaussBGPoint* g_point,
                        const CvGaussBGStatModelParams *bg_model_params)
{
    int k, m;
    //new value of last one
    g_point->g_values[bg_model_params->n_gauss - 1].match_sum = 1;
    
    //get sum of all but last value of match_sum
    int match_sum_total = 0;
    for(k = 0; k < bg_model_params->n_gauss ; k++)
        match_sum_total += g_point->g_values[k].match_sum;

    for(m = 0; m < nChannels; m++)
    {
        //first pass mean is image value
        g_point->g_values[bg_model_params->n_gauss - 1].variance[m] = bg_model_params->variance_init;
        g_point->g_values[bg_model_params->n_gauss - 1].mean[m] = pixel[m];
    }
    for(k = 0; k < bg_model_params->n_gauss; k++)
    {
        g_point->g_values[k].weight = (double)g_point->g_values[k].match_sum /
            (double)match_sum_total;
    }
}

static void icvGetSortKey( const int nChannels, double* sort_key, const CvGaussBGPoint* g_point,
                           const CvGaussBGStatModelParams *bg_model_params )
{
    int k, m;
    for( k = 0; k < bg_model_params->n_gauss; k++ )
    {
        // Avoid division by zero
        if( g_point->g_values[k].match_sum > 0 )
        {
            // Independence assumption between components
            double variance_sum = 0.0;
            for( m = 0; m < nChannels; m++ )
                variance_sum += g_point->g_values[k].variance[m];
            
            sort_key[k] = g_point->g_values[k].weight/sqrt(variance_sum);
        }
        else
            sort_key[k]= 0.0;
    }
}


static void icvBackgroundTest( const int nChannels, int n, int i, int j, int *match, CvGaussBGModel* bg_model )
{
    int m, b;
    uchar pixelValue = (uchar)255; // will switch to 0 if match found
    double weight_sum = 0.0;
    CvGaussBGPoint* g_point = bg_model->g_point;
    
    for( m = 0; m < nChannels; m++)
        bg_model->background->imageData[ bg_model->background->widthStep*i + j*nChannels + m]  = (unsigned char)(g_point[n].g_values[0].mean[m]+0.5);
    
    for( b = 0; b < bg_model->params.n_gauss; b++)
    {
        weight_sum += g_point[n].g_values[b].weight;
        if( match[b] )
            pixelValue = 0;
        if( weight_sum > bg_model->params.bg_threshold )
            break;
    }
    
    bg_model->foreground->imageData[ bg_model->foreground->widthStep*i + j] = pixelValue;
}

/* End of file. */
