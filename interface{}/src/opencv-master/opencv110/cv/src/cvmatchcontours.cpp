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
//    Name: cvMatchContours
//    Purpose:
//      Calculates matching of the two contours
//    Context:
//    Parameters:
//      contour_1 - pointer to the first input contour object.
//      contour_2 - pointer to the second input contour object.
//      method - method for the matching calculation
//      (now CV_IPPI_CONTOURS_MATCH_I1, CV_CONTOURS_MATCH_I2 or
//      CV_CONTOURS_MATCH_I3 only  )
//      rezult - output calculated measure 
//
//F*/
CV_IMPL  double
cvMatchShapes( const void* contour1, const void* contour2,
               int method, double /*parameter*/ )
{
    CvMoments moments;
    CvHuMoments huMoments;
    double ma[7], mb[7];
    int i, sma, smb;
    double eps = 1.e-5;
    double mmm;
    double result = 0;

    CV_FUNCNAME( "cvMatchShapes" );

    __BEGIN__;

    if( !contour1 || !contour2 )
        CV_ERROR( CV_StsNullPtr, "" );

/*   first moments calculation */
    CV_CALL( cvMoments( contour1, &moments ));

/*  Hu moments calculation   */
    CV_CALL( cvGetHuMoments( &moments, &huMoments ));

    ma[0] = huMoments.hu1;
    ma[1] = huMoments.hu2;
    ma[2] = huMoments.hu3;
    ma[3] = huMoments.hu4;
    ma[4] = huMoments.hu5;
    ma[5] = huMoments.hu6;
    ma[6] = huMoments.hu7;


/*   second moments calculation  */
    CV_CALL( cvMoments( contour2, &moments ));

/*  Hu moments calculation   */
    CV_CALL( cvGetHuMoments( &moments, &huMoments ));

    mb[0] = huMoments.hu1;
    mb[1] = huMoments.hu2;
    mb[2] = huMoments.hu3;
    mb[3] = huMoments.hu4;
    mb[4] = huMoments.hu5;
    mb[5] = huMoments.hu6;
    mb[6] = huMoments.hu7;

    switch (method)
    {
    case 1:
        {
            for( i = 0; i < 7; i++ )
            {
                double ama = fabs( ma[i] );
                double amb = fabs( mb[i] );

                if( ma[i] > 0 )
                    sma = 1;
                else if( ma[i] < 0 )
                    sma = -1;
                else
                    sma = 0;
                if( mb[i] > 0 )
                    smb = 1;
                else if( mb[i] < 0 )
                    smb = -1;
                else
                    smb = 0;

                if( ama > eps && amb > eps )
                {
                    ama = 1. / (sma * log10( ama ));
                    amb = 1. / (smb * log10( amb ));
                    result += fabs( -ama + amb );
                }
            }
            break;
        }

    case 2:
        {
            for( i = 0; i < 7; i++ )
            {
                double ama = fabs( ma[i] );
                double amb = fabs( mb[i] );

                if( ma[i] > 0 )
                    sma = 1;
                else if( ma[i] < 0 )
                    sma = -1;
                else
                    sma = 0;
                if( mb[i] > 0 )
                    smb = 1;
                else if( mb[i] < 0 )
                    smb = -1;
                else
                    smb = 0;

                if( ama > eps && amb > eps )
                {
                    ama = sma * log10( ama );
                    amb = smb * log10( amb );
                    result += fabs( -ama + amb );
                }
            }
            break;
        }

    case 3:
        {
            for( i = 0; i < 7; i++ )
            {
                double ama = fabs( ma[i] );
                double amb = fabs( mb[i] );

                if( ma[i] > 0 )
                    sma = 1;
                else if( ma[i] < 0 )
                    sma = -1;
                else
                    sma = 0;
                if( mb[i] > 0 )
                    smb = 1;
                else if( mb[i] < 0 )
                    smb = -1;
                else
                    smb = 0;

                if( ama > eps && amb > eps )
                {
                    ama = sma * log10( ama );
                    amb = smb * log10( amb );
                    mmm = fabs( (ama - amb) / ama );
                    if( result < mmm )
                        result = mmm;
                }
            }
            break;
        }
    default:
        CV_ERROR_FROM_STATUS( CV_BADCOEF_ERR );
    }

    __END__;

    return result;
}



/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: icvMatchContourTrees
//    Purpose:
//      Calculates matching of the two contour trees
//    Context:
//    Parameters:
//      tree1 - pointer to the first input contour tree object.
//      tree2 - pointer to the second input contour tree object.
//      method - method for the matching calculation
//      (now CV_CONTOUR_TREES_MATCH_I1 only  )
//      threshold - threshold for the contour trees matching 
//      result - output calculated measure 
//F*/
CV_IMPL  double
cvMatchContourTrees( const CvContourTree* tree1, const CvContourTree* tree2,
                     int method, double threshold )
{
    _CvTrianAttr **ptr_p1 = 0, **ptr_p2 = 0;    /*pointers to the pointer's buffer */
    _CvTrianAttr **ptr_n1 = 0, **ptr_n2 = 0;    /*pointers to the pointer's buffer */
    _CvTrianAttr **ptr11, **ptr12, **ptr21, **ptr22;

    int lpt1, lpt2, lpt, flag, flag_n, i, j, ibuf, ibuf1;
    double match_v, d12, area1, area2, r11, r12, r21, r22, w1, w2;
    double eps = 1.e-5;
    char s1, s2;
    _CvTrianAttr tree_1, tree_2;        /*current vertex 1 and 2 tree */
    CvSeqReader reader1, reader2;
    double result = 0;

    CV_FUNCNAME("cvMatchContourTrees");
    __BEGIN__;

    if( !tree1 || !tree2 )
        CV_ERROR( CV_StsNullPtr, "" );

    if( method != CV_CONTOUR_TREES_MATCH_I1 )
        CV_ERROR( CV_StsBadArg, "Unknown/unsupported comparison method" );

    if( !CV_IS_SEQ_POLYGON_TREE( tree1 ))
        CV_ERROR( CV_StsBadArg, "The first argument is not a valid contour tree" );

    if( !CV_IS_SEQ_POLYGON_TREE( tree2 ))
        CV_ERROR( CV_StsBadArg, "The second argument is not a valid contour tree" );

    lpt1 = tree1->total;
    lpt2 = tree2->total;
    lpt = lpt1 > lpt2 ? lpt1 : lpt2;

    ptr_p1 = ptr_n1 = ptr_p2 = ptr_n2 = NULL;
    CV_CALL( ptr_p1 = (_CvTrianAttr **) cvAlloc( lpt * sizeof( _CvTrianAttr * )));
    CV_CALL( ptr_p2 = (_CvTrianAttr **) cvAlloc( lpt * sizeof( _CvTrianAttr * )));

    CV_CALL( ptr_n1 = (_CvTrianAttr **) cvAlloc( lpt * sizeof( _CvTrianAttr * )));
    CV_CALL( ptr_n2 = (_CvTrianAttr **) cvAlloc( lpt * sizeof( _CvTrianAttr * )));

    cvStartReadSeq( (CvSeq *) tree1, &reader1, 0 );
    cvStartReadSeq( (CvSeq *) tree2, &reader2, 0 );

/*read the root of the first and second tree*/
    CV_READ_SEQ_ELEM( tree_1, reader1 );
    CV_READ_SEQ_ELEM( tree_2, reader2 );

/*write to buffer pointers to root's childs vertexs*/
    ptr_p1[0] = tree_1.next_v1;
    ptr_p1[1] = tree_1.next_v2;
    ptr_p2[0] = tree_2.next_v1;
    ptr_p2[1] = tree_2.next_v2;
    i = 2;
    match_v = 0.;
    area1 = tree_1.area;
    area2 = tree_2.area;

    if( area1 < eps || area2 < eps || lpt < 4 )
        CV_ERROR( CV_StsBadSize, "" );

    r11 = r12 = r21 = r22 = w1 = w2 = d12 = 0;
    flag = 0;
    s1 = s2 = 0;
    do
    {
        if( flag == 0 )
        {
            ptr11 = ptr_p1;
            ptr12 = ptr_n1;
            ptr21 = ptr_p2;
            ptr22 = ptr_n2;
            flag = 1;
        }
        else
        {
            ptr11 = ptr_n1;
            ptr12 = ptr_p1;
            ptr21 = ptr_n2;
            ptr22 = ptr_p2;
            flag = 0;
        }
        ibuf = 0;
        for( j = 0; j < i; j++ )
        {
            flag_n = 0;
            if( ptr11[j] != NULL )
            {
                r11 = ptr11[j]->r1;
                r12 = ptr11[j]->r2;
                flag_n = 1;
                w1 = ptr11[j]->area / area1;
                s1 = ptr11[j]->sign;
            }
            else
            {
                r11 = r21 = 0;
            }
            if( ptr21[j] != NULL )
            {
                r21 = ptr21[j]->r1;
                r22 = ptr21[j]->r2;
                flag_n = 1;
                w2 = ptr21[j]->area / area2;
                s2 = ptr21[j]->sign;
            }
            else
            {
                r21 = r22 = 0;
            }
            if( flag_n != 0 )
/* calculate node distance */
            {
                switch (method)
                {
                case 1:
                    {
                        double t0, t1;
                        if( s1 != s2 )
                        {
                            t0 = fabs( r11 * w1 + r21 * w2 );
                            t1 = fabs( r12 * w1 + r22 * w2 );
                        }
                        else
                        {
                            t0 = fabs( r11 * w1 - r21 * w2 );
                            t1 = fabs( r12 * w1 - r22 * w2 );
                        }
                        d12 = t0 + t1;
                        break;
                    }
                }
                match_v += d12;
                ibuf1 = ibuf + 1;
/*write to buffer the pointer to child vertexes*/
                if( ptr11[j] != NULL )
                {
                    ptr12[ibuf] = ptr11[j]->next_v1;
                    ptr12[ibuf1] = ptr11[j]->next_v2;
                }
                else
                {
                    ptr12[ibuf] = NULL;
                    ptr12[ibuf1] = NULL;
                }
                if( ptr21[j] != NULL )
                {
                    ptr22[ibuf] = ptr21[j]->next_v1;
                    ptr22[ibuf1] = ptr21[j]->next_v2;
                }
                else
                {
                    ptr22[ibuf] = NULL;
                    ptr22[ibuf1] = NULL;
                }
                ibuf += 2;
            }
        }
        i = ibuf;
    }
    while( i > 0 && match_v < threshold );

    result = match_v;

    __END__;

    cvFree( &ptr_n2 );
    cvFree( &ptr_n1 );
    cvFree( &ptr_p2 );
    cvFree( &ptr_p1 );

    return result;
}


/* End of file. */
