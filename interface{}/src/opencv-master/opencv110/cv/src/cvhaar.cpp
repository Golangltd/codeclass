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

/* Haar features calculation */

#include "_cv.h"
#include <stdio.h>

/* these settings affect the quality of detection: change with care */
#define CV_ADJUST_FEATURES 1
#define CV_ADJUST_WEIGHTS  0

typedef int sumtype;
typedef double sqsumtype;

typedef struct CvHidHaarFeature
{
    struct
    {
        sumtype *p0, *p1, *p2, *p3;
        float weight;
    }
    rect[CV_HAAR_FEATURE_MAX];
}
CvHidHaarFeature;


typedef struct CvHidHaarTreeNode
{
    CvHidHaarFeature feature;
    float threshold;
    int left;
    int right;
}
CvHidHaarTreeNode;


typedef struct CvHidHaarClassifier
{
    int count;
    //CvHaarFeature* orig_feature;
    CvHidHaarTreeNode* node;
    float* alpha;
}
CvHidHaarClassifier;


typedef struct CvHidHaarStageClassifier
{
    int  count;
    float threshold;
    CvHidHaarClassifier* classifier;
    int two_rects;

    struct CvHidHaarStageClassifier* next;
    struct CvHidHaarStageClassifier* child;
    struct CvHidHaarStageClassifier* parent;
}
CvHidHaarStageClassifier;


struct CvHidHaarClassifierCascade
{
    int  count;
    int  is_stump_based;
    int  has_tilted_features;
    int  is_tree;
    double inv_window_area;
    CvMat sum, sqsum, tilted;
    CvHidHaarStageClassifier* stage_classifier;
    sqsumtype *pq0, *pq1, *pq2, *pq3;
    sumtype *p0, *p1, *p2, *p3;

    void** ipp_stages;
};


/* IPP functions for object detection */
icvHaarClassifierInitAlloc_32f_t icvHaarClassifierInitAlloc_32f_p = 0;
icvHaarClassifierFree_32f_t icvHaarClassifierFree_32f_p = 0;
icvApplyHaarClassifier_32f_C1R_t icvApplyHaarClassifier_32f_C1R_p = 0;
icvRectStdDev_32f_C1R_t icvRectStdDev_32f_C1R_p = 0;

const int icv_object_win_border = 1;
const float icv_stage_threshold_bias = 0.0001f;

static CvHaarClassifierCascade*
icvCreateHaarClassifierCascade( int stage_count )
{
    CvHaarClassifierCascade* cascade = 0;

    CV_FUNCNAME( "icvCreateHaarClassifierCascade" );

    __BEGIN__;

    int block_size = sizeof(*cascade) + stage_count*sizeof(*cascade->stage_classifier);

    if( stage_count <= 0 )
        CV_ERROR( CV_StsOutOfRange, "Number of stages should be positive" );

    CV_CALL( cascade = (CvHaarClassifierCascade*)cvAlloc( block_size ));
    memset( cascade, 0, block_size );

    cascade->stage_classifier = (CvHaarStageClassifier*)(cascade + 1);
    cascade->flags = CV_HAAR_MAGIC_VAL;
    cascade->count = stage_count;

    __END__;

    return cascade;
}

static void
icvReleaseHidHaarClassifierCascade( CvHidHaarClassifierCascade** _cascade )
{
    if( _cascade && *_cascade )
    {
        CvHidHaarClassifierCascade* cascade = *_cascade;
        if( cascade->ipp_stages && icvHaarClassifierFree_32f_p )
        {
            int i;
            for( i = 0; i < cascade->count; i++ )
            {
                if( cascade->ipp_stages[i] )
                    icvHaarClassifierFree_32f_p( cascade->ipp_stages[i] );
            }
        }
        cvFree( &cascade->ipp_stages );
        cvFree( _cascade );
    }
}

/* create more efficient internal representation of haar classifier cascade */
static CvHidHaarClassifierCascade*
icvCreateHidHaarClassifierCascade( CvHaarClassifierCascade* cascade )
{
    CvRect* ipp_features = 0;
    float *ipp_weights = 0, *ipp_thresholds = 0, *ipp_val1 = 0, *ipp_val2 = 0;
    int* ipp_counts = 0;

    CvHidHaarClassifierCascade* out = 0;

    CV_FUNCNAME( "icvCreateHidHaarClassifierCascade" );

    __BEGIN__;

    int i, j, k, l;
    int datasize;
    int total_classifiers = 0;
    int total_nodes = 0;
    char errorstr[100];
    CvHidHaarClassifier* haar_classifier_ptr;
    CvHidHaarTreeNode* haar_node_ptr;
    CvSize orig_window_size;
    int has_tilted_features = 0;
    int max_count = 0;

    if( !CV_IS_HAAR_CLASSIFIER(cascade) )
        CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier pointer" );

    if( cascade->hid_cascade )
        CV_ERROR( CV_StsError, "hid_cascade has been already created" );

    if( !cascade->stage_classifier )
        CV_ERROR( CV_StsNullPtr, "" );

    if( cascade->count <= 0 )
        CV_ERROR( CV_StsOutOfRange, "Negative number of cascade stages" );

    orig_window_size = cascade->orig_window_size;

    /* check input structure correctness and calculate total memory size needed for
       internal representation of the classifier cascade */
    for( i = 0; i < cascade->count; i++ )
    {
        CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;

        if( !stage_classifier->classifier ||
            stage_classifier->count <= 0 )
        {
            sprintf( errorstr, "header of the stage classifier #%d is invalid "
                     "(has null pointers or non-positive classfier count)", i );
            CV_ERROR( CV_StsError, errorstr );
        }

        max_count = MAX( max_count, stage_classifier->count );
        total_classifiers += stage_classifier->count;

        for( j = 0; j < stage_classifier->count; j++ )
        {
            CvHaarClassifier* classifier = stage_classifier->classifier + j;

            total_nodes += classifier->count;
            for( l = 0; l < classifier->count; l++ )
            {
                for( k = 0; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    if( classifier->haar_feature[l].rect[k].r.width )
                    {
                        CvRect r = classifier->haar_feature[l].rect[k].r;
                        int tilted = classifier->haar_feature[l].tilted;
                        has_tilted_features |= tilted != 0;
                        if( r.width < 0 || r.height < 0 || r.y < 0 ||
                            r.x + r.width > orig_window_size.width
                            ||
                            (!tilted &&
                            (r.x < 0 || r.y + r.height > orig_window_size.height))
                            ||
                            (tilted && (r.x - r.height < 0 ||
                            r.y + r.width + r.height > orig_window_size.height)))
                        {
                            sprintf( errorstr, "rectangle #%d of the classifier #%d of "
                                     "the stage classifier #%d is not inside "
                                     "the reference (original) cascade window", k, j, i );
                            CV_ERROR( CV_StsNullPtr, errorstr );
                        }
                    }
                }
            }
        }
    }

    // this is an upper boundary for the whole hidden cascade size
    datasize = sizeof(CvHidHaarClassifierCascade) +
               sizeof(CvHidHaarStageClassifier)*cascade->count +
               sizeof(CvHidHaarClassifier) * total_classifiers +
               sizeof(CvHidHaarTreeNode) * total_nodes +
               sizeof(void*)*(total_nodes + total_classifiers);

    CV_CALL( out = (CvHidHaarClassifierCascade*)cvAlloc( datasize ));
    memset( out, 0, sizeof(*out) );

    /* init header */
    out->count = cascade->count;
    out->stage_classifier = (CvHidHaarStageClassifier*)(out + 1);
    haar_classifier_ptr = (CvHidHaarClassifier*)(out->stage_classifier + cascade->count);
    haar_node_ptr = (CvHidHaarTreeNode*)(haar_classifier_ptr + total_classifiers);

    out->is_stump_based = 1;
    out->has_tilted_features = has_tilted_features;
    out->is_tree = 0;

    /* initialize internal representation */
    for( i = 0; i < cascade->count; i++ )
    {
        CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;
        CvHidHaarStageClassifier* hid_stage_classifier = out->stage_classifier + i;

        hid_stage_classifier->count = stage_classifier->count;
        hid_stage_classifier->threshold = stage_classifier->threshold - icv_stage_threshold_bias;
        hid_stage_classifier->classifier = haar_classifier_ptr;
        hid_stage_classifier->two_rects = 1;
        haar_classifier_ptr += stage_classifier->count;

        hid_stage_classifier->parent = (stage_classifier->parent == -1)
            ? NULL : out->stage_classifier + stage_classifier->parent;
        hid_stage_classifier->next = (stage_classifier->next == -1)
            ? NULL : out->stage_classifier + stage_classifier->next;
        hid_stage_classifier->child = (stage_classifier->child == -1)
            ? NULL : out->stage_classifier + stage_classifier->child;

        out->is_tree |= hid_stage_classifier->next != NULL;

        for( j = 0; j < stage_classifier->count; j++ )
        {
            CvHaarClassifier* classifier = stage_classifier->classifier + j;
            CvHidHaarClassifier* hid_classifier = hid_stage_classifier->classifier + j;
            int node_count = classifier->count;
            float* alpha_ptr = (float*)(haar_node_ptr + node_count);

            hid_classifier->count = node_count;
            hid_classifier->node = haar_node_ptr;
            hid_classifier->alpha = alpha_ptr;

            for( l = 0; l < node_count; l++ )
            {
                CvHidHaarTreeNode* node = hid_classifier->node + l;
                CvHaarFeature* feature = classifier->haar_feature + l;
                memset( node, -1, sizeof(*node) );
                node->threshold = classifier->threshold[l];
                node->left = classifier->left[l];
                node->right = classifier->right[l];

                if( fabs(feature->rect[2].weight) < DBL_EPSILON ||
                    feature->rect[2].r.width == 0 ||
                    feature->rect[2].r.height == 0 )
                    memset( &(node->feature.rect[2]), 0, sizeof(node->feature.rect[2]) );
                else
                    hid_stage_classifier->two_rects = 0;
            }

            memcpy( alpha_ptr, classifier->alpha, (node_count+1)*sizeof(alpha_ptr[0]));
            haar_node_ptr =
                (CvHidHaarTreeNode*)cvAlignPtr(alpha_ptr+node_count+1, sizeof(void*));

            out->is_stump_based &= node_count == 1;
        }
    }

    {
    int can_use_ipp = icvHaarClassifierInitAlloc_32f_p != 0 &&
        icvHaarClassifierFree_32f_p != 0 &&
                      icvApplyHaarClassifier_32f_C1R_p != 0 &&
                      icvRectStdDev_32f_C1R_p != 0 &&
                      !out->has_tilted_features && !out->is_tree && out->is_stump_based;

    if( can_use_ipp )
    {
        int ipp_datasize = cascade->count*sizeof(out->ipp_stages[0]);
        float ipp_weight_scale=(float)(1./((orig_window_size.width-icv_object_win_border*2)*
            (orig_window_size.height-icv_object_win_border*2)));

        CV_CALL( out->ipp_stages = (void**)cvAlloc( ipp_datasize ));
        memset( out->ipp_stages, 0, ipp_datasize );

        CV_CALL( ipp_features = (CvRect*)cvAlloc( max_count*3*sizeof(ipp_features[0]) ));
        CV_CALL( ipp_weights = (float*)cvAlloc( max_count*3*sizeof(ipp_weights[0]) ));
        CV_CALL( ipp_thresholds = (float*)cvAlloc( max_count*sizeof(ipp_thresholds[0]) ));
        CV_CALL( ipp_val1 = (float*)cvAlloc( max_count*sizeof(ipp_val1[0]) ));
        CV_CALL( ipp_val2 = (float*)cvAlloc( max_count*sizeof(ipp_val2[0]) ));
        CV_CALL( ipp_counts = (int*)cvAlloc( max_count*sizeof(ipp_counts[0]) ));

        for( i = 0; i < cascade->count; i++ )
        {
            CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;
            for( j = 0, k = 0; j < stage_classifier->count; j++ )
            {
                CvHaarClassifier* classifier = stage_classifier->classifier + j;
                int rect_count = 2 + (classifier->haar_feature->rect[2].r.width != 0);

                ipp_thresholds[j] = classifier->threshold[0];
                ipp_val1[j] = classifier->alpha[0];
                ipp_val2[j] = classifier->alpha[1];
                ipp_counts[j] = rect_count;

                for( l = 0; l < rect_count; l++, k++ )
                {
                    ipp_features[k] = classifier->haar_feature->rect[l].r;
                    //ipp_features[k].y = orig_window_size.height - ipp_features[k].y - ipp_features[k].height;
                    ipp_weights[k] = classifier->haar_feature->rect[l].weight*ipp_weight_scale;
                }
            }

            if( icvHaarClassifierInitAlloc_32f_p( &out->ipp_stages[i],
                ipp_features, ipp_weights, ipp_thresholds,
                ipp_val1, ipp_val2, ipp_counts, stage_classifier->count ) < 0 )
                break;
        }

        if( i < cascade->count )
        {
            for( j = 0; j < i; j++ )
                if( icvHaarClassifierFree_32f_p && out->ipp_stages[i] )
                    icvHaarClassifierFree_32f_p( out->ipp_stages[i] );
            cvFree( &out->ipp_stages );
        }
    }
    }

    cascade->hid_cascade = out;
    assert( (char*)haar_node_ptr - (char*)out <= datasize );

    __END__;

    if( cvGetErrStatus() < 0 )
        icvReleaseHidHaarClassifierCascade( &out );

    cvFree( &ipp_features );
    cvFree( &ipp_weights );
    cvFree( &ipp_thresholds );
    cvFree( &ipp_val1 );
    cvFree( &ipp_val2 );
    cvFree( &ipp_counts );

    return out;
}


#define sum_elem_ptr(sum,row,col)  \
    ((sumtype*)CV_MAT_ELEM_PTR_FAST((sum),(row),(col),sizeof(sumtype)))

#define sqsum_elem_ptr(sqsum,row,col)  \
    ((sqsumtype*)CV_MAT_ELEM_PTR_FAST((sqsum),(row),(col),sizeof(sqsumtype)))

#define calc_sum(rect,offset) \
    ((rect).p0[offset] - (rect).p1[offset] - (rect).p2[offset] + (rect).p3[offset])


CV_IMPL void
cvSetImagesForHaarClassifierCascade( CvHaarClassifierCascade* _cascade,
                                     const CvArr* _sum,
                                     const CvArr* _sqsum,
                                     const CvArr* _tilted_sum,
                                     double scale )
{
    CV_FUNCNAME("cvSetImagesForHaarClassifierCascade");

    __BEGIN__;

    CvMat sum_stub, *sum = (CvMat*)_sum;
    CvMat sqsum_stub, *sqsum = (CvMat*)_sqsum;
    CvMat tilted_stub, *tilted = (CvMat*)_tilted_sum;
    CvHidHaarClassifierCascade* cascade;
    int coi0 = 0, coi1 = 0;
    int i;
    CvRect equ_rect;
    double weight_scale;

    if( !CV_IS_HAAR_CLASSIFIER(_cascade) )
        CV_ERROR( !_cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier pointer" );

    if( scale <= 0 )
        CV_ERROR( CV_StsOutOfRange, "Scale must be positive" );

    CV_CALL( sum = cvGetMat( sum, &sum_stub, &coi0 ));
    CV_CALL( sqsum = cvGetMat( sqsum, &sqsum_stub, &coi1 ));

    if( coi0 || coi1 )
        CV_ERROR( CV_BadCOI, "COI is not supported" );

    if( !CV_ARE_SIZES_EQ( sum, sqsum ))
        CV_ERROR( CV_StsUnmatchedSizes, "All integral images must have the same size" );

    if( CV_MAT_TYPE(sqsum->type) != CV_64FC1 ||
        CV_MAT_TYPE(sum->type) != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Only (32s, 64f, 32s) combination of (sum,sqsum,tilted_sum) formats is allowed" );

    if( !_cascade->hid_cascade )
        CV_CALL( icvCreateHidHaarClassifierCascade(_cascade) );

    cascade = _cascade->hid_cascade;

    if( cascade->has_tilted_features )
    {
        CV_CALL( tilted = cvGetMat( tilted, &tilted_stub, &coi1 ));

        if( CV_MAT_TYPE(tilted->type) != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Only (32s, 64f, 32s) combination of (sum,sqsum,tilted_sum) formats is allowed" );

        if( sum->step != tilted->step )
            CV_ERROR( CV_StsUnmatchedSizes,
            "Sum and tilted_sum must have the same stride (step, widthStep)" );

        if( !CV_ARE_SIZES_EQ( sum, tilted ))
            CV_ERROR( CV_StsUnmatchedSizes, "All integral images must have the same size" );
        cascade->tilted = *tilted;
    }

    _cascade->scale = scale;
    _cascade->real_window_size.width = cvRound( _cascade->orig_window_size.width * scale );
    _cascade->real_window_size.height = cvRound( _cascade->orig_window_size.height * scale );

    cascade->sum = *sum;
    cascade->sqsum = *sqsum;

    equ_rect.x = equ_rect.y = cvRound(scale);
    equ_rect.width = cvRound((_cascade->orig_window_size.width-2)*scale);
    equ_rect.height = cvRound((_cascade->orig_window_size.height-2)*scale);
    weight_scale = 1./(equ_rect.width*equ_rect.height);
    cascade->inv_window_area = weight_scale;

    cascade->p0 = sum_elem_ptr(*sum, equ_rect.y, equ_rect.x);
    cascade->p1 = sum_elem_ptr(*sum, equ_rect.y, equ_rect.x + equ_rect.width );
    cascade->p2 = sum_elem_ptr(*sum, equ_rect.y + equ_rect.height, equ_rect.x );
    cascade->p3 = sum_elem_ptr(*sum, equ_rect.y + equ_rect.height,
                                     equ_rect.x + equ_rect.width );

    cascade->pq0 = sqsum_elem_ptr(*sqsum, equ_rect.y, equ_rect.x);
    cascade->pq1 = sqsum_elem_ptr(*sqsum, equ_rect.y, equ_rect.x + equ_rect.width );
    cascade->pq2 = sqsum_elem_ptr(*sqsum, equ_rect.y + equ_rect.height, equ_rect.x );
    cascade->pq3 = sqsum_elem_ptr(*sqsum, equ_rect.y + equ_rect.height,
                                          equ_rect.x + equ_rect.width );

    /* init pointers in haar features according to real window size and
       given image pointers */
    {
#ifdef _OPENMP
    int max_threads = cvGetNumThreads();
    #pragma omp parallel for num_threads(max_threads) schedule(dynamic)
#endif // _OPENMP
    for( i = 0; i < _cascade->count; i++ )
    {
        int j, k, l;
        for( j = 0; j < cascade->stage_classifier[i].count; j++ )
        {
            for( l = 0; l < cascade->stage_classifier[i].classifier[j].count; l++ )
            {
                CvHaarFeature* feature =
                    &_cascade->stage_classifier[i].classifier[j].haar_feature[l];
                /* CvHidHaarClassifier* classifier =
                    cascade->stage_classifier[i].classifier + j; */
                CvHidHaarFeature* hidfeature =
                    &cascade->stage_classifier[i].classifier[j].node[l].feature;
                double sum0 = 0, area0 = 0;
                CvRect r[3];
#if CV_ADJUST_FEATURES
                int base_w = -1, base_h = -1;
                int new_base_w = 0, new_base_h = 0;
                int kx, ky;
                int flagx = 0, flagy = 0;
                int x0 = 0, y0 = 0;
#endif
                int nr;

                /* align blocks */
                for( k = 0; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    if( !hidfeature->rect[k].p0 )
                        break;
#if CV_ADJUST_FEATURES
                    r[k] = feature->rect[k].r;
                    base_w = (int)CV_IMIN( (unsigned)base_w, (unsigned)(r[k].width-1) );
                    base_w = (int)CV_IMIN( (unsigned)base_w, (unsigned)(r[k].x - r[0].x-1) );
                    base_h = (int)CV_IMIN( (unsigned)base_h, (unsigned)(r[k].height-1) );
                    base_h = (int)CV_IMIN( (unsigned)base_h, (unsigned)(r[k].y - r[0].y-1) );
#endif
                }

                nr = k;

#if CV_ADJUST_FEATURES
                base_w += 1;
                base_h += 1;
                kx = r[0].width / base_w;
                ky = r[0].height / base_h;

                if( kx <= 0 )
                {
                    flagx = 1;
                    new_base_w = cvRound( r[0].width * scale ) / kx;
                    x0 = cvRound( r[0].x * scale );
                }

                if( ky <= 0 )
                {
                    flagy = 1;
                    new_base_h = cvRound( r[0].height * scale ) / ky;
                    y0 = cvRound( r[0].y * scale );
                }
#endif

                for( k = 0; k < nr; k++ )
                {
                    CvRect tr;
                    double correction_ratio;

#if CV_ADJUST_FEATURES
                    if( flagx )
                    {
                        tr.x = (r[k].x - r[0].x) * new_base_w / base_w + x0;
                        tr.width = r[k].width * new_base_w / base_w;
                    }
                    else
#endif
                    {
                        tr.x = cvRound( r[k].x * scale );
                        tr.width = cvRound( r[k].width * scale );
                    }

#if CV_ADJUST_FEATURES
                    if( flagy )
                    {
                        tr.y = (r[k].y - r[0].y) * new_base_h / base_h + y0;
                        tr.height = r[k].height * new_base_h / base_h;
                    }
                    else
#endif
                    {
                        tr.y = cvRound( r[k].y * scale );
                        tr.height = cvRound( r[k].height * scale );
                    }

#if CV_ADJUST_WEIGHTS
                    {
                    // RAINER START
                    const float orig_feature_size =  (float)(feature->rect[k].r.width)*feature->rect[k].r.height;
                    const float orig_norm_size = (float)(_cascade->orig_window_size.width)*(_cascade->orig_window_size.height);
                    const float feature_size = float(tr.width*tr.height);
                    //const float normSize    = float(equ_rect.width*equ_rect.height);
                    float target_ratio = orig_feature_size / orig_norm_size;
                    //float isRatio = featureSize / normSize;
                    //correctionRatio = targetRatio / isRatio / normSize;
                    correction_ratio = target_ratio / feature_size;
                    // RAINER END
                    }
#else
                    correction_ratio = weight_scale * (!feature->tilted ? 1 : 0.5);
#endif

                    if( !feature->tilted )
                    {
                        hidfeature->rect[k].p0 = sum_elem_ptr(*sum, tr.y, tr.x);
                        hidfeature->rect[k].p1 = sum_elem_ptr(*sum, tr.y, tr.x + tr.width);
                        hidfeature->rect[k].p2 = sum_elem_ptr(*sum, tr.y + tr.height, tr.x);
                        hidfeature->rect[k].p3 = sum_elem_ptr(*sum, tr.y + tr.height, tr.x + tr.width);
                    }
                    else
                    {
                        hidfeature->rect[k].p2 = sum_elem_ptr(*tilted, tr.y + tr.width, tr.x + tr.width);
                        hidfeature->rect[k].p3 = sum_elem_ptr(*tilted, tr.y + tr.width + tr.height,
                                                              tr.x + tr.width - tr.height);
                        hidfeature->rect[k].p0 = sum_elem_ptr(*tilted, tr.y, tr.x);
                        hidfeature->rect[k].p1 = sum_elem_ptr(*tilted, tr.y + tr.height, tr.x - tr.height);
                    }

                    hidfeature->rect[k].weight = (float)(feature->rect[k].weight * correction_ratio);

                    if( k == 0 )
                        area0 = tr.width * tr.height;
                    else
                        sum0 += hidfeature->rect[k].weight * tr.width * tr.height;
                }

                hidfeature->rect[0].weight = (float)(-sum0/area0);
            } /* l */
        } /* j */
    }
    }

    __END__;
}


CV_INLINE
double icvEvalHidHaarClassifier( CvHidHaarClassifier* classifier,
                                 double variance_norm_factor,
                                 size_t p_offset )
{
    int idx = 0;
    do
    {
        CvHidHaarTreeNode* node = classifier->node + idx;
        double t = node->threshold * variance_norm_factor;

        double sum = calc_sum(node->feature.rect[0],p_offset) * node->feature.rect[0].weight;
        sum += calc_sum(node->feature.rect[1],p_offset) * node->feature.rect[1].weight;

        if( node->feature.rect[2].p0 )
            sum += calc_sum(node->feature.rect[2],p_offset) * node->feature.rect[2].weight;

        idx = sum < t ? node->left : node->right;
    }
    while( idx > 0 );
    return classifier->alpha[-idx];
}


CV_IMPL int
cvRunHaarClassifierCascade( CvHaarClassifierCascade* _cascade,
                            CvPoint pt, int start_stage )
{
    int result = -1;
    CV_FUNCNAME("cvRunHaarClassifierCascade");

    __BEGIN__;

    int p_offset, pq_offset;
    int i, j;
    double mean, variance_norm_factor;
    CvHidHaarClassifierCascade* cascade;

    if( !CV_IS_HAAR_CLASSIFIER(_cascade) )
        CV_ERROR( !_cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid cascade pointer" );

    cascade = _cascade->hid_cascade;
    if( !cascade )
        CV_ERROR( CV_StsNullPtr, "Hidden cascade has not been created.\n"
            "Use cvSetImagesForHaarClassifierCascade" );

    if( pt.x < 0 || pt.y < 0 ||
        pt.x + _cascade->real_window_size.width >= cascade->sum.width-2 ||
        pt.y + _cascade->real_window_size.height >= cascade->sum.height-2 )
        EXIT;

    p_offset = pt.y * (cascade->sum.step/sizeof(sumtype)) + pt.x;
    pq_offset = pt.y * (cascade->sqsum.step/sizeof(sqsumtype)) + pt.x;
    mean = calc_sum(*cascade,p_offset)*cascade->inv_window_area;
    variance_norm_factor = cascade->pq0[pq_offset] - cascade->pq1[pq_offset] -
                           cascade->pq2[pq_offset] + cascade->pq3[pq_offset];
    variance_norm_factor = variance_norm_factor*cascade->inv_window_area - mean*mean;
    if( variance_norm_factor >= 0. )
        variance_norm_factor = sqrt(variance_norm_factor);
    else
        variance_norm_factor = 1.;

    if( cascade->is_tree )
    {
        CvHidHaarStageClassifier* ptr;
        assert( start_stage == 0 );

        result = 1;
        ptr = cascade->stage_classifier;

        while( ptr )
        {
            double stage_sum = 0;

            for( j = 0; j < ptr->count; j++ )
            {
                stage_sum += icvEvalHidHaarClassifier( ptr->classifier + j,
                    variance_norm_factor, p_offset );
            }

            if( stage_sum >= ptr->threshold )
            {
                ptr = ptr->child;
            }
            else
            {
                while( ptr && ptr->next == NULL ) ptr = ptr->parent;
                if( ptr == NULL )
                {
                    result = 0;
                    EXIT;
                }
                ptr = ptr->next;
            }
        }
    }
    else if( cascade->is_stump_based )
    {
        for( i = start_stage; i < cascade->count; i++ )
        {
            double stage_sum = 0;

            if( cascade->stage_classifier[i].two_rects )
            {
                for( j = 0; j < cascade->stage_classifier[i].count; j++ )
                {
                    CvHidHaarClassifier* classifier = cascade->stage_classifier[i].classifier + j;
                    CvHidHaarTreeNode* node = classifier->node;
                    double sum, t = node->threshold*variance_norm_factor, a, b;

                    sum = calc_sum(node->feature.rect[0],p_offset) * node->feature.rect[0].weight;
                    sum += calc_sum(node->feature.rect[1],p_offset) * node->feature.rect[1].weight;

                    a = classifier->alpha[0];
                    b = classifier->alpha[1];
                    stage_sum += sum < t ? a : b;
                }
            }
            else
            {
                for( j = 0; j < cascade->stage_classifier[i].count; j++ )
                {
                    CvHidHaarClassifier* classifier = cascade->stage_classifier[i].classifier + j;
                    CvHidHaarTreeNode* node = classifier->node;
                    double sum, t = node->threshold*variance_norm_factor, a, b;

                    sum = calc_sum(node->feature.rect[0],p_offset) * node->feature.rect[0].weight;
                    sum += calc_sum(node->feature.rect[1],p_offset) * node->feature.rect[1].weight;

                    if( node->feature.rect[2].p0 )
                        sum += calc_sum(node->feature.rect[2],p_offset) * node->feature.rect[2].weight;

                    a = classifier->alpha[0];
                    b = classifier->alpha[1];
                    stage_sum += sum < t ? a : b;
                }
            }

            if( stage_sum < cascade->stage_classifier[i].threshold )
            {
                result = -i;
                EXIT;
            }
        }
    }
    else
    {
        for( i = start_stage; i < cascade->count; i++ )
        {
            double stage_sum = 0;

            for( j = 0; j < cascade->stage_classifier[i].count; j++ )
            {
                stage_sum += icvEvalHidHaarClassifier(
                    cascade->stage_classifier[i].classifier + j,
                    variance_norm_factor, p_offset );
            }

            if( stage_sum < cascade->stage_classifier[i].threshold )
            {
                result = -i;
                EXIT;
            }
        }
    }

    result = 1;

    __END__;

    return result;
}


static int is_equal( const void* _r1, const void* _r2, void* )
{
    const CvRect* r1 = (const CvRect*)_r1;
    const CvRect* r2 = (const CvRect*)_r2;
    int distance = cvRound(r1->width*0.2);

    return r2->x <= r1->x + distance &&
           r2->x >= r1->x - distance &&
           r2->y <= r1->y + distance &&
           r2->y >= r1->y - distance &&
           r2->width <= cvRound( r1->width * 1.2 ) &&
           cvRound( r2->width * 1.2 ) >= r1->width;
}


#define VERY_ROUGH_SEARCH 0

CV_IMPL CvSeq*
cvHaarDetectObjects( const CvArr* _img,
                     CvHaarClassifierCascade* cascade,
                     CvMemStorage* storage, double scale_factor,
                     int min_neighbors, int flags, CvSize min_size )
{
    int split_stage = 2;

    CvMat stub, *img = (CvMat*)_img;
    CvMat *temp = 0, *sum = 0, *tilted = 0, *sqsum = 0, *norm_img = 0, *sumcanny = 0, *img_small = 0;
    CvSeq* result_seq = 0;
    CvMemStorage* temp_storage = 0;
    CvAvgComp* comps = 0;
    CvSeq* seq_thread[CV_MAX_THREADS] = {0};
    int i, max_threads = 0;

    CV_FUNCNAME( "cvHaarDetectObjects" );

    __BEGIN__;

    CvSeq *seq = 0, *seq2 = 0, *idx_seq = 0, *big_seq = 0;
    CvAvgComp result_comp = {{0,0,0,0},0};
    double factor;
    int npass = 2, coi;
    bool do_canny_pruning = (flags & CV_HAAR_DO_CANNY_PRUNING) != 0;
    bool find_biggest_object = (flags & CV_HAAR_FIND_BIGGEST_OBJECT) != 0;
    bool rough_search = (flags & CV_HAAR_DO_ROUGH_SEARCH) != 0;

    if( !CV_IS_HAAR_CLASSIFIER(cascade) )
        CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "Null storage pointer" );

    CV_CALL( img = cvGetMat( img, &stub, &coi ));
    if( coi )
        CV_ERROR( CV_BadCOI, "COI is not supported" );

    if( CV_MAT_DEPTH(img->type) != CV_8U )
        CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

    if( find_biggest_object )
        flags &= ~CV_HAAR_SCALE_IMAGE;

    CV_CALL( temp = cvCreateMat( img->rows, img->cols, CV_8UC1 ));
    CV_CALL( sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 ));
    CV_CALL( sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 ));
    CV_CALL( temp_storage = cvCreateChildMemStorage( storage ));

    if( !cascade->hid_cascade )
        CV_CALL( icvCreateHidHaarClassifierCascade(cascade) );

    if( cascade->hid_cascade->has_tilted_features )
        tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

    seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
    seq2 = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), temp_storage );
    result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

    max_threads = cvGetNumThreads();
    if( max_threads > 1 )
        for( i = 0; i < max_threads; i++ )
        {
            CvMemStorage* temp_storage_thread;
            CV_CALL( temp_storage_thread = cvCreateMemStorage(0));
            CV_CALL( seq_thread[i] = cvCreateSeq( 0, sizeof(CvSeq),
                sizeof(CvRect), temp_storage_thread ));
        }
    else
        seq_thread[0] = seq;

    if( CV_MAT_CN(img->type) > 1 )
    {
        cvCvtColor( img, temp, CV_BGR2GRAY );
        img = temp;
    }

    if( flags & CV_HAAR_FIND_BIGGEST_OBJECT )
        flags &= ~(CV_HAAR_SCALE_IMAGE|CV_HAAR_DO_CANNY_PRUNING);

    if( flags & CV_HAAR_SCALE_IMAGE )
    {
        CvSize win_size0 = cascade->orig_window_size;
        int use_ipp = cascade->hid_cascade->ipp_stages != 0 &&
                    icvApplyHaarClassifier_32f_C1R_p != 0;

        if( use_ipp )
            CV_CALL( norm_img = cvCreateMat( img->rows, img->cols, CV_32FC1 ));
        CV_CALL( img_small = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 ));

        for( factor = 1; ; factor *= scale_factor )
        {
            int strip_count, strip_size;
            int ystep = factor > 2. ? 1 : 2;
            CvSize win_size = { cvRound(win_size0.width*factor),
                                cvRound(win_size0.height*factor) };
            CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
            CvSize sz1 = { sz.width - win_size0.width, sz.height - win_size0.height };
            CvRect equ_rect = { icv_object_win_border, icv_object_win_border,
                win_size0.width - icv_object_win_border*2,
                win_size0.height - icv_object_win_border*2 };
            CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
            CvMat* _tilted = 0;

            if( sz1.width <= 0 || sz1.height <= 0 )
                break;
            if( win_size.width < min_size.width || win_size.height < min_size.height )
                continue;

            img1 = cvMat( sz.height, sz.width, CV_8UC1, img_small->data.ptr );
            sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
            sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
            if( tilted )
            {
                tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
                _tilted = &tilted1;
            }
            norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, norm_img ? norm_img->data.ptr : 0 );
            mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );

            cvResize( img, &img1, CV_INTER_LINEAR );
            cvIntegral( &img1, &sum1, &sqsum1, _tilted );

            if( max_threads > 1 )
            {
                strip_count = MAX(MIN(sz1.height/ystep, max_threads*3), 1);
                strip_size = (sz1.height + strip_count - 1)/strip_count;
                strip_size = (strip_size / ystep)*ystep;
            }
            else
            {
                strip_count = 1;
                strip_size = sz1.height;
            }

            if( !use_ipp )
                cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, 0, 1. );
            else
            {
                for( i = 0; i <= sz.height; i++ )
                {
                    const int* isum = (int*)(sum1.data.ptr + sum1.step*i);
                    float* fsum = (float*)isum;
                    const int FLT_DELTA = -(1 << 24);
                    int j;
                    for( j = 0; j <= sz.width; j++ )
                        fsum[j] = (float)(isum[j] + FLT_DELTA);
                }
            }

        #ifdef _OPENMP
            #pragma omp parallel for num_threads(max_threads) schedule(dynamic)
        #endif
            for( i = 0; i < strip_count; i++ )
            {
                int thread_id = cvGetThreadNum();
                int positive = 0;
                int y1 = i*strip_size, y2 = (i+1)*strip_size/* - ystep + 1*/;
                CvSize ssz;
                int x, y, j;
                if( i == strip_count - 1 || y2 > sz1.height )
                    y2 = sz1.height;
                ssz = cvSize(sz1.width, y2 - y1);

                if( use_ipp )
                {
                    icvRectStdDev_32f_C1R_p(
                        (float*)(sum1.data.ptr + y1*sum1.step), sum1.step,
                        (double*)(sqsum1.data.ptr + y1*sqsum1.step), sqsum1.step,
                        (float*)(norm1.data.ptr + y1*norm1.step), norm1.step, ssz, equ_rect );

                    positive = (ssz.width/ystep)*((ssz.height + ystep-1)/ystep);
                    memset( mask1.data.ptr + y1*mask1.step, ystep == 1, mask1.height*mask1.step);
                    
                    if( ystep > 1 )
                    {
                        for( y = y1, positive = 0; y < y2; y += ystep )
                            for( x = 0; x < ssz.width; x += ystep )
                                mask1.data.ptr[mask1.step*y + x] = (uchar)1;
                    }

                    for( j = 0; j < cascade->count; j++ )
                    {
                        if( icvApplyHaarClassifier_32f_C1R_p(
                            (float*)(sum1.data.ptr + y1*sum1.step), sum1.step,
                            (float*)(norm1.data.ptr + y1*norm1.step), norm1.step,
                            mask1.data.ptr + y1*mask1.step, mask1.step, ssz, &positive,
                            cascade->hid_cascade->stage_classifier[j].threshold,
                            cascade->hid_cascade->ipp_stages[j]) < 0 )
                        {
                            positive = 0;
                            break;
                        }
                        if( positive <= 0 )
                            break;
                    }
                }
                else
                {
                    for( y = y1, positive = 0; y < y2; y += ystep )
                        for( x = 0; x < ssz.width; x += ystep )
                        {
                            mask1.data.ptr[mask1.step*y + x] =
                                cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0;
                            positive += mask1.data.ptr[mask1.step*y + x];
                        }
                }

                if( positive > 0 )
                {
                    for( y = y1; y < y2; y += ystep )
                        for( x = 0; x < ssz.width; x += ystep )
                            if( mask1.data.ptr[mask1.step*y + x] != 0 )
                            {
                                CvRect obj_rect = { cvRound(x*factor), cvRound(y*factor),
                                                    win_size.width, win_size.height };
                                cvSeqPush( seq_thread[thread_id], &obj_rect );
                            }
                }
            }

            // gather the results
            if( max_threads > 1 )
                for( i = 0; i < max_threads; i++ )
                {
                    CvSeq* s = seq_thread[i];
                    int j, total = s->total;
                    CvSeqBlock* b = s->first;
                    for( j = 0; j < total; j += b->count, b = b->next )
                        cvSeqPushMulti( seq, b->data, b->count );
                }
        }
    }
    else
    {
        int n_factors = 0;
        CvRect scan_roi_rect = {0,0,0,0};
        bool is_found = false, scan_roi = false;

        cvIntegral( img, sum, sqsum, tilted );

        if( do_canny_pruning )
        {
            sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
            cvCanny( img, temp, 0, 50, 3 );
            cvIntegral( temp, sumcanny );
        }

        if( (unsigned)split_stage >= (unsigned)cascade->count ||
            cascade->hid_cascade->is_tree )
        {
            split_stage = cascade->count;
            npass = 1;
        }

        for( n_factors = 0, factor = 1;
             factor*cascade->orig_window_size.width < img->cols - 10 &&
             factor*cascade->orig_window_size.height < img->rows - 10;
             n_factors++, factor *= scale_factor )
            ;

        if( find_biggest_object )
        {
            scale_factor = 1./scale_factor;
            factor *= scale_factor;
            big_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
        }
        else
            factor = 1;

        for( ; n_factors-- > 0 && !is_found; factor *= scale_factor )
        {
            const double ystep = MAX( 2, factor );
            CvSize win_size = { cvRound( cascade->orig_window_size.width * factor ),
                                cvRound( cascade->orig_window_size.height * factor )};
            CvRect equ_rect = { 0, 0, 0, 0 };
            int *p0 = 0, *p1 = 0, *p2 = 0, *p3 = 0;
            int *pq0 = 0, *pq1 = 0, *pq2 = 0, *pq3 = 0;
            int pass, stage_offset = 0;
            int start_x = 0, start_y = 0;
            int end_x = cvRound((img->cols - win_size.width) / ystep);
            int end_y = cvRound((img->rows - win_size.height) / ystep);

            if( win_size.width < min_size.width || win_size.height < min_size.height )
            {
                if( find_biggest_object )
                    break;
                continue;
            }

            cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
            cvZero( temp );

            if( do_canny_pruning )
            {
                equ_rect.x = cvRound(win_size.width*0.15);
                equ_rect.y = cvRound(win_size.height*0.15);
                equ_rect.width = cvRound(win_size.width*0.7);
                equ_rect.height = cvRound(win_size.height*0.7);

                p0 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step) + equ_rect.x;
                p1 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step)
                            + equ_rect.x + equ_rect.width;
                p2 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step) + equ_rect.x;
                p3 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step)
                            + equ_rect.x + equ_rect.width;

                pq0 = (int*)(sum->data.ptr + equ_rect.y*sum->step) + equ_rect.x;
                pq1 = (int*)(sum->data.ptr + equ_rect.y*sum->step)
                            + equ_rect.x + equ_rect.width;
                pq2 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step) + equ_rect.x;
                pq3 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step)
                            + equ_rect.x + equ_rect.width;
            }

            if( scan_roi )
            {
                //adjust start_height and stop_height
                start_y = cvRound(scan_roi_rect.y / ystep);
                end_y = cvRound((scan_roi_rect.y + scan_roi_rect.height - win_size.height) / ystep);

                start_x = cvRound(scan_roi_rect.x / ystep);
                end_x = cvRound((scan_roi_rect.x + scan_roi_rect.width - win_size.width) / ystep);
            }

            cascade->hid_cascade->count = split_stage;

            for( pass = 0; pass < npass; pass++ )
            {
            #ifdef _OPENMP
                #pragma omp parallel for num_threads(max_threads) schedule(dynamic)
            #endif
                for( int _iy = start_y; _iy < end_y; _iy++ )
                {
                    int thread_id = cvGetThreadNum();
                    int iy = cvRound(_iy*ystep);
                    int _ix, _xstep = 1;
                    uchar* mask_row = temp->data.ptr + temp->step * iy;

                    for( _ix = start_x; _ix < end_x; _ix += _xstep )
                    {
                        int ix = cvRound(_ix*ystep); // it really should be ystep

                        if( pass == 0 )
                        {
                            int result;
                            _xstep = 2;

                            if( do_canny_pruning )
                            {
                                int offset;
                                int s, sq;

                                offset = iy*(sum->step/sizeof(p0[0])) + ix;
                                s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
                                sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
                                if( s < 100 || sq < 20 )
                                    continue;
                            }

                            result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy), 0 );
                            if( result > 0 )
                            {
                                if( pass < npass - 1 )
                                    mask_row[ix] = 1;
                                else
                                {
                                    CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
                                    cvSeqPush( seq_thread[thread_id], &rect );
                                }
                            }
                            if( result < 0 )
                                _xstep = 1;
                        }
                        else if( mask_row[ix] )
                        {
                            int result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy),
                                                                     stage_offset );
                            if( result > 0 )
                            {
                                if( pass == npass - 1 )
                                {
                                    CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
                                    cvSeqPush( seq_thread[thread_id], &rect );
                                }
                            }
                            else
                                mask_row[ix] = 0;
                        }
                    }
                }
                stage_offset = cascade->hid_cascade->count;
                cascade->hid_cascade->count = cascade->count;
            }

            // gather the results
            if( max_threads > 1 )
	            for( i = 0; i < max_threads; i++ )
	            {
		            CvSeq* s = seq_thread[i];
                    int j, total = s->total;
                    CvSeqBlock* b = s->first;
                    for( j = 0; j < total; j += b->count, b = b->next )
                        cvSeqPushMulti( seq, b->data, b->count );
	            }

            if( find_biggest_object )
            {
                CvSeq* bseq = min_neighbors > 0 ? big_seq : seq;
                
                if( min_neighbors > 0 && !scan_roi )
                {
                    // group retrieved rectangles in order to filter out noise
                    int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
                    CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
                    memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

                #if VERY_ROUGH_SEARCH
                    if( rough_search )
                    {
                        for( i = 0; i < seq->total; i++ )
                        {
                            CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
                            int idx = *(int*)cvGetSeqElem( idx_seq, i );
                            assert( (unsigned)idx < (unsigned)ncomp );

                            comps[idx].neighbors++;
                            comps[idx].rect.x += r1.x;
                            comps[idx].rect.y += r1.y;
                            comps[idx].rect.width += r1.width;
                            comps[idx].rect.height += r1.height;
                        }

                        // calculate average bounding box
                        for( i = 0; i < ncomp; i++ )
                        {
                            int n = comps[i].neighbors;
                            if( n >= min_neighbors )
                            {
                                CvAvgComp comp;
                                comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
                                comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
                                comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
                                comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
                                comp.neighbors = n;
                                cvSeqPush( bseq, &comp );
                            }
                        }
                    }
                    else
                #endif
                    {
                        for( i = 0 ; i <= ncomp; i++ )
                            comps[i].rect.x = comps[i].rect.y = INT_MAX;

                        // count number of neighbors
                        for( i = 0; i < seq->total; i++ )
                        {
                            CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
                            int idx = *(int*)cvGetSeqElem( idx_seq, i );
                            assert( (unsigned)idx < (unsigned)ncomp );

                            comps[idx].neighbors++;

                            // rect.width and rect.height will store coordinate of right-bottom corner
                            comps[idx].rect.x = MIN(comps[idx].rect.x, r1.x);
                            comps[idx].rect.y = MIN(comps[idx].rect.y, r1.y);
                            comps[idx].rect.width = MAX(comps[idx].rect.width, r1.x+r1.width-1);
                            comps[idx].rect.height = MAX(comps[idx].rect.height, r1.y+r1.height-1);
                        }

                        // calculate enclosing box
                        for( i = 0; i < ncomp; i++ )
                        {
                            int n = comps[i].neighbors;
                            if( n >= min_neighbors )
                            {
                                CvAvgComp comp;
                                int t;
                                double min_scale = rough_search ? 0.6 : 0.4;
                                comp.rect.x = comps[i].rect.x;
                                comp.rect.y = comps[i].rect.y;
                                comp.rect.width = comps[i].rect.width - comps[i].rect.x + 1;
                                comp.rect.height = comps[i].rect.height - comps[i].rect.y + 1;

                                // update min_size
                                t = cvRound( comp.rect.width*min_scale );
                                min_size.width = MAX( min_size.width, t );

                                t = cvRound( comp.rect.height*min_scale );
                                min_size.height = MAX( min_size.height, t );

                                //expand the box by 20% because we could miss some neighbours
                                //see 'is_equal' function
                            #if 1
                                int offset = cvRound(comp.rect.width * 0.2);
                                int right = MIN( img->cols-1, comp.rect.x+comp.rect.width-1 + offset );
                                int bottom = MIN( img->rows-1, comp.rect.y+comp.rect.height-1 + offset);
                                comp.rect.x = MAX( comp.rect.x - offset, 0 );
                                comp.rect.y = MAX( comp.rect.y - offset, 0 );
                                comp.rect.width = right - comp.rect.x + 1;
                                comp.rect.height = bottom - comp.rect.y + 1;
                            #endif

                                comp.neighbors = n;
                                cvSeqPush( bseq, &comp );
                            }
                        }
                    }

                    cvFree( &comps );
                }

                // extract the biggest rect
                if( bseq->total > 0 )
                {
                    int max_area = 0;
                    for( i = 0; i < bseq->total; i++ )
                    {
                        CvAvgComp* comp = (CvAvgComp*)cvGetSeqElem( bseq, i );
                        int area = comp->rect.width * comp->rect.height;
                        if( max_area < area )
                        {
                            max_area = area;
                            result_comp.rect = comp->rect;
                            result_comp.neighbors = bseq == seq ? 1 : comp->neighbors;
                        }
                    }

                    //Prepare information for further scanning inside the biggest rectangle

                #if VERY_ROUGH_SEARCH
                    // change scan ranges to roi in case of required
                    if( !rough_search && !scan_roi )
                    {
                        scan_roi = true;
                        scan_roi_rect = result_comp.rect;
                        cvClearSeq(bseq);
                    }
                    else if( rough_search )
                        is_found = true;
                #else
                    if( !scan_roi )
                    {
                        scan_roi = true;
                        scan_roi_rect = result_comp.rect;
                        cvClearSeq(bseq);
                    }
                #endif
                }
            }
        }
    }

    if( min_neighbors == 0 && !find_biggest_object )
    {
        for( i = 0; i < seq->total; i++ )
        {
            CvRect* rect = (CvRect*)cvGetSeqElem( seq, i );
            CvAvgComp comp;
            comp.rect = *rect;
            comp.neighbors = 1;
            cvSeqPush( result_seq, &comp );
        }
    }

    if( min_neighbors != 0
#if VERY_ROUGH_SEARCH        
        && (!find_biggest_object || !rough_search)
#endif        
        )
    {
        // group retrieved rectangles in order to filter out noise
        int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
        CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
        memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

        // count number of neighbors
        for( i = 0; i < seq->total; i++ )
        {
            CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
            int idx = *(int*)cvGetSeqElem( idx_seq, i );
            assert( (unsigned)idx < (unsigned)ncomp );

            comps[idx].neighbors++;

            comps[idx].rect.x += r1.x;
            comps[idx].rect.y += r1.y;
            comps[idx].rect.width += r1.width;
            comps[idx].rect.height += r1.height;
        }

        // calculate average bounding box
        for( i = 0; i < ncomp; i++ )
        {
            int n = comps[i].neighbors;
            if( n >= min_neighbors )
            {
                CvAvgComp comp;
                comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
                comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
                comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
                comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
                comp.neighbors = comps[i].neighbors;

                cvSeqPush( seq2, &comp );
            }
        }

        if( !find_biggest_object )
        {
            // filter out small face rectangles inside large face rectangles
            for( i = 0; i < seq2->total; i++ )
            {
                CvAvgComp r1 = *(CvAvgComp*)cvGetSeqElem( seq2, i );
                int j, flag = 1;

                for( j = 0; j < seq2->total; j++ )
                {
                    CvAvgComp r2 = *(CvAvgComp*)cvGetSeqElem( seq2, j );
                    int distance = cvRound( r2.rect.width * 0.2 );

                    if( i != j &&
                        r1.rect.x >= r2.rect.x - distance &&
                        r1.rect.y >= r2.rect.y - distance &&
                        r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
                        r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
                        (r2.neighbors > MAX( 3, r1.neighbors ) || r1.neighbors < 3) )
                    {
                        flag = 0;
                        break;
                    }
                }

                if( flag )
                    cvSeqPush( result_seq, &r1 );
            }
        }
        else
        {
            int max_area = 0;
            for( i = 0; i < seq2->total; i++ )
            {
                CvAvgComp* comp = (CvAvgComp*)cvGetSeqElem( seq2, i );
                int area = comp->rect.width * comp->rect.height;
                if( max_area < area )
                {
                    max_area = area;
                    result_comp = *comp;
                }                
            }
        }
    }

    if( find_biggest_object && result_comp.rect.width > 0 )
        cvSeqPush( result_seq, &result_comp );

    __END__;

    if( max_threads > 1 )
	    for( i = 0; i < max_threads; i++ )
	    {
		    if( seq_thread[i] )
                cvReleaseMemStorage( &seq_thread[i]->storage );
	    }

    cvReleaseMemStorage( &temp_storage );
    cvReleaseMat( &sum );
    cvReleaseMat( &sqsum );
    cvReleaseMat( &tilted );
    cvReleaseMat( &temp );
    cvReleaseMat( &sumcanny );
    cvReleaseMat( &norm_img );
    cvReleaseMat( &img_small );
    cvFree( &comps );

    return result_seq;
}


static CvHaarClassifierCascade*
icvLoadCascadeCART( const char** input_cascade, int n, CvSize orig_window_size )
{
    int i;
    CvHaarClassifierCascade* cascade = icvCreateHaarClassifierCascade(n);
    cascade->orig_window_size = orig_window_size;

    for( i = 0; i < n; i++ )
    {
        int j, count, l;
        float threshold = 0;
        const char* stage = input_cascade[i];
        int dl = 0;

        /* tree links */
        int parent = -1;
        int next = -1;

        sscanf( stage, "%d%n", &count, &dl );
        stage += dl;

        assert( count > 0 );
        cascade->stage_classifier[i].count = count;
        cascade->stage_classifier[i].classifier =
            (CvHaarClassifier*)cvAlloc( count*sizeof(cascade->stage_classifier[i].classifier[0]));

        for( j = 0; j < count; j++ )
        {
            CvHaarClassifier* classifier = cascade->stage_classifier[i].classifier + j;
            int k, rects = 0;
            char str[100];

            sscanf( stage, "%d%n", &classifier->count, &dl );
            stage += dl;

            classifier->haar_feature = (CvHaarFeature*) cvAlloc(
                classifier->count * ( sizeof( *classifier->haar_feature ) +
                                      sizeof( *classifier->threshold ) +
                                      sizeof( *classifier->left ) +
                                      sizeof( *classifier->right ) ) +
                (classifier->count + 1) * sizeof( *classifier->alpha ) );
            classifier->threshold = (float*) (classifier->haar_feature+classifier->count);
            classifier->left = (int*) (classifier->threshold + classifier->count);
            classifier->right = (int*) (classifier->left + classifier->count);
            classifier->alpha = (float*) (classifier->right + classifier->count);

            for( l = 0; l < classifier->count; l++ )
            {
                sscanf( stage, "%d%n", &rects, &dl );
                stage += dl;

                assert( rects >= 2 && rects <= CV_HAAR_FEATURE_MAX );

                for( k = 0; k < rects; k++ )
                {
                    CvRect r;
                    int band = 0;
                    sscanf( stage, "%d%d%d%d%d%f%n",
                            &r.x, &r.y, &r.width, &r.height, &band,
                            &(classifier->haar_feature[l].rect[k].weight), &dl );
                    stage += dl;
                    classifier->haar_feature[l].rect[k].r = r;
                }
                sscanf( stage, "%s%n", str, &dl );
                stage += dl;

                classifier->haar_feature[l].tilted = strncmp( str, "tilted", 6 ) == 0;

                for( k = rects; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    memset( classifier->haar_feature[l].rect + k, 0,
                            sizeof(classifier->haar_feature[l].rect[k]) );
                }

                sscanf( stage, "%f%d%d%n", &(classifier->threshold[l]),
                                       &(classifier->left[l]),
                                       &(classifier->right[l]), &dl );
                stage += dl;
            }
            for( l = 0; l <= classifier->count; l++ )
            {
                sscanf( stage, "%f%n", &(classifier->alpha[l]), &dl );
                stage += dl;
            }
        }

        sscanf( stage, "%f%n", &threshold, &dl );
        stage += dl;

        cascade->stage_classifier[i].threshold = threshold;

        /* load tree links */
        if( sscanf( stage, "%d%d%n", &parent, &next, &dl ) != 2 )
        {
            parent = i - 1;
            next = -1;
        }
        stage += dl;

        cascade->stage_classifier[i].parent = parent;
        cascade->stage_classifier[i].next = next;
        cascade->stage_classifier[i].child = -1;

        if( parent != -1 && cascade->stage_classifier[parent].child == -1 )
        {
            cascade->stage_classifier[parent].child = i;
        }
    }

    return cascade;
}

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif

CV_IMPL CvHaarClassifierCascade*
cvLoadHaarClassifierCascade( const char* directory, CvSize orig_window_size )
{
    const char** input_cascade = 0;
    CvHaarClassifierCascade *cascade = 0;

    CV_FUNCNAME( "cvLoadHaarClassifierCascade" );

    __BEGIN__;

    int i, n;
    const char* slash;
    char name[_MAX_PATH];
    int size = 0;
    char* ptr = 0;

    if( !directory )
        CV_ERROR( CV_StsNullPtr, "Null path is passed" );

    n = (int)strlen(directory)-1;
    slash = directory[n] == '\\' || directory[n] == '/' ? "" : "/";

    /* try to read the classifier from directory */
    for( n = 0; ; n++ )
    {
        sprintf( name, "%s%s%d/AdaBoostCARTHaarClassifier.txt", directory, slash, n );
        FILE* f = fopen( name, "rb" );
        if( !f )
            break;
        fseek( f, 0, SEEK_END );
        size += ftell( f ) + 1;
        fclose(f);
    }

    if( n == 0 && slash[0] )
    {
        CV_CALL( cascade = (CvHaarClassifierCascade*)cvLoad( directory ));
        EXIT;
    }
    else if( n == 0 )
        CV_ERROR( CV_StsBadArg, "Invalid path" );

    size += (n+1)*sizeof(char*);
    CV_CALL( input_cascade = (const char**)cvAlloc( size ));
    ptr = (char*)(input_cascade + n + 1);

    for( i = 0; i < n; i++ )
    {
        sprintf( name, "%s/%d/AdaBoostCARTHaarClassifier.txt", directory, i );
        FILE* f = fopen( name, "rb" );
        if( !f )
            CV_ERROR( CV_StsError, "" );
        fseek( f, 0, SEEK_END );
        size = ftell( f );
        fseek( f, 0, SEEK_SET );
        fread( ptr, 1, size, f );
        fclose(f);
        input_cascade[i] = ptr;
        ptr += size;
        *ptr++ = '\0';
    }

    input_cascade[n] = 0;
    cascade = icvLoadCascadeCART( input_cascade, n, orig_window_size );

    __END__;

    if( input_cascade )
        cvFree( &input_cascade );

    if( cvGetErrStatus() < 0 )
        cvReleaseHaarClassifierCascade( &cascade );

    return cascade;
}


CV_IMPL void
cvReleaseHaarClassifierCascade( CvHaarClassifierCascade** _cascade )
{
    if( _cascade && *_cascade )
    {
        int i, j;
        CvHaarClassifierCascade* cascade = *_cascade;

        for( i = 0; i < cascade->count; i++ )
        {
            for( j = 0; j < cascade->stage_classifier[i].count; j++ )
                cvFree( &cascade->stage_classifier[i].classifier[j].haar_feature );
            cvFree( &cascade->stage_classifier[i].classifier );
        }
        icvReleaseHidHaarClassifierCascade( &cascade->hid_cascade );
        cvFree( _cascade );
    }
}


/****************************************************************************************\
*                                  Persistence functions                                 *
\****************************************************************************************/

/* field names */

#define ICV_HAAR_SIZE_NAME            "size"
#define ICV_HAAR_STAGES_NAME          "stages"
#define ICV_HAAR_TREES_NAME             "trees"
#define ICV_HAAR_FEATURE_NAME             "feature"
#define ICV_HAAR_RECTS_NAME                 "rects"
#define ICV_HAAR_TILTED_NAME                "tilted"
#define ICV_HAAR_THRESHOLD_NAME           "threshold"
#define ICV_HAAR_LEFT_NODE_NAME           "left_node"
#define ICV_HAAR_LEFT_VAL_NAME            "left_val"
#define ICV_HAAR_RIGHT_NODE_NAME          "right_node"
#define ICV_HAAR_RIGHT_VAL_NAME           "right_val"
#define ICV_HAAR_STAGE_THRESHOLD_NAME   "stage_threshold"
#define ICV_HAAR_PARENT_NAME            "parent"
#define ICV_HAAR_NEXT_NAME              "next"

static int
icvIsHaarClassifier( const void* struct_ptr )
{
    return CV_IS_HAAR_CLASSIFIER( struct_ptr );
}

static void*
icvReadHaarClassifier( CvFileStorage* fs, CvFileNode* node )
{
    CvHaarClassifierCascade* cascade = NULL;

    CV_FUNCNAME( "cvReadHaarClassifier" );

    __BEGIN__;

    char buf[256];
    CvFileNode* seq_fn = NULL; /* sequence */
    CvFileNode* fn = NULL;
    CvFileNode* stages_fn = NULL;
    CvSeqReader stages_reader;
    int n;
    int i, j, k, l;
    int parent, next;

    CV_CALL( stages_fn = cvGetFileNodeByName( fs, node, ICV_HAAR_STAGES_NAME ) );
    if( !stages_fn || !CV_NODE_IS_SEQ( stages_fn->tag) )
        CV_ERROR( CV_StsError, "Invalid stages node" );

    n = stages_fn->data.seq->total;
    CV_CALL( cascade = icvCreateHaarClassifierCascade(n) );

    /* read size */
    CV_CALL( seq_fn = cvGetFileNodeByName( fs, node, ICV_HAAR_SIZE_NAME ) );
    if( !seq_fn || !CV_NODE_IS_SEQ( seq_fn->tag ) || seq_fn->data.seq->total != 2 )
        CV_ERROR( CV_StsError, "size node is not a valid sequence." );
    CV_CALL( fn = (CvFileNode*) cvGetSeqElem( seq_fn->data.seq, 0 ) );
    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0 )
        CV_ERROR( CV_StsError, "Invalid size node: width must be positive integer" );
    cascade->orig_window_size.width = fn->data.i;
    CV_CALL( fn = (CvFileNode*) cvGetSeqElem( seq_fn->data.seq, 1 ) );
    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0 )
        CV_ERROR( CV_StsError, "Invalid size node: height must be positive integer" );
    cascade->orig_window_size.height = fn->data.i;

    CV_CALL( cvStartReadSeq( stages_fn->data.seq, &stages_reader ) );
    for( i = 0; i < n; ++i )
    {
        CvFileNode* stage_fn;
        CvFileNode* trees_fn;
        CvSeqReader trees_reader;

        stage_fn = (CvFileNode*) stages_reader.ptr;
        if( !CV_NODE_IS_MAP( stage_fn->tag ) )
        {
            sprintf( buf, "Invalid stage %d", i );
            CV_ERROR( CV_StsError, buf );
        }

        CV_CALL( trees_fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_TREES_NAME ) );
        if( !trees_fn || !CV_NODE_IS_SEQ( trees_fn->tag )
            || trees_fn->data.seq->total <= 0 )
        {
            sprintf( buf, "Trees node is not a valid sequence. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }

        CV_CALL( cascade->stage_classifier[i].classifier =
            (CvHaarClassifier*) cvAlloc( trees_fn->data.seq->total
                * sizeof( cascade->stage_classifier[i].classifier[0] ) ) );
        for( j = 0; j < trees_fn->data.seq->total; ++j )
        {
            cascade->stage_classifier[i].classifier[j].haar_feature = NULL;
        }
        cascade->stage_classifier[i].count = trees_fn->data.seq->total;

        CV_CALL( cvStartReadSeq( trees_fn->data.seq, &trees_reader ) );
        for( j = 0; j < trees_fn->data.seq->total; ++j )
        {
            CvFileNode* tree_fn;
            CvSeqReader tree_reader;
            CvHaarClassifier* classifier;
            int last_idx;

            classifier = &cascade->stage_classifier[i].classifier[j];
            tree_fn = (CvFileNode*) trees_reader.ptr;
            if( !CV_NODE_IS_SEQ( tree_fn->tag ) || tree_fn->data.seq->total <= 0 )
            {
                sprintf( buf, "Tree node is not a valid sequence."
                         " (stage %d, tree %d)", i, j );
                CV_ERROR( CV_StsError, buf );
            }

            classifier->count = tree_fn->data.seq->total;
            CV_CALL( classifier->haar_feature = (CvHaarFeature*) cvAlloc(
                classifier->count * ( sizeof( *classifier->haar_feature ) +
                                      sizeof( *classifier->threshold ) +
                                      sizeof( *classifier->left ) +
                                      sizeof( *classifier->right ) ) +
                (classifier->count + 1) * sizeof( *classifier->alpha ) ) );
            classifier->threshold = (float*) (classifier->haar_feature+classifier->count);
            classifier->left = (int*) (classifier->threshold + classifier->count);
            classifier->right = (int*) (classifier->left + classifier->count);
            classifier->alpha = (float*) (classifier->right + classifier->count);

            CV_CALL( cvStartReadSeq( tree_fn->data.seq, &tree_reader ) );
            for( k = 0, last_idx = 0; k < tree_fn->data.seq->total; ++k )
            {
                CvFileNode* node_fn;
                CvFileNode* feature_fn;
                CvFileNode* rects_fn;
                CvSeqReader rects_reader;

                node_fn = (CvFileNode*) tree_reader.ptr;
                if( !CV_NODE_IS_MAP( node_fn->tag ) )
                {
                    sprintf( buf, "Tree node %d is not a valid map. (stage %d, tree %d)",
                             k, i, j );
                    CV_ERROR( CV_StsError, buf );
                }
                CV_CALL( feature_fn = cvGetFileNodeByName( fs, node_fn,
                    ICV_HAAR_FEATURE_NAME ) );
                if( !feature_fn || !CV_NODE_IS_MAP( feature_fn->tag ) )
                {
                    sprintf( buf, "Feature node is not a valid map. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                CV_CALL( rects_fn = cvGetFileNodeByName( fs, feature_fn,
                    ICV_HAAR_RECTS_NAME ) );
                if( !rects_fn || !CV_NODE_IS_SEQ( rects_fn->tag )
                    || rects_fn->data.seq->total < 1
                    || rects_fn->data.seq->total > CV_HAAR_FEATURE_MAX )
                {
                    sprintf( buf, "Rects node is not a valid sequence. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                CV_CALL( cvStartReadSeq( rects_fn->data.seq, &rects_reader ) );
                for( l = 0; l < rects_fn->data.seq->total; ++l )
                {
                    CvFileNode* rect_fn;
                    CvRect r;

                    rect_fn = (CvFileNode*) rects_reader.ptr;
                    if( !CV_NODE_IS_SEQ( rect_fn->tag ) || rect_fn->data.seq->total != 5 )
                    {
                        sprintf( buf, "Rect %d is not a valid sequence. "
                                 "(stage %d, tree %d, node %d)", l, i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }

                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 0 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i < 0 )
                    {
                        sprintf( buf, "x coordinate must be non-negative integer. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.x = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 1 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i < 0 )
                    {
                        sprintf( buf, "y coordinate must be non-negative integer. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.y = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 2 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0
                        || r.x + fn->data.i > cascade->orig_window_size.width )
                    {
                        sprintf( buf, "width must be positive integer and "
                                 "(x + width) must not exceed window width. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.width = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 3 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0
                        || r.y + fn->data.i > cascade->orig_window_size.height )
                    {
                        sprintf( buf, "height must be positive integer and "
                                 "(y + height) must not exceed window height. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.height = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 4 );
                    if( !CV_NODE_IS_REAL( fn->tag ) )
                    {
                        sprintf( buf, "weight must be real number. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }

                    classifier->haar_feature[k].rect[l].weight = (float) fn->data.f;
                    classifier->haar_feature[k].rect[l].r = r;

                    CV_NEXT_SEQ_ELEM( sizeof( *rect_fn ), rects_reader );
                } /* for each rect */
                for( l = rects_fn->data.seq->total; l < CV_HAAR_FEATURE_MAX; ++l )
                {
                    classifier->haar_feature[k].rect[l].weight = 0;
                    classifier->haar_feature[k].rect[l].r = cvRect( 0, 0, 0, 0 );
                }

                CV_CALL( fn = cvGetFileNodeByName( fs, feature_fn, ICV_HAAR_TILTED_NAME));
                if( !fn || !CV_NODE_IS_INT( fn->tag ) )
                {
                    sprintf( buf, "tilted must be 0 or 1. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                classifier->haar_feature[k].tilted = ( fn->data.i != 0 );
                CV_CALL( fn = cvGetFileNodeByName( fs, node_fn, ICV_HAAR_THRESHOLD_NAME));
                if( !fn || !CV_NODE_IS_REAL( fn->tag ) )
                {
                    sprintf( buf, "threshold must be real number. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                classifier->threshold[k] = (float) fn->data.f;
                CV_CALL( fn = cvGetFileNodeByName( fs, node_fn, ICV_HAAR_LEFT_NODE_NAME));
                if( fn )
                {
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= k
                        || fn->data.i >= tree_fn->data.seq->total )
                    {
                        sprintf( buf, "left node must be valid node number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* left node */
                    classifier->left[k] = fn->data.i;
                }
                else
                {
                    CV_CALL( fn = cvGetFileNodeByName( fs, node_fn,
                        ICV_HAAR_LEFT_VAL_NAME ) );
                    if( !fn )
                    {
                        sprintf( buf, "left node or left value must be specified. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    if( !CV_NODE_IS_REAL( fn->tag ) )
                    {
                        sprintf( buf, "left value must be real number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* left value */
                    if( last_idx >= classifier->count + 1 )
                    {
                        sprintf( buf, "Tree structure is broken: too many values. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    classifier->left[k] = -last_idx;
                    classifier->alpha[last_idx++] = (float) fn->data.f;
                }
                CV_CALL( fn = cvGetFileNodeByName( fs, node_fn,ICV_HAAR_RIGHT_NODE_NAME));
                if( fn )
                {
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= k
                        || fn->data.i >= tree_fn->data.seq->total )
                    {
                        sprintf( buf, "right node must be valid node number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* right node */
                    classifier->right[k] = fn->data.i;
                }
                else
                {
                    CV_CALL( fn = cvGetFileNodeByName( fs, node_fn,
                        ICV_HAAR_RIGHT_VAL_NAME ) );
                    if( !fn )
                    {
                        sprintf( buf, "right node or right value must be specified. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    if( !CV_NODE_IS_REAL( fn->tag ) )
                    {
                        sprintf( buf, "right value must be real number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* right value */
                    if( last_idx >= classifier->count + 1 )
                    {
                        sprintf( buf, "Tree structure is broken: too many values. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    classifier->right[k] = -last_idx;
                    classifier->alpha[last_idx++] = (float) fn->data.f;
                }

                CV_NEXT_SEQ_ELEM( sizeof( *node_fn ), tree_reader );
            } /* for each node */
            if( last_idx != classifier->count + 1 )
            {
                sprintf( buf, "Tree structure is broken: too few values. "
                         "(stage %d, tree %d)", i, j );
                CV_ERROR( CV_StsError, buf );
            }

            CV_NEXT_SEQ_ELEM( sizeof( *tree_fn ), trees_reader );
        } /* for each tree */

        CV_CALL( fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_STAGE_THRESHOLD_NAME));
        if( !fn || !CV_NODE_IS_REAL( fn->tag ) )
        {
            sprintf( buf, "stage threshold must be real number. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }
        cascade->stage_classifier[i].threshold = (float) fn->data.f;

        parent = i - 1;
        next = -1;

        CV_CALL( fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_PARENT_NAME ) );
        if( !fn || !CV_NODE_IS_INT( fn->tag )
            || fn->data.i < -1 || fn->data.i >= cascade->count )
        {
            sprintf( buf, "parent must be integer number. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }
        parent = fn->data.i;
        CV_CALL( fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_NEXT_NAME ) );
        if( !fn || !CV_NODE_IS_INT( fn->tag )
            || fn->data.i < -1 || fn->data.i >= cascade->count )
        {
            sprintf( buf, "next must be integer number. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }
        next = fn->data.i;

        cascade->stage_classifier[i].parent = parent;
        cascade->stage_classifier[i].next = next;
        cascade->stage_classifier[i].child = -1;

        if( parent != -1 && cascade->stage_classifier[parent].child == -1 )
        {
            cascade->stage_classifier[parent].child = i;
        }

        CV_NEXT_SEQ_ELEM( sizeof( *stage_fn ), stages_reader );
    } /* for each stage */

    __END__;

    if( cvGetErrStatus() < 0 )
    {
        cvReleaseHaarClassifierCascade( &cascade );
        cascade = NULL;
    }

    return cascade;
}

static void
icvWriteHaarClassifier( CvFileStorage* fs, const char* name, const void* struct_ptr,
                        CvAttrList attributes )
{
    CV_FUNCNAME( "cvWriteHaarClassifier" );

    __BEGIN__;

    int i, j, k, l;
    char buf[256];
    const CvHaarClassifierCascade* cascade = (const CvHaarClassifierCascade*) struct_ptr;

    /* TODO: parameters check */

    CV_CALL( cvStartWriteStruct( fs, name, CV_NODE_MAP, CV_TYPE_NAME_HAAR, attributes ) );

    CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_SIZE_NAME, CV_NODE_SEQ | CV_NODE_FLOW ) );
    CV_CALL( cvWriteInt( fs, NULL, cascade->orig_window_size.width ) );
    CV_CALL( cvWriteInt( fs, NULL, cascade->orig_window_size.height ) );
    CV_CALL( cvEndWriteStruct( fs ) ); /* size */

    CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_STAGES_NAME, CV_NODE_SEQ ) );
    for( i = 0; i < cascade->count; ++i )
    {
        CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_MAP ) );
        sprintf( buf, "stage %d", i );
        CV_CALL( cvWriteComment( fs, buf, 1 ) );

        CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_TREES_NAME, CV_NODE_SEQ ) );

        for( j = 0; j < cascade->stage_classifier[i].count; ++j )
        {
            CvHaarClassifier* tree = &cascade->stage_classifier[i].classifier[j];

            CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_SEQ ) );
            sprintf( buf, "tree %d", j );
            CV_CALL( cvWriteComment( fs, buf, 1 ) );

            for( k = 0; k < tree->count; ++k )
            {
                CvHaarFeature* feature = &tree->haar_feature[k];

                CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_MAP ) );
                if( k )
                {
                    sprintf( buf, "node %d", k );
                }
                else
                {
                    sprintf( buf, "root node" );
                }
                CV_CALL( cvWriteComment( fs, buf, 1 ) );

                CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_FEATURE_NAME, CV_NODE_MAP ) );

                CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_RECTS_NAME, CV_NODE_SEQ ) );
                for( l = 0; l < CV_HAAR_FEATURE_MAX && feature->rect[l].r.width != 0; ++l )
                {
                    CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_SEQ | CV_NODE_FLOW ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.x ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.y ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.width ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.height ) );
                    CV_CALL( cvWriteReal( fs, NULL, feature->rect[l].weight ) );
                    CV_CALL( cvEndWriteStruct( fs ) ); /* rect */
                }
                CV_CALL( cvEndWriteStruct( fs ) ); /* rects */
                CV_CALL( cvWriteInt( fs, ICV_HAAR_TILTED_NAME, feature->tilted ) );
                CV_CALL( cvEndWriteStruct( fs ) ); /* feature */

                CV_CALL( cvWriteReal( fs, ICV_HAAR_THRESHOLD_NAME, tree->threshold[k]) );

                if( tree->left[k] > 0 )
                {
                    CV_CALL( cvWriteInt( fs, ICV_HAAR_LEFT_NODE_NAME, tree->left[k] ) );
                }
                else
                {
                    CV_CALL( cvWriteReal( fs, ICV_HAAR_LEFT_VAL_NAME,
                        tree->alpha[-tree->left[k]] ) );
                }

                if( tree->right[k] > 0 )
                {
                    CV_CALL( cvWriteInt( fs, ICV_HAAR_RIGHT_NODE_NAME, tree->right[k] ) );
                }
                else
                {
                    CV_CALL( cvWriteReal( fs, ICV_HAAR_RIGHT_VAL_NAME,
                        tree->alpha[-tree->right[k]] ) );
                }

                CV_CALL( cvEndWriteStruct( fs ) ); /* split */
            }

            CV_CALL( cvEndWriteStruct( fs ) ); /* tree */
        }

        CV_CALL( cvEndWriteStruct( fs ) ); /* trees */

        CV_CALL( cvWriteReal( fs, ICV_HAAR_STAGE_THRESHOLD_NAME,
                              cascade->stage_classifier[i].threshold) );

        CV_CALL( cvWriteInt( fs, ICV_HAAR_PARENT_NAME,
                              cascade->stage_classifier[i].parent ) );
        CV_CALL( cvWriteInt( fs, ICV_HAAR_NEXT_NAME,
                              cascade->stage_classifier[i].next ) );

        CV_CALL( cvEndWriteStruct( fs ) ); /* stage */
    } /* for each stage */

    CV_CALL( cvEndWriteStruct( fs ) ); /* stages */
    CV_CALL( cvEndWriteStruct( fs ) ); /* root */

    __END__;
}

static void*
icvCloneHaarClassifier( const void* struct_ptr )
{
    CvHaarClassifierCascade* cascade = NULL;

    CV_FUNCNAME( "cvCloneHaarClassifier" );

    __BEGIN__;

    int i, j, k, n;
    const CvHaarClassifierCascade* cascade_src =
        (const CvHaarClassifierCascade*) struct_ptr;

    n = cascade_src->count;
    CV_CALL( cascade = icvCreateHaarClassifierCascade(n) );
    cascade->orig_window_size = cascade_src->orig_window_size;

    for( i = 0; i < n; ++i )
    {
        cascade->stage_classifier[i].parent = cascade_src->stage_classifier[i].parent;
        cascade->stage_classifier[i].next = cascade_src->stage_classifier[i].next;
        cascade->stage_classifier[i].child = cascade_src->stage_classifier[i].child;
        cascade->stage_classifier[i].threshold = cascade_src->stage_classifier[i].threshold;

        cascade->stage_classifier[i].count = 0;
        CV_CALL( cascade->stage_classifier[i].classifier =
            (CvHaarClassifier*) cvAlloc( cascade_src->stage_classifier[i].count
                * sizeof( cascade->stage_classifier[i].classifier[0] ) ) );

        cascade->stage_classifier[i].count = cascade_src->stage_classifier[i].count;

        for( j = 0; j < cascade->stage_classifier[i].count; ++j )
        {
            cascade->stage_classifier[i].classifier[j].haar_feature = NULL;
        }

        for( j = 0; j < cascade->stage_classifier[i].count; ++j )
        {
            const CvHaarClassifier* classifier_src =
                &cascade_src->stage_classifier[i].classifier[j];
            CvHaarClassifier* classifier =
                &cascade->stage_classifier[i].classifier[j];

            classifier->count = classifier_src->count;
            CV_CALL( classifier->haar_feature = (CvHaarFeature*) cvAlloc(
                classifier->count * ( sizeof( *classifier->haar_feature ) +
                                      sizeof( *classifier->threshold ) +
                                      sizeof( *classifier->left ) +
                                      sizeof( *classifier->right ) ) +
                (classifier->count + 1) * sizeof( *classifier->alpha ) ) );
            classifier->threshold = (float*) (classifier->haar_feature+classifier->count);
            classifier->left = (int*) (classifier->threshold + classifier->count);
            classifier->right = (int*) (classifier->left + classifier->count);
            classifier->alpha = (float*) (classifier->right + classifier->count);
            for( k = 0; k < classifier->count; ++k )
            {
                classifier->haar_feature[k] = classifier_src->haar_feature[k];
                classifier->threshold[k] = classifier_src->threshold[k];
                classifier->left[k] = classifier_src->left[k];
                classifier->right[k] = classifier_src->right[k];
                classifier->alpha[k] = classifier_src->alpha[k];
            }
            classifier->alpha[classifier->count] =
                classifier_src->alpha[classifier->count];
        }
    }

    __END__;

    return cascade;
}


CvType haar_type( CV_TYPE_NAME_HAAR, icvIsHaarClassifier,
                  (CvReleaseFunc)cvReleaseHaarClassifierCascade,
                  icvReadHaarClassifier, icvWriteHaarClassifier,
                  icvCloneHaarClassifier );

/* End of file. */
