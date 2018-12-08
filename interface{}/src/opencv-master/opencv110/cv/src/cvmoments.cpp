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

/* The function calculates center of gravity and central second order moments */
static void
icvCompleteMomentState( CvMoments* moments )
{
    double cx = 0, cy = 0;
    double mu20, mu11, mu02;

    assert( moments != 0 );
    moments->inv_sqrt_m00 = 0;

    if( fabs(moments->m00) > DBL_EPSILON )
    {
        double inv_m00 = 1. / moments->m00;
        cx = moments->m10 * inv_m00;
        cy = moments->m01 * inv_m00;
        moments->inv_sqrt_m00 = sqrt( fabs(inv_m00) );
    }

    /* mu20 = m20 - m10*cx */
    mu20 = moments->m20 - moments->m10 * cx;
    /* mu11 = m11 - m10*cy */
    mu11 = moments->m11 - moments->m10 * cy;
    /* mu02 = m02 - m01*cy */
    mu02 = moments->m02 - moments->m01 * cy;

    moments->mu20 = mu20;
    moments->mu11 = mu11;
    moments->mu02 = mu02;

    /* mu30 = m30 - cx*(3*mu20 + cx*m10) */
    moments->mu30 = moments->m30 - cx * (3 * mu20 + cx * moments->m10);
    mu11 += mu11;
    /* mu21 = m21 - cx*(2*mu11 + cx*m01) - cy*mu20 */
    moments->mu21 = moments->m21 - cx * (mu11 + cx * moments->m01) - cy * mu20;
    /* mu12 = m12 - cy*(2*mu11 + cy*m10) - cx*mu02 */
    moments->mu12 = moments->m12 - cy * (mu11 + cy * moments->m10) - cx * mu02;
    /* mu03 = m03 - cy*(3*mu02 + cy*m01) */
    moments->mu03 = moments->m03 - cy * (3 * mu02 + cy * moments->m01);
}


static void
icvContourMoments( CvSeq* contour, CvMoments* moments )
{
    int is_float = CV_SEQ_ELTYPE(contour) == CV_32FC2;

    if( contour->total )
    {
        CvSeqReader reader;
        double a00, a10, a01, a20, a11, a02, a30, a21, a12, a03;
        double xi, yi, xi2, yi2, xi_1, yi_1, xi_12, yi_12, dxy, xii_1, yii_1;
        int lpt = contour->total;

        a00 = a10 = a01 = a20 = a11 = a02 = a30 = a21 = a12 = a03 = 0;

        cvStartReadSeq( contour, &reader, 0 );

        if( !is_float )
        {
            xi_1 = ((CvPoint*)(reader.ptr))->x;
            yi_1 = ((CvPoint*)(reader.ptr))->y;
        }
        else
        {
            xi_1 = ((CvPoint2D32f*)(reader.ptr))->x;
            yi_1 = ((CvPoint2D32f*)(reader.ptr))->y;
        }
        CV_NEXT_SEQ_ELEM( contour->elem_size, reader );
        
        xi_12 = xi_1 * xi_1;
        yi_12 = yi_1 * yi_1;

        while( lpt-- > 0 )
        {
            if( !is_float )
            {
                xi = ((CvPoint*)(reader.ptr))->x;
                yi = ((CvPoint*)(reader.ptr))->y;
            }
            else
            {
                xi = ((CvPoint2D32f*)(reader.ptr))->x;
                yi = ((CvPoint2D32f*)(reader.ptr))->y;
            }
            CV_NEXT_SEQ_ELEM( contour->elem_size, reader );

            xi2 = xi * xi;
            yi2 = yi * yi;
            dxy = xi_1 * yi - xi * yi_1;
            xii_1 = xi_1 + xi;
            yii_1 = yi_1 + yi;

            a00 += dxy;
            a10 += dxy * xii_1;
            a01 += dxy * yii_1;
            a20 += dxy * (xi_1 * xii_1 + xi2);
            a11 += dxy * (xi_1 * (yii_1 + yi_1) + xi * (yii_1 + yi));
            a02 += dxy * (yi_1 * yii_1 + yi2);
            a30 += dxy * xii_1 * (xi_12 + xi2);
            a03 += dxy * yii_1 * (yi_12 + yi2);
            a21 +=
                dxy * (xi_12 * (3 * yi_1 + yi) + 2 * xi * xi_1 * yii_1 +
                       xi2 * (yi_1 + 3 * yi));
            a12 +=
                dxy * (yi_12 * (3 * xi_1 + xi) + 2 * yi * yi_1 * xii_1 +
                       yi2 * (xi_1 + 3 * xi));

            xi_1 = xi;
            yi_1 = yi;
            xi_12 = xi2;
            yi_12 = yi2;
        }

        double db1_2, db1_6, db1_12, db1_24, db1_20, db1_60;

        if( fabs(a00) > FLT_EPSILON )
        {
            if( a00 > 0 )
            {
                db1_2 = 0.5;
                db1_6 = 0.16666666666666666666666666666667;
                db1_12 = 0.083333333333333333333333333333333;
                db1_24 = 0.041666666666666666666666666666667;
                db1_20 = 0.05;
                db1_60 = 0.016666666666666666666666666666667;
            }
            else
            {
                db1_2 = -0.5;
                db1_6 = -0.16666666666666666666666666666667;
                db1_12 = -0.083333333333333333333333333333333;
                db1_24 = -0.041666666666666666666666666666667;
                db1_20 = -0.05;
                db1_60 = -0.016666666666666666666666666666667;
            }

            /*  spatial moments    */
            moments->m00 = a00 * db1_2;
            moments->m10 = a10 * db1_6;
            moments->m01 = a01 * db1_6;
            moments->m20 = a20 * db1_12;
            moments->m11 = a11 * db1_24;
            moments->m02 = a02 * db1_12;
            moments->m30 = a30 * db1_20;
            moments->m21 = a21 * db1_60;
            moments->m12 = a12 * db1_60;
            moments->m03 = a03 * db1_20;

            icvCompleteMomentState( moments );
        }
    }
}


/* summarizes moment values for all tiles */
static void
icvAccumulateMoments( double *tiles, CvSize size, CvSize tile_size, CvMoments * moments )
{
    int x, y;

    for( y = 0; y < size.height; y += tile_size.height )
    {
        for( x = 0; x < size.width; x += tile_size.width, tiles += 10 )
        {
            double dx = x, dy = y;
            double dxm = dx * tiles[0], dym = dy * tiles[0];

            /* + m00 ( = m00' ) */
            moments->m00 += tiles[0];

            /* + m10 ( = m10' + dx*m00' ) */
            moments->m10 += tiles[1] + dxm;

            /* + m01 ( = m01' + dy*m00' ) */
            moments->m01 += tiles[2] + dym;

            /* + m20 ( = m20' + 2*dx*m10' + dx*dx*m00' ) */
            moments->m20 += tiles[3] + dx * (tiles[1] * 2 + dxm);

            /* + m11 ( = m11' + dx*m01' + dy*m10' + dx*dy*m00' ) */
            moments->m11 += tiles[4] + dx * (tiles[2] + dym) + dy * tiles[1];

            /* + m02 ( = m02' + 2*dy*m01' + dy*dy*m00' ) */
            moments->m02 += tiles[5] + dy * (tiles[2] * 2 + dym);

            /* + m30 ( = m30' + 3*dx*m20' + 3*dx*dx*m10' + dx*dx*dx*m00' ) */
            moments->m30 += tiles[6] + dx * (3. * tiles[3] + dx * (3. * tiles[1] + dxm));

            /* + m21 (= m21' + dx*(2*m11' + 2*dy*m10' + dx*m01' + dx*dy*m00') + dy*m20') */
            moments->m21 += tiles[7] + dx * (2 * (tiles[4] + dy * tiles[1]) +
                                             dx * (tiles[2] + dym)) + dy * tiles[3];

            /* + m12 (= m12' + dy*(2*m11' + 2*dx*m01' + dy*m10' + dx*dy*m00') + dx*m02') */
            moments->m12 += tiles[8] + dy * (2 * (tiles[4] + dx * tiles[2]) +
                                             dy * (tiles[1] + dxm)) + dx * tiles[5];

            /* + m03 ( = m03' + 3*dy*m02' + 3*dy*dy*m01' + dy*dy*dy*m00' ) */
            moments->m03 += tiles[9] + dy * (3. * tiles[5] + dy * (3. * tiles[2] + dym));
        }
    }

    icvCompleteMomentState( moments );
}


/****************************************************************************************\
*                                   Spatial Moments                                      *
\****************************************************************************************/

#define ICV_DEF_CALC_MOMENTS_IN_TILE( __op__, name, flavor, srctype, temptype, momtype ) \
static CvStatus CV_STDCALL icv##name##_##flavor##_CnCR                                   \
( const srctype* img, int step, CvSize size, int cn, int coi, double *moments )          \
{                                                                                        \
    int x, y, sx_init = (size.width & -4) * (size.width & -4), sy = 0;                   \
    momtype mom[10];                                                                     \
                                                                                         \
    assert( img && size.width && (size.width | size.height) >= 0 );                      \
    memset( mom, 0, 10 * sizeof( mom[0] ));                                              \
                                                                                         \
    if( coi )                                                                            \
        img += coi - 1;                                                                  \
    step /= sizeof(img[0]);                                                              \
                                                                                         \
    for( y = 0; y < size.height; sy += 2 * y + 1, y++, img += step )                     \
    {                                                                                    \
        temptype  x0 = 0;                                                                \
        temptype  x1 = 0;                                                                \
        temptype  x2 = 0;                                                                \
        momtype   x3 = 0;                                                                \
        int sx = sx_init;                                                                \
        const srctype* ptr = img;                                                        \
                                                                                         \
        for( x = 0; x < size.width - 3; x += 4, ptr += cn*4 )                            \
        {                                                                                \
            temptype p0 = __op__(ptr[0]), p1 = __op__(ptr[cn]),                          \
                     p2 = __op__(ptr[2*cn]), p3 = __op__(ptr[3*cn]);                     \
            temptype t = p1;                                                             \
            temptype a, b, c;                                                            \
                                                                                         \
            p0 += p1 + p2 + p3; /* p0 + p1 + p2 + p3 */                                  \
            p1 += 2 * p2 + 3 * p3;      /* p1 + p2*2 + p3*3 */                           \
            p2 = p1 + 2 * p2 + 6 * p3;  /* p1 + p2*4 + p3*9 */                           \
            p3 = 2 * p2 - t + 9 * p3;   /* p1 + p2*8 + p3*27 */                          \
                                                                                         \
            a = x * p0 + p1;    /* x*p0 + (x+1)*p1 + (x+2)*p2 + (x+3)*p3 */              \
            b = x * p1 + p2;    /* (x+1)*p1 + 2*(x+2)*p2 + 3*(x+3)*p3 */                 \
            c = x * p2 + p3;    /* (x+1)*p1 + 4*(x+2)*p2 + 9*(x+3)*p3 */                 \
                                                                                         \
            x0 += p0;                                                                    \
            x1 += a;                                                                     \
            a = a * x + b;      /*(x^2)*p0+((x+1)^2)*p1+((x+2)^2)*p2+((x+3)^2)*p3 */     \
            x2 += a;                                                                     \
            x3 += ((momtype)(a + b)) * x + c;  /*x3 += (x^3)*p0+((x+1)^3)*p1 +  */       \
                                               /*  ((x+2)^3)*p2+((x+3)^3)*p3   */        \
        }                                                                                \
                                                                                         \
        /* process the rest */                                                           \
        for( ; x < size.width; sx += 2 * x + 1, x++, ptr += cn )                         \
        {                                                                                \
            temptype p = __op__(ptr[0]);                                                 \
            temptype xp = x * p;                                                         \
                                                                                         \
            x0 += p;                                                                     \
            x1 += xp;                                                                    \
            x2 += sx * p;                                                                \
            x3 += ((momtype)sx) * xp;                                                    \
        }                                                                                \
                                                                                         \
        {                                                                                \
            temptype py = y * x0;                                                        \
                                                                                         \
            mom[9] += ((momtype)py) * sy;  /* m03 */                                     \
            mom[8] += ((momtype)x1) * sy;  /* m12 */                                     \
            mom[7] += ((momtype)x2) * y;   /* m21 */                                     \
            mom[6] += x3;                  /* m30 */                                     \
            mom[5] += x0 * sy;             /* m02 */                                     \
            mom[4] += x1 * y;              /* m11 */                                     \
            mom[3] += x2;                  /* m20 */                                     \
            mom[2] += py;                  /* m01 */                                     \
            mom[1] += x1;                  /* m10 */                                     \
            mom[0] += x0;                  /* m00 */                                     \
        }                                                                                \
    }                                                                                    \
                                                                                         \
    for( x = 0; x < 10; x++ )                                                            \
        moments[x] = (double)mom[x];                                                     \
                                                                                         \
    return CV_OK;                                                                        \
}


ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NOP, MomentsInTile, 8u, uchar, int, int )
ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NOP, MomentsInTile, 16u, ushort, int, int64 )
ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NOP, MomentsInTile, 16s, short, int, int64 )
ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NOP, MomentsInTile, 32f, float, double, double )
ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NOP, MomentsInTile, 64f, double, double, double )

ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NONZERO, MomentsInTileBin, 8u, uchar, int, int )
ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NONZERO, MomentsInTileBin, 16s, ushort, int, int )
ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NONZERO_FLT, MomentsInTileBin, 32f, int, int, int )
ICV_DEF_CALC_MOMENTS_IN_TILE( CV_NONZERO_FLT, MomentsInTileBin, 64f, int64, double, double )

#define icvMomentsInTile_8s_CnCR  0
#define icvMomentsInTile_32s_CnCR  0
#define icvMomentsInTileBin_8s_CnCR   icvMomentsInTileBin_8u_CnCR
#define icvMomentsInTileBin_16u_CnCR   icvMomentsInTileBin_16s_CnCR
#define icvMomentsInTileBin_32s_CnCR  0

CV_DEF_INIT_FUNC_TAB_2D( MomentsInTile, CnCR )
CV_DEF_INIT_FUNC_TAB_2D( MomentsInTileBin, CnCR )

////////////////////////////////// IPP moment functions //////////////////////////////////

icvMoments_8u_C1R_t icvMoments_8u_C1R_p = 0;
icvMoments_32f_C1R_t icvMoments_32f_C1R_p = 0;
icvMomentInitAlloc_64f_t icvMomentInitAlloc_64f_p = 0;
icvMomentFree_64f_t icvMomentFree_64f_p = 0;
icvGetSpatialMoment_64f_t icvGetSpatialMoment_64f_p = 0;

typedef CvStatus (CV_STDCALL * CvMomentIPPFunc)
    ( const void* img, int step, CvSize size, void* momentstate );

CV_IMPL void
cvMoments( const void* array, CvMoments* moments, int binary )
{
    static CvFuncTable mom_tab;
    static CvFuncTable mombin_tab;
    static int inittab = 0;
    double* tiles = 0;
    void* ippmomentstate = 0;

    CV_FUNCNAME("cvMoments");

    __BEGIN__;

    int type = 0, depth, cn, pix_size;
    int coi = 0;
    int x, y, k, tile_num = 1;
    CvSize size, tile_size = { 32, 32 };
    CvMat stub, *mat = (CvMat*)array;
    CvFunc2DnC_1A1P func = 0;
    CvMomentIPPFunc ipp_func = 0;
    CvContour contour_header;
    CvSeq* contour = 0;
    CvSeqBlock block;

    if( CV_IS_SEQ( array ))
    {
        contour = (CvSeq*)array;
        if( !CV_IS_SEQ_POLYGON( contour ))
            CV_ERROR( CV_StsBadArg, "The passed sequence is not a valid contour" );
    }

    if( !inittab )
    {
        icvInitMomentsInTileCnCRTable( &mom_tab );
        icvInitMomentsInTileBinCnCRTable( &mombin_tab );
        inittab = 1;
    }
    
    if( !moments )
        CV_ERROR( CV_StsNullPtr, "" );

    memset( moments, 0, sizeof(*moments));

    if( !contour )
    {
        CV_CALL( mat = cvGetMat( mat, &stub, &coi ));
        type = CV_MAT_TYPE( mat->type );

        if( type == CV_32SC2 || type == CV_32FC2 )
        {
            CV_CALL( contour = cvPointSeqFromMat(
                CV_SEQ_KIND_CURVE | CV_SEQ_FLAG_CLOSED,
                mat, &contour_header, &block ));
        }
    }

    if( contour )
    {
        icvContourMoments( contour, moments );
        EXIT;
    }

    type = CV_MAT_TYPE( mat->type );
    depth = CV_MAT_DEPTH( type );
    cn = CV_MAT_CN( type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( mat );

    if( cn > 1 && coi == 0 )
        CV_ERROR( CV_StsBadArg, "Invalid image type" );

    if( size.width <= 0 || size.height <= 0 )
    {
        EXIT;
    }

    if( type == CV_8UC1 )
        ipp_func = (CvMomentIPPFunc)icvMoments_8u_C1R_p;
    else if( type == CV_32FC1 )
        ipp_func = (CvMomentIPPFunc)icvMoments_32f_C1R_p;

    if( ipp_func && !binary )
    {
        int matstep = mat->step ? mat->step : CV_STUB_STEP;
        IPPI_CALL( icvMomentInitAlloc_64f_p( &ippmomentstate, cvAlgHintAccurate ));
        IPPI_CALL( ipp_func( mat->data.ptr, matstep, size, ippmomentstate ));
        icvGetSpatialMoment_64f_p( ippmomentstate, 0, 0, 0, cvPoint(0,0), &moments->m00 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 1, 0, 0, cvPoint(0,0), &moments->m10 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 0, 1, 0, cvPoint(0,0), &moments->m01 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 2, 0, 0, cvPoint(0,0), &moments->m20 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 1, 1, 0, cvPoint(0,0), &moments->m11 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 0, 2, 0, cvPoint(0,0), &moments->m02 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 3, 0, 0, cvPoint(0,0), &moments->m30 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 2, 1, 0, cvPoint(0,0), &moments->m21 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 1, 2, 0, cvPoint(0,0), &moments->m12 );
        icvGetSpatialMoment_64f_p( ippmomentstate, 0, 3, 0, cvPoint(0,0), &moments->m03 );
        icvCompleteMomentState( moments );
        EXIT;
    }

    func = (CvFunc2DnC_1A1P)(!binary ? mom_tab.fn_2d[depth] : mombin_tab.fn_2d[depth]);

    if( !func )
        CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

    if( depth >= CV_32S && !binary )
        tile_size = size;
    else
        tile_num = ((size.width + tile_size.width - 1)/tile_size.width)*
                   ((size.height + tile_size.height - 1)/tile_size.height);

    CV_CALL( tiles = (double*)cvAlloc( tile_num*10*sizeof(double)));

    for( y = 0, k = 0; y < size.height; y += tile_size.height )
    {
        CvSize cur_tile_size = tile_size;
        if( y + cur_tile_size.height > size.height )
            cur_tile_size.height = size.height - y;
        
        for( x = 0; x < size.width; x += tile_size.width, k++ )
        {
            if( x + cur_tile_size.width > size.width )
                cur_tile_size.width = size.width - x;

            assert( k < tile_num );

            IPPI_CALL( func( mat->data.ptr + y*mat->step + x*pix_size,
                             mat->step, cur_tile_size, cn, coi, tiles + k*10 ));
        }
    }

    icvAccumulateMoments( tiles, size, tile_size, moments );

    __END__;

    if( ippmomentstate )
        icvMomentFree_64f_p( ippmomentstate );

    cvFree( &tiles );
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: cvGetHuMoments
//    Purpose: Returns Hu moments
//    Context:
//    Parameters:
//      mState  - moment structure filled by one of the icvMoments[Binary]*** function
//      HuState - pointer to output structure containing seven Hu moments
//    Returns:
//      CV_NO_ERR if success or error code
//    Notes:
//F*/
CV_IMPL void
cvGetHuMoments( CvMoments * mState, CvHuMoments * HuState )
{
    CV_FUNCNAME( "cvGetHuMoments" );

    __BEGIN__;

    if( !mState || !HuState )
        CV_ERROR_FROM_STATUS( CV_NULLPTR_ERR );

    {
        double m00s = mState->inv_sqrt_m00, m00 = m00s * m00s, s2 = m00 * m00, s3 = s2 * m00s;

        double nu20 = mState->mu20 * s2,
            nu11 = mState->mu11 * s2,
            nu02 = mState->mu02 * s2,
            nu30 = mState->mu30 * s3,
            nu21 = mState->mu21 * s3, nu12 = mState->mu12 * s3, nu03 = mState->mu03 * s3;

        double t0 = nu30 + nu12;
        double t1 = nu21 + nu03;

        double q0 = t0 * t0, q1 = t1 * t1;

        double n4 = 4 * nu11;
        double s = nu20 + nu02;
        double d = nu20 - nu02;

        HuState->hu1 = s;
        HuState->hu2 = d * d + n4 * nu11;
        HuState->hu4 = q0 + q1;
        HuState->hu6 = d * (q0 - q1) + n4 * t0 * t1;

        t0 *= q0 - 3 * q1;
        t1 *= 3 * q0 - q1;

        q0 = nu30 - 3 * nu12;
        q1 = 3 * nu21 - nu03;

        HuState->hu3 = q0 * q0 + q1 * q1;
        HuState->hu5 = q0 * t0 + q1 * t1;
        HuState->hu7 = q1 * t0 - q0 * t1;
    }

    __END__;
}


/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: cvGetSpatialMoment
//    Purpose:  Returns spatial moment(x_order, y_order) which is determined as:
//              m(x_o,y_o) = sum (x ^ x_o)*(y ^ y_o)*I(x,y)
//              0 <= x_o, y_o; x_o + y_o <= 3
//    Context:
//    Parameters:
//      mom  - moment structure filled by one of the icvMoments[Binary]*** function
//      x_order - x order of the moment
//      y_order - y order of the moment
//    Returns:
//      moment value or large negative number (-DBL_MAX) if error
//    Notes:
//F*/
CV_IMPL double
cvGetSpatialMoment( CvMoments * moments, int x_order, int y_order )
{
    int order = x_order + y_order;
    double moment = -DBL_MAX;

    CV_FUNCNAME( "cvGetSpatialMoment" );

    __BEGIN__;

    if( !moments )
        CV_ERROR_FROM_STATUS( CV_NULLPTR_ERR );
    if( (x_order | y_order) < 0 || order > 3 )
        CV_ERROR_FROM_STATUS( CV_BADRANGE_ERR );

    moment = (&(moments->m00))[order + (order >> 1) + (order > 2) * 2 + y_order];

    __END__;

    return moment;
}


/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: cvGetCentralMoment
//    Purpose:  Returns central moment(x_order, y_order) which is determined as:
//              mu(x_o,y_o) = sum ((x - xc)^ x_o)*((y - yc) ^ y_o)*I(x,y)
//              0 <= x_o, y_o; x_o + y_o <= 3,
//              (xc, yc) = (m10/m00,m01/m00) - center of gravity 
//    Context:
//    Parameters:
//      mom  - moment structure filled by one of the icvMoments[Binary]*** function
//      x_order - x order of the moment
//      y_order - y order of the moment
//    Returns:
//      moment value or large negative number (-DBL_MAX) if error
//    Notes:
//F*/
CV_IMPL double
cvGetCentralMoment( CvMoments * moments, int x_order, int y_order )
{
    int order = x_order + y_order;
    double mu = 0;

    CV_FUNCNAME( "cvGetCentralMoment" );

    __BEGIN__;

    if( !moments )
        CV_ERROR_FROM_STATUS( CV_NULLPTR_ERR );
    if( (x_order | y_order) < 0 || order > 3 )
        CV_ERROR_FROM_STATUS( CV_BADRANGE_ERR );

    if( order >= 2 )
    {
        mu = (&(moments->m00))[4 + order * 3 + y_order];
    }
    else if( order == 0 )
        mu = moments->m00;

    __END__;

    return mu;
}


/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: cvGetNormalizedCentralMoment
//    Purpose: Returns normalized central moment(x_order,y_order) which is determined as:
//             nu(x_o,y_o) = mu(x_o, y_o)/(m00 ^ (((x_o + y_o)/2) + 1))
//             0 <= x_o, y_o; x_o + y_o <= 3,
//             (xc, yc) = (m10/m00,m01/m00) - center of gravity 
//    Context:
//    Parameters:
//      mom  - moment structure filled by one of the icvMoments[Binary]*** function
//      x_order - x order of the moment
//      y_order - y order of the moment
//    Returns:
//      moment value or large negative number (-DBL_MAX) if error
//    Notes:
//F*/
CV_IMPL double
cvGetNormalizedCentralMoment( CvMoments * moments, int x_order, int y_order )
{
    int order = x_order + y_order;
    double mu = 0;
    double m00s, m00;

    CV_FUNCNAME( "cvGetCentralNormalizedMoment" );

    __BEGIN__;

    mu = cvGetCentralMoment( moments, x_order, y_order );
    CV_CHECK();

    m00s = moments->inv_sqrt_m00;
    m00 = m00s * m00s;

    while( --order >= 0 )
        m00 *= m00s;
    mu *= m00;

    __END__;

    return mu;
}


/* End of file. */
