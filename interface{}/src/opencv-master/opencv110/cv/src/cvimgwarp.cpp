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

/* ////////////////////////////////////////////////////////////////////
//
//  Geometrical transforms on images and matrices: rotation, zoom etc.
//
// */

#include "_cv.h"


/************** interpolation constants and tables ***************/

#define ICV_WARP_MUL_ONE_8U(x)  ((x) << ICV_WARP_SHIFT)
#define ICV_WARP_DESCALE_8U(x)  CV_DESCALE((x), ICV_WARP_SHIFT*2)
#define ICV_WARP_CLIP_X(x)      ((unsigned)(x) < (unsigned)ssize.width ? \
                                (x) : (x) < 0 ? 0 : ssize.width - 1)
#define ICV_WARP_CLIP_Y(y)      ((unsigned)(y) < (unsigned)ssize.height ? \
                                (y) : (y) < 0 ? 0 : ssize.height - 1)

float icvLinearCoeffs[(ICV_LINEAR_TAB_SIZE+1)*2];

void icvInitLinearCoeffTab()
{
    static int inittab = 0;
    if( !inittab )
    {
        for( int i = 0; i <= ICV_LINEAR_TAB_SIZE; i++ )
        {
            float x = (float)i/ICV_LINEAR_TAB_SIZE;
            icvLinearCoeffs[i*2] = x;
            icvLinearCoeffs[i*2+1] = 1.f - x;
        }

        inittab = 1;
    }
}


float icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE+1)*2];

void icvInitCubicCoeffTab()
{
    static int inittab = 0;
    if( !inittab )
    {
#if 0
        // classical Mitchell-Netravali filter
        const double B = 1./3;
        const double C = 1./3;
        const double p0 = (6 - 2*B)/6.;
        const double p2 = (-18 + 12*B + 6*C)/6.;
        const double p3 = (12 - 9*B - 6*C)/6.;
        const double q0 = (8*B + 24*C)/6.;
        const double q1 = (-12*B - 48*C)/6.;
        const double q2 = (6*B + 30*C)/6.;
        const double q3 = (-B - 6*C)/6.;

        #define ICV_CUBIC_1(x)  (((x)*p3 + p2)*(x)*(x) + p0)
        #define ICV_CUBIC_2(x)  ((((x)*q3 + q2)*(x) + q1)*(x) + q0)
#else
        // alternative "sharp" filter
        const double A = -0.75;
        #define ICV_CUBIC_1(x)  (((A + 2)*(x) - (A + 3))*(x)*(x) + 1)
        #define ICV_CUBIC_2(x)  (((A*(x) - 5*A)*(x) + 8*A)*(x) - 4*A)
#endif
        for( int i = 0; i <= ICV_CUBIC_TAB_SIZE; i++ )
        {
            float x = (float)i/ICV_CUBIC_TAB_SIZE;
            icvCubicCoeffs[i*2] = (float)ICV_CUBIC_1(x);
            x += 1.f;
            icvCubicCoeffs[i*2+1] = (float)ICV_CUBIC_2(x);
        }

        inittab = 1;
    }
}


/****************************************************************************************\
*                                         Resize                                         *
\****************************************************************************************/

static CvStatus CV_STDCALL
icvResize_NN_8u_C1R( const uchar* src, int srcstep, CvSize ssize,
                     uchar* dst, int dststep, CvSize dsize, int pix_size )
{
    int* x_ofs = (int*)cvStackAlloc( dsize.width * sizeof(x_ofs[0]) );
    int pix_size4 = pix_size / sizeof(int);
    int x, y, t;

    for( x = 0; x < dsize.width; x++ )
    {
        t = (ssize.width*x*2 + MIN(ssize.width, dsize.width) - 1)/(dsize.width*2);
        t -= t >= ssize.width;
        x_ofs[x] = t*pix_size;
    }

    for( y = 0; y < dsize.height; y++, dst += dststep )
    {
        const uchar* tsrc;
        t = (ssize.height*y*2 + MIN(ssize.height, dsize.height) - 1)/(dsize.height*2);
        t -= t >= ssize.height;
        tsrc = src + srcstep*t;

        switch( pix_size )
        {
        case 1:
            for( x = 0; x <= dsize.width - 2; x += 2 )
            {
                uchar t0 = tsrc[x_ofs[x]];
                uchar t1 = tsrc[x_ofs[x+1]];

                dst[x] = t0;
                dst[x+1] = t1;
            }

            for( ; x < dsize.width; x++ )
                dst[x] = tsrc[x_ofs[x]];
            break;
        case 2:
            for( x = 0; x < dsize.width; x++ )
                *(ushort*)(dst + x*2) = *(ushort*)(tsrc + x_ofs[x]);
            break;
        case 3:
            for( x = 0; x < dsize.width; x++ )
            {
                const uchar* _tsrc = tsrc + x_ofs[x];
                dst[x*3] = _tsrc[0]; dst[x*3+1] = _tsrc[1]; dst[x*3+2] = _tsrc[2];
            }
            break;
        case 4:
            for( x = 0; x < dsize.width; x++ )
                *(int*)(dst + x*4) = *(int*)(tsrc + x_ofs[x]);
            break;
        case 6:
            for( x = 0; x < dsize.width; x++ )
            {
                const ushort* _tsrc = (const ushort*)(tsrc + x_ofs[x]);
                ushort* _tdst = (ushort*)(dst + x*6);
                _tdst[0] = _tsrc[0]; _tdst[1] = _tsrc[1]; _tdst[2] = _tsrc[2];
            }
            break;
        default:
            for( x = 0; x < dsize.width; x++ )
                CV_MEMCPY_INT( dst + x*pix_size, tsrc + x_ofs[x], pix_size4 );
        }
    }

    return CV_OK;
}


typedef struct CvResizeAlpha
{
    int idx;
    union
    {
        float alpha;
        int ialpha;
    };
}
CvResizeAlpha;


#define  ICV_DEF_RESIZE_BILINEAR_FUNC( flavor, arrtype, worktype, alpha_field,  \
                                       mul_one_macro, descale_macro )           \
static CvStatus CV_STDCALL                                                      \
icvResize_Bilinear_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                                   arrtype* dst, int dststep, CvSize dsize,     \
                                   int cn, int xmax,                            \
                                   const CvResizeAlpha* xofs,                   \
                                   const CvResizeAlpha* yofs,                   \
                                   worktype* buf0, worktype* buf1 )             \
{                                                                               \
    int prev_sy0 = -1, prev_sy1 = -1;                                           \
    int k, dx, dy;                                                              \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
    xmax *= cn;                                                                 \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
    {                                                                           \
        worktype fy = yofs[dy].alpha_field, *swap_t;                            \
        int sy0 = yofs[dy].idx, sy1 = sy0 + (fy > 0 && sy0 < ssize.height-1);   \
                                                                                \
        if( sy0 == prev_sy0 && sy1 == prev_sy1 )                                \
            k = 2;                                                              \
        else if( sy0 == prev_sy1 )                                              \
        {                                                                       \
            CV_SWAP( buf0, buf1, swap_t );                                      \
            k = 1;                                                              \
        }                                                                       \
        else                                                                    \
            k = 0;                                                              \
                                                                                \
        for( ; k < 2; k++ )                                                     \
        {                                                                       \
            worktype* _buf = k == 0 ? buf0 : buf1;                              \
            const arrtype* _src;                                                \
            int sy = k == 0 ? sy0 : sy1;                                        \
            if( k == 1 && sy1 == sy0 )                                          \
            {                                                                   \
                memcpy( buf1, buf0, dsize.width*sizeof(buf0[0]) );              \
                continue;                                                       \
            }                                                                   \
                                                                                \
            _src = src + sy*srcstep;                                            \
            for( dx = 0; dx < xmax; dx++ )                                      \
            {                                                                   \
                int sx = xofs[dx].idx;                                          \
                worktype fx = xofs[dx].alpha_field;                             \
                worktype t = _src[sx];                                          \
                _buf[dx] = mul_one_macro(t) + fx*(_src[sx+cn] - t);             \
            }                                                                   \
                                                                                \
            for( ; dx < dsize.width; dx++ )                                     \
                _buf[dx] = mul_one_macro(_src[xofs[dx].idx]);                   \
        }                                                                       \
                                                                                \
        prev_sy0 = sy0;                                                         \
        prev_sy1 = sy1;                                                         \
                                                                                \
        if( sy0 == sy1 )                                                        \
            for( dx = 0; dx < dsize.width; dx++ )                               \
                dst[dx] = (arrtype)descale_macro( mul_one_macro(buf0[dx]));     \
        else                                                                    \
            for( dx = 0; dx < dsize.width; dx++ )                               \
                dst[dx] = (arrtype)descale_macro( mul_one_macro(buf0[dx]) +     \
                                                  fy*(buf1[dx] - buf0[dx]));    \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


typedef struct CvDecimateAlpha
{
    int si, di;
    float alpha;
}
CvDecimateAlpha;


#define  ICV_DEF_RESIZE_AREA_FAST_FUNC( flavor, arrtype, worktype, cast_macro ) \
static CvStatus CV_STDCALL                                                      \
icvResize_AreaFast_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                              arrtype* dst, int dststep, CvSize dsize, int cn,  \
                              const int* ofs, const int* xofs )                 \
{                                                                               \
    int dy, dx, k = 0;                                                          \
    int scale_x = ssize.width/dsize.width;                                      \
    int scale_y = ssize.height/dsize.height;                                    \
    int area = scale_x*scale_y;                                                 \
    float scale = 1.f/(scale_x*scale_y);                                        \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
        for( dx = 0; dx < dsize.width; dx++ )                                   \
        {                                                                       \
            const arrtype* _src = src + dy*scale_y*srcstep + xofs[dx];          \
            worktype sum = 0;                                                   \
                                                                                \
            for( k = 0; k <= area - 4; k += 4 )                                 \
                sum += _src[ofs[k]] + _src[ofs[k+1]] +                          \
                       _src[ofs[k+2]] + _src[ofs[k+3]];                         \
                                                                                \
            for( ; k < area; k++ )                                              \
                sum += _src[ofs[k]];                                            \
                                                                                \
            dst[dx] = (arrtype)cast_macro( sum*scale );                         \
        }                                                                       \
                                                                                \
    return CV_OK;                                                               \
}


#define  ICV_DEF_RESIZE_AREA_FUNC( flavor, arrtype, load_macro, cast_macro )    \
static CvStatus CV_STDCALL                                                      \
icvResize_Area_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,   \
                               arrtype* dst, int dststep, CvSize dsize,         \
                               int cn, const CvDecimateAlpha* xofs,             \
                               int xofs_count, float* buf, float* sum )         \
{                                                                               \
    int k, sy, dx, cur_dy = 0;                                                  \
    float scale_y = (float)ssize.height/dsize.height;                           \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
                                                                                \
    for( sy = 0; sy < ssize.height; sy++, src += srcstep )                      \
    {                                                                           \
        if( cn == 1 )                                                           \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                buf[dxn] = buf[dxn] + load_macro(src[xofs[k].si])*alpha;        \
            }                                                                   \
        else if( cn == 2 )                                                      \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1;                                 \
            }                                                                   \
        else if( cn == 3 )                                                      \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                float t2 = buf[dxn+2] + load_macro(src[sxn+2])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1; buf[dxn+2] = t2;                \
            }                                                                   \
        else                                                                    \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1;                                 \
                t0 = buf[dxn+2] + load_macro(src[sxn+2])*alpha;                 \
                t1 = buf[dxn+3] + load_macro(src[sxn+3])*alpha;                 \
                buf[dxn+2] = t0; buf[dxn+3] = t1;                               \
            }                                                                   \
                                                                                \
        if( (cur_dy + 1)*scale_y <= sy + 1 || sy == ssize.height - 1 )          \
        {                                                                       \
            float beta = sy + 1 - (cur_dy+1)*scale_y, beta1;                    \
            beta = MAX( beta, 0 );                                              \
            beta1 = 1 - beta;                                                   \
            if( fabs(beta) < 1e-3 )                                             \
                for( dx = 0; dx < dsize.width; dx++ )                           \
                {                                                               \
                    dst[dx] = (arrtype)cast_macro(sum[dx] + buf[dx]);           \
                    sum[dx] = buf[dx] = 0;                                      \
                }                                                               \
            else                                                                \
                for( dx = 0; dx < dsize.width; dx++ )                           \
                {                                                               \
                    dst[dx] = (arrtype)cast_macro(sum[dx] + buf[dx]*beta1);     \
                    sum[dx] = buf[dx]*beta;                                     \
                    buf[dx] = 0;                                                \
                }                                                               \
            dst += dststep;                                                     \
            cur_dy++;                                                           \
        }                                                                       \
        else                                                                    \
            for( dx = 0; dx < dsize.width; dx += 2 )                            \
            {                                                                   \
                float t0 = sum[dx] + buf[dx];                                   \
                float t1 = sum[dx+1] + buf[dx+1];                               \
                sum[dx] = t0; sum[dx+1] = t1;                                   \
                buf[dx] = buf[dx+1] = 0;                                        \
            }                                                                   \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


#define  ICV_DEF_RESIZE_BICUBIC_FUNC( flavor, arrtype, worktype, load_macro,    \
                                      cast_macro1, cast_macro2 )                \
static CvStatus CV_STDCALL                                                      \
icvResize_Bicubic_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                                  arrtype* dst, int dststep, CvSize dsize,      \
                                  int cn, int xmin, int xmax,                   \
                                  const CvResizeAlpha* xofs, float** buf )      \
{                                                                               \
    float scale_y = (float)ssize.height/dsize.height;                           \
    int dx, dy, sx, sy, sy2, ify;                                               \
    int prev_sy2 = -2;                                                          \
                                                                                \
    xmin *= cn; xmax *= cn;                                                     \
    dsize.width *= cn;                                                          \
    ssize.width *= cn;                                                          \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
    {                                                                           \
        float w0, w1, w2, w3;                                                   \
        float fy, x, sum;                                                       \
        float *row, *row0, *row1, *row2, *row3;                                 \
        int k1, k = 4;                                                          \
                                                                                \
        fy = dy*scale_y;                                                        \
        sy = cvFloor(fy);                                                       \
        fy -= sy;                                                               \
        ify = cvRound(fy*ICV_CUBIC_TAB_SIZE);                                   \
        sy2 = sy + 2;                                                           \
                                                                                \
        if( sy2 > prev_sy2 )                                                    \
        {                                                                       \
            int delta = prev_sy2 - sy + 2;                                      \
            for( k = 0; k < delta; k++ )                                        \
                CV_SWAP( buf[k], buf[k+4-delta], row );                         \
        }                                                                       \
                                                                                \
        for( sy += k - 1; k < 4; k++, sy++ )                                    \
        {                                                                       \
            const arrtype* _src = src + sy*srcstep;                             \
                                                                                \
            row = buf[k];                                                       \
            if( sy < 0 )                                                        \
                continue;                                                       \
            if( sy >= ssize.height )                                            \
            {                                                                   \
                assert( k > 0 );                                                \
                memcpy( row, buf[k-1], dsize.width*sizeof(row[0]) );            \
                continue;                                                       \
            }                                                                   \
                                                                                \
            for( dx = 0; dx < xmin; dx++ )                                      \
            {                                                                   \
                int ifx = xofs[dx].ialpha, sx0 = xofs[dx].idx;                  \
                sx = sx0 + cn*2;                                                \
                while( sx >= ssize.width )                                      \
                    sx -= cn;                                                   \
                x = load_macro(_src[sx]);                                       \
                sum = x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2 + 1];       \
                if( (unsigned)(sx = sx0 + cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2];          \
                if( (unsigned)(sx = sx0) < (unsigned)ssize.width )              \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[ifx*2];                                 \
                if( (unsigned)(sx = sx0 - cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                row[dx] = sum + x*icvCubicCoeffs[ifx*2 + 1];                    \
            }                                                                   \
                                                                                \
            for( ; dx < xmax; dx++ )                                            \
            {                                                                   \
                int ifx = xofs[dx].ialpha;                                      \
                int sx0 = xofs[dx].idx;                                         \
                row[dx] = _src[sx0 - cn]*icvCubicCoeffs[ifx*2 + 1] +            \
                    _src[sx0]*icvCubicCoeffs[ifx*2] +                           \
                    _src[sx0 + cn]*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] + \
                    _src[sx0 + cn*2]*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
            }                                                                   \
                                                                                \
            for( ; dx < dsize.width; dx++ )                                     \
            {                                                                   \
                int ifx = xofs[dx].ialpha, sx0 = xofs[dx].idx;                  \
                x = load_macro(_src[sx0 - cn]);                                 \
                sum = x*icvCubicCoeffs[ifx*2 + 1];                              \
                if( (unsigned)(sx = sx0) < (unsigned)ssize.width )              \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[ifx*2];                                 \
                if( (unsigned)(sx = sx0 + cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2];          \
                if( (unsigned)(sx = sx0 + cn*2) < (unsigned)ssize.width )       \
                    x = load_macro(_src[sx]);                                   \
                row[dx] = sum + x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1]; \
            }                                                                   \
                                                                                \
            if( sy == 0 )                                                       \
                for( k1 = 0; k1 < k; k1++ )                                     \
                    memcpy( buf[k1], row, dsize.width*sizeof(row[0]));          \
        }                                                                       \
                                                                                \
        prev_sy2 = sy2;                                                         \
                                                                                \
        row0 = buf[0]; row1 = buf[1];                                           \
        row2 = buf[2]; row3 = buf[3];                                           \
                                                                                \
        w0 = icvCubicCoeffs[ify*2+1];                                           \
        w1 = icvCubicCoeffs[ify*2];                                             \
        w2 = icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ify)*2];                      \
        w3 = icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ify)*2 + 1];                  \
                                                                                \
        for( dx = 0; dx < dsize.width; dx++ )                                   \
        {                                                                       \
            worktype val = cast_macro1( row0[dx]*w0 + row1[dx]*w1 +             \
                                        row2[dx]*w2 + row3[dx]*w3 );            \
            dst[dx] = cast_macro2(val);                                         \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_RESIZE_BILINEAR_FUNC( 8u, uchar, int, ialpha,
                              ICV_WARP_MUL_ONE_8U, ICV_WARP_DESCALE_8U )
ICV_DEF_RESIZE_BILINEAR_FUNC( 16u, ushort, float, alpha, CV_NOP, cvRound )
ICV_DEF_RESIZE_BILINEAR_FUNC( 32f, float, float, alpha, CV_NOP, CV_NOP )

ICV_DEF_RESIZE_BICUBIC_FUNC( 8u, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U )
ICV_DEF_RESIZE_BICUBIC_FUNC( 16u, ushort, int, CV_NOP, cvRound, CV_CAST_16U )
ICV_DEF_RESIZE_BICUBIC_FUNC( 32f, float, float, CV_NOP, CV_NOP, CV_NOP )

ICV_DEF_RESIZE_AREA_FAST_FUNC( 8u, uchar, int, cvRound )
ICV_DEF_RESIZE_AREA_FAST_FUNC( 16u, ushort, int, cvRound )
ICV_DEF_RESIZE_AREA_FAST_FUNC( 32f, float, float, CV_NOP )

ICV_DEF_RESIZE_AREA_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_RESIZE_AREA_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_RESIZE_AREA_FUNC( 32f, float, CV_NOP, CV_NOP )


static void icvInitResizeTab( CvFuncTable* bilin_tab,
                              CvFuncTable* bicube_tab,
                              CvFuncTable* areafast_tab,
                              CvFuncTable* area_tab )
{
    bilin_tab->fn_2d[CV_8U] = (void*)icvResize_Bilinear_8u_CnR;
    bilin_tab->fn_2d[CV_16U] = (void*)icvResize_Bilinear_16u_CnR;
    bilin_tab->fn_2d[CV_32F] = (void*)icvResize_Bilinear_32f_CnR;

    bicube_tab->fn_2d[CV_8U] = (void*)icvResize_Bicubic_8u_CnR;
    bicube_tab->fn_2d[CV_16U] = (void*)icvResize_Bicubic_16u_CnR;
    bicube_tab->fn_2d[CV_32F] = (void*)icvResize_Bicubic_32f_CnR;

    areafast_tab->fn_2d[CV_8U] = (void*)icvResize_AreaFast_8u_CnR;
    areafast_tab->fn_2d[CV_16U] = (void*)icvResize_AreaFast_16u_CnR;
    areafast_tab->fn_2d[CV_32F] = (void*)icvResize_AreaFast_32f_CnR;

    area_tab->fn_2d[CV_8U] = (void*)icvResize_Area_8u_CnR;
    area_tab->fn_2d[CV_16U] = (void*)icvResize_Area_16u_CnR;
    area_tab->fn_2d[CV_32F] = (void*)icvResize_Area_32f_CnR;
}


typedef CvStatus (CV_STDCALL * CvResizeBilinearFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, int xmax, const CvResizeAlpha* xofs,
                      const CvResizeAlpha* yofs, float* buf0, float* buf1 );

typedef CvStatus (CV_STDCALL * CvResizeBicubicFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, int xmin, int xmax,
                      const CvResizeAlpha* xofs, float** buf );

typedef CvStatus (CV_STDCALL * CvResizeAreaFastFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, const int* ofs, const int *xofs );

typedef CvStatus (CV_STDCALL * CvResizeAreaFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, const CvDecimateAlpha* xofs,
                      int xofs_count, float* buf, float* sum );


////////////////////////////////// IPP resize functions //////////////////////////////////

icvResize_8u_C1R_t icvResize_8u_C1R_p = 0;
icvResize_8u_C3R_t icvResize_8u_C3R_p = 0;
icvResize_8u_C4R_t icvResize_8u_C4R_p = 0;
icvResize_16u_C1R_t icvResize_16u_C1R_p = 0;
icvResize_16u_C3R_t icvResize_16u_C3R_p = 0;
icvResize_16u_C4R_t icvResize_16u_C4R_p = 0;
icvResize_32f_C1R_t icvResize_32f_C1R_p = 0;
icvResize_32f_C3R_t icvResize_32f_C3R_p = 0;
icvResize_32f_C4R_t icvResize_32f_C4R_p = 0;

typedef CvStatus (CV_STDCALL * CvResizeIPPFunc)
( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
  void* dst, int dststep, CvSize dstroi,
  double xfactor, double yfactor, int interpolation );

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvResize( const CvArr* srcarr, CvArr* dstarr, int method )
{
    static CvFuncTable bilin_tab, bicube_tab, areafast_tab, area_tab;
    static int inittab = 0;
    void* temp_buf = 0;

    CV_FUNCNAME( "cvResize" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize ssize, dsize;
    float scale_x, scale_y;
    int k, sx, sy, dx, dy;
    int type, depth, cn;
    
    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( CV_ARE_SIZES_EQ( src, dst ))
    {
        CV_CALL( cvCopy( src, dst ));
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !inittab )
    {
        icvInitResizeTab( &bilin_tab, &bicube_tab, &areafast_tab, &area_tab );
        inittab = 1;
    }

    ssize = cvGetMatSize( src );
    dsize = cvGetMatSize( dst );
    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    scale_x = (float)ssize.width/dsize.width;
    scale_y = (float)ssize.height/dsize.height;

    if( method == CV_INTER_CUBIC &&
        (MIN(ssize.width, dsize.width) <= 4 ||
        MIN(ssize.height, dsize.height) <= 4) )
        method = CV_INTER_LINEAR;

    if( icvResize_8u_C1R_p &&
        MIN(ssize.width, dsize.width) > 4 &&
        MIN(ssize.height, dsize.height) > 4 )
    {
        CvResizeIPPFunc ipp_func =
            type == CV_8UC1 ? icvResize_8u_C1R_p :
            type == CV_8UC3 ? icvResize_8u_C3R_p :
            type == CV_8UC4 ? icvResize_8u_C4R_p :
            type == CV_16UC1 ? icvResize_16u_C1R_p :
            type == CV_16UC3 ? icvResize_16u_C3R_p :
            type == CV_16UC4 ? icvResize_16u_C4R_p :
            type == CV_32FC1 ? icvResize_32f_C1R_p :
            type == CV_32FC3 ? icvResize_32f_C3R_p :
            type == CV_32FC4 ? icvResize_32f_C4R_p : 0;
        if( ipp_func && (CV_INTER_NN < method && method < CV_INTER_AREA))
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            IPPI_CALL( ipp_func( src->data.ptr, ssize, srcstep,
                                 cvRect(0,0,ssize.width,ssize.height),
                                 dst->data.ptr, dststep, dsize,
                                 (double)dsize.width/ssize.width,
                                 (double)dsize.height/ssize.height, 1 << method ));
            EXIT;
        }
    }

    if( method == CV_INTER_NN )
    {
        IPPI_CALL( icvResize_NN_8u_C1R( src->data.ptr, src->step, ssize,
                                        dst->data.ptr, dst->step, dsize,
                                        CV_ELEM_SIZE(src->type)));
    }
    else if( method == CV_INTER_LINEAR || method == CV_INTER_AREA )
    {
        if( method == CV_INTER_AREA &&
            ssize.width >= dsize.width && ssize.height >= dsize.height )
        {
            // "area" method for (scale_x > 1 & scale_y > 1)
            int iscale_x = cvRound(scale_x);
            int iscale_y = cvRound(scale_y);

            if( fabs(scale_x - iscale_x) < DBL_EPSILON &&
                fabs(scale_y - iscale_y) < DBL_EPSILON )
            {
                int area = iscale_x*iscale_y;
                int srcstep = src->step / CV_ELEM_SIZE(depth);
                int* ofs = (int*)cvStackAlloc( (area + dsize.width*cn)*sizeof(int) );
                int* xofs = ofs + area;
                CvResizeAreaFastFunc func = (CvResizeAreaFastFunc)areafast_tab.fn_2d[depth];

                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );
                
                for( sy = 0, k = 0; sy < iscale_y; sy++ )
                    for( sx = 0; sx < iscale_x; sx++ )
                        ofs[k++] = sy*srcstep + sx*cn;

                for( dx = 0; dx < dsize.width; dx++ )
                {
                    sx = dx*iscale_x*cn;
                    for( k = 0; k < cn; k++ )
                        xofs[dx*cn + k] = sx + k;
                }

                IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                                 dst->step, dsize, cn, ofs, xofs ));
            }
            else
            {
                int buf_len = dsize.width*cn + 4, buf_size, xofs_count = 0;
                float scale = 1.f/(scale_x*scale_y);
                float *buf, *sum;
                CvDecimateAlpha* xofs;
                CvResizeAreaFunc func = (CvResizeAreaFunc)area_tab.fn_2d[depth];

                if( !func || cn > 4 )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                buf_size = buf_len*2*sizeof(float) + ssize.width*2*sizeof(CvDecimateAlpha);
                if( buf_size < CV_MAX_LOCAL_SIZE )
                    buf = (float*)cvStackAlloc(buf_size);
                else
                    CV_CALL( temp_buf = buf = (float*)cvAlloc(buf_size));
                sum = buf + buf_len;
                xofs = (CvDecimateAlpha*)(sum + buf_len);

                for( dx = 0, k = 0; dx < dsize.width; dx++ )
                {
                    float fsx1 = dx*scale_x, fsx2 = fsx1 + scale_x;
                    int sx1 = cvCeil(fsx1), sx2 = cvFloor(fsx2);

                    assert( (unsigned)sx1 < (unsigned)ssize.width );

                    if( sx1 > fsx1 )
                    {
                        assert( k < ssize.width*2 );            
                        xofs[k].di = dx*cn;
                        xofs[k].si = (sx1-1)*cn;
                        xofs[k++].alpha = (sx1 - fsx1)*scale;
                    }

                    for( sx = sx1; sx < sx2; sx++ )
                    {
                        assert( k < ssize.width*2 );
                        xofs[k].di = dx*cn;
                        xofs[k].si = sx*cn;
                        xofs[k++].alpha = scale;
                    }

                    if( fsx2 - sx2 > 1e-3 )
                    {
                        assert( k < ssize.width*2 );
                        assert((unsigned)sx2 < (unsigned)ssize.width );
                        xofs[k].di = dx*cn;
                        xofs[k].si = sx2*cn;
                        xofs[k++].alpha = (fsx2 - sx2)*scale;
                    }
                }

                xofs_count = k;
                memset( sum, 0, buf_len*sizeof(float) );
                memset( buf, 0, buf_len*sizeof(float) );

                IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                                 dst->step, dsize, cn, xofs, xofs_count, buf, sum ));
            }
        }
        else // true "area" method for the cases (scale_x > 1 & scale_y < 1) and
             // (scale_x < 1 & scale_y > 1) is not implemented.
             // instead, it is emulated via some variant of bilinear interpolation.
        {
            float inv_scale_x = (float)dsize.width/ssize.width;
            float inv_scale_y = (float)dsize.height/ssize.height;
            int xmax = dsize.width, width = dsize.width*cn, buf_size;
            float *buf0, *buf1;
            CvResizeAlpha *xofs, *yofs;
            int area_mode = method == CV_INTER_AREA;
            float fx, fy;
            CvResizeBilinearFunc func = (CvResizeBilinearFunc)bilin_tab.fn_2d[depth];

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            buf_size = width*2*sizeof(float) + (width + dsize.height)*sizeof(CvResizeAlpha);
            if( buf_size < CV_MAX_LOCAL_SIZE )
                buf0 = (float*)cvStackAlloc(buf_size);
            else
                CV_CALL( temp_buf = buf0 = (float*)cvAlloc(buf_size));
            buf1 = buf0 + width;
            xofs = (CvResizeAlpha*)(buf1 + width);
            yofs = xofs + width;

            for( dx = 0; dx < dsize.width; dx++ )
            {
                if( !area_mode )
                {
                    fx = (float)((dx+0.5)*scale_x - 0.5);
                    sx = cvFloor(fx);
                    fx -= sx;
                }
                else
                {
                    sx = cvFloor(dx*scale_x);
                    fx = (dx+1) - (sx+1)*inv_scale_x;
                    fx = fx <= 0 ? 0.f : fx - cvFloor(fx);
                }

                if( sx < 0 )
                    fx = 0, sx = 0;

                if( sx >= ssize.width-1 )
                {
                    fx = 0, sx = ssize.width-1;
                    if( xmax >= dsize.width )
                        xmax = dx;
                }

                if( depth != CV_8U )
                    for( k = 0, sx *= cn; k < cn; k++ )
                        xofs[dx*cn + k].idx = sx + k, xofs[dx*cn + k].alpha = fx;
                else
                    for( k = 0, sx *= cn; k < cn; k++ )
                        xofs[dx*cn + k].idx = sx + k,
                        xofs[dx*cn + k].ialpha = CV_FLT_TO_FIX(fx, ICV_WARP_SHIFT);
            }

            for( dy = 0; dy < dsize.height; dy++ )
            {
                if( !area_mode )
                {
                    fy = (float)((dy+0.5)*scale_y - 0.5);
                    sy = cvFloor(fy);
                    fy -= sy;
                    if( sy < 0 )
                        sy = 0, fy = 0;
                }
                else
                {
                    sy = cvFloor(dy*scale_y);
                    fy = (dy+1) - (sy+1)*inv_scale_y;
                    fy = fy <= 0 ? 0.f : fy - cvFloor(fy);
                }

                yofs[dy].idx = sy;
                if( depth != CV_8U )
                    yofs[dy].alpha = fy;
                else
                    yofs[dy].ialpha = CV_FLT_TO_FIX(fy, ICV_WARP_SHIFT);
            }

            IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                             dst->step, dsize, cn, xmax, xofs, yofs, buf0, buf1 ));
        }
    }
    else if( method == CV_INTER_CUBIC )
    {
        int width = dsize.width*cn, buf_size;
        int xmin = dsize.width, xmax = -1;
        CvResizeAlpha* xofs;
        float* buf[4];
        CvResizeBicubicFunc func = (CvResizeBicubicFunc)bicube_tab.fn_2d[depth];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        
        buf_size = width*(4*sizeof(float) + sizeof(xofs[0]));
        if( buf_size < CV_MAX_LOCAL_SIZE )
            buf[0] = (float*)cvStackAlloc(buf_size);
        else
            CV_CALL( temp_buf = buf[0] = (float*)cvAlloc(buf_size));

        for( k = 1; k < 4; k++ )
            buf[k] = buf[k-1] + width;
        xofs = (CvResizeAlpha*)(buf[3] + width);

        icvInitCubicCoeffTab();

        for( dx = 0; dx < dsize.width; dx++ )
        {
            float fx = dx*scale_x;
            sx = cvFloor(fx);
            fx -= sx;
            int ifx = cvRound(fx*ICV_CUBIC_TAB_SIZE);
            if( sx-1 >= 0 && xmin > dx )
                xmin = dx;
            if( sx+2 < ssize.width )
                xmax = dx + 1;
        
            // at least one of 4 points should be within the image - to
            // be able to set other points to the same value. see the loops
            // for( dx = 0; dx < xmin; dx++ ) ... and for( ; dx < width; dx++ ) ...
            if( sx < -2 )
                sx = -2;
            else if( sx > ssize.width )
                sx = ssize.width;

            for( k = 0; k < cn; k++ )
            {
                xofs[dx*cn + k].idx = sx*cn + k;
                xofs[dx*cn + k].ialpha = ifx;
            }
        }
    
        IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                         dst->step, dsize, cn, xmin, xmax, xofs, buf ));
    }
    else
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported interpolation method" );

    __END__;

    cvFree( &temp_buf );
}


/****************************************************************************************\
*                                     WarpAffine                                         *
\****************************************************************************************/

#define ICV_DEF_WARP_AFFINE_BILINEAR_FUNC( flavor, arrtype, worktype,       \
            scale_alpha_macro, mul_one_macro, descale_macro, cast_macro )   \
static CvStatus CV_STDCALL                                                  \
icvWarpAffine_Bilinear_##flavor##_CnR(                                      \
    const arrtype* src, int step, CvSize ssize,                             \
    arrtype* dst, int dststep, CvSize dsize,                                \
    const double* matrix, int cn,                                           \
    const arrtype* fillval, const int* ofs )                                \
{                                                                           \
    int x, y, k;                                                            \
    double  A12 = matrix[1], b1 = matrix[2];                                \
    double  A22 = matrix[4], b2 = matrix[5];                                \
                                                                            \
    step /= sizeof(src[0]);                                                 \
    dststep /= sizeof(dst[0]);                                              \
                                                                            \
    for( y = 0; y < dsize.height; y++, dst += dststep )                     \
    {                                                                       \
        int xs = CV_FLT_TO_FIX( A12*y + b1, ICV_WARP_SHIFT );               \
        int ys = CV_FLT_TO_FIX( A22*y + b2, ICV_WARP_SHIFT );               \
                                                                            \
        for( x = 0; x < dsize.width; x++ )                                  \
        {                                                                   \
            int ixs = xs + ofs[x*2];                                        \
            int iys = ys + ofs[x*2+1];                                      \
            worktype a = scale_alpha_macro( ixs & ICV_WARP_MASK );          \
            worktype b = scale_alpha_macro( iys & ICV_WARP_MASK );          \
            worktype p0, p1;                                                \
            ixs >>= ICV_WARP_SHIFT;                                         \
            iys >>= ICV_WARP_SHIFT;                                         \
                                                                            \
            if( (unsigned)ixs < (unsigned)(ssize.width - 1) &&              \
                (unsigned)iys < (unsigned)(ssize.height - 1) )              \
            {                                                               \
                const arrtype* ptr = src + step*iys + ixs*cn;               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = mul_one_macro(ptr[k]) +                            \
                        a * (ptr[k+cn] - ptr[k]);                           \
                    p1 = mul_one_macro(ptr[k+step]) +                       \
                        a * (ptr[k+cn+step] - ptr[k+step]);                 \
                    p0 = descale_macro(mul_one_macro(p0) + b*(p1 - p0));    \
                    dst[x*cn+k] = (arrtype)cast_macro(p0);                  \
                }                                                           \
            }                                                               \
            else if( (unsigned)(ixs+1) < (unsigned)(ssize.width+1) &&       \
                     (unsigned)(iys+1) < (unsigned)(ssize.height+1))        \
            {                                                               \
                int x0 = ICV_WARP_CLIP_X( ixs );                            \
                int y0 = ICV_WARP_CLIP_Y( iys );                            \
                int x1 = ICV_WARP_CLIP_X( ixs + 1 );                        \
                int y1 = ICV_WARP_CLIP_Y( iys + 1 );                        \
                const arrtype* ptr0, *ptr1, *ptr2, *ptr3;                   \
                                                                            \
                ptr0 = src + y0*step + x0*cn;                               \
                ptr1 = src + y0*step + x1*cn;                               \
                ptr2 = src + y1*step + x0*cn;                               \
                ptr3 = src + y1*step + x1*cn;                               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = mul_one_macro(ptr0[k]) + a * (ptr1[k] - ptr0[k]);  \
                    p1 = mul_one_macro(ptr2[k]) + a * (ptr3[k] - ptr2[k]);  \
                    p0 = descale_macro( mul_one_macro(p0) + b*(p1 - p0) );  \
                    dst[x*cn+k] = (arrtype)cast_macro(p0);                  \
                }                                                           \
            }                                                               \
            else if( fillval )                                              \
                for( k = 0; k < cn; k++ )                                   \
                    dst[x*cn+k] = fillval[k];                               \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_WARP_SCALE_ALPHA(x) ((x)*(1./(ICV_WARP_MASK+1)))

ICV_DEF_WARP_AFFINE_BILINEAR_FUNC( 8u, uchar, int, CV_NOP, ICV_WARP_MUL_ONE_8U,
                                   ICV_WARP_DESCALE_8U, CV_NOP )
//ICV_DEF_WARP_AFFINE_BILINEAR_FUNC( 8u, uchar, double, ICV_WARP_SCALE_ALPHA, CV_NOP,
//                                   CV_NOP, ICV_WARP_CAST_8U )
ICV_DEF_WARP_AFFINE_BILINEAR_FUNC( 16u, ushort, double, ICV_WARP_SCALE_ALPHA, CV_NOP,
                                   CV_NOP, cvRound )
ICV_DEF_WARP_AFFINE_BILINEAR_FUNC( 32f, float, double, ICV_WARP_SCALE_ALPHA, CV_NOP,
                                   CV_NOP, CV_NOP )


typedef CvStatus (CV_STDCALL * CvWarpAffineFunc)(
    const void* src, int srcstep, CvSize ssize,
    void* dst, int dststep, CvSize dsize,
    const double* matrix, int cn,
    const void* fillval, const int* ofs );

static void icvInitWarpAffineTab( CvFuncTable* bilin_tab )
{
    bilin_tab->fn_2d[CV_8U] = (void*)icvWarpAffine_Bilinear_8u_CnR;
    bilin_tab->fn_2d[CV_16U] = (void*)icvWarpAffine_Bilinear_16u_CnR;
    bilin_tab->fn_2d[CV_32F] = (void*)icvWarpAffine_Bilinear_32f_CnR;
}


/////////////////////////////// IPP warpaffine functions /////////////////////////////////

icvWarpAffineBack_8u_C1R_t icvWarpAffineBack_8u_C1R_p = 0;
icvWarpAffineBack_8u_C3R_t icvWarpAffineBack_8u_C3R_p = 0;
icvWarpAffineBack_8u_C4R_t icvWarpAffineBack_8u_C4R_p = 0;
icvWarpAffineBack_32f_C1R_t icvWarpAffineBack_32f_C1R_p = 0;
icvWarpAffineBack_32f_C3R_t icvWarpAffineBack_32f_C3R_p = 0;
icvWarpAffineBack_32f_C4R_t icvWarpAffineBack_32f_C4R_p = 0;

typedef CvStatus (CV_STDCALL * CvWarpAffineBackIPPFunc)
( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
  void* dst, int dststep, CvRect dstroi,
  const double* coeffs, int interpolation );

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvWarpAffine( const CvArr* srcarr, CvArr* dstarr, const CvMat* matrix,
              int flags, CvScalar fillval )
{
    static CvFuncTable bilin_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvWarpAffine" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    int k, type, depth, cn, *ofs = 0;
    double src_matrix[6], dst_matrix[6];
    double fillbuf[4];
    int method = flags & 3;
    CvMat srcAb = cvMat( 2, 3, CV_64F, src_matrix ),
          dstAb = cvMat( 2, 3, CV_64F, dst_matrix ),
          A, b, invA, invAb;
    CvWarpAffineFunc func;
    CvSize ssize, dsize;
    
    if( !inittab )
    {
        icvInitWarpAffineTab( &bilin_tab );
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_IS_MAT(matrix) || CV_MAT_CN(matrix->type) != 1 ||
        CV_MAT_DEPTH(matrix->type) < CV_32F || matrix->rows != 2 || matrix->cols != 3 )
        CV_ERROR( CV_StsBadArg,
        "Transformation matrix should be 2x3 floating-point single-channel matrix" );

    if( flags & CV_WARP_INVERSE_MAP )
        cvConvertScale( matrix, &dstAb );
    else
    {
        // [R|t] -> [R^-1 | -(R^-1)*t]
        cvConvertScale( matrix, &srcAb );
        cvGetCols( &srcAb, &A, 0, 2 );
        cvGetCol( &srcAb, &b, 2 );
        cvGetCols( &dstAb, &invA, 0, 2 );
        cvGetCol( &dstAb, &invAb, 2 );
        cvInvert( &A, &invA, CV_SVD );
        cvGEMM( &invA, &b, -1, 0, 0, &invAb );
    }

    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( cn > 4 )
        CV_ERROR( CV_BadNumChannels, "" );

    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);

    if( icvWarpAffineBack_8u_C1R_p && MIN( ssize.width, dsize.width ) >= 4 &&
        MIN( ssize.height, dsize.height ) >= 4 )
    {
        CvWarpAffineBackIPPFunc ipp_func =
            type == CV_8UC1 ? icvWarpAffineBack_8u_C1R_p :
            type == CV_8UC3 ? icvWarpAffineBack_8u_C3R_p :
            type == CV_8UC4 ? icvWarpAffineBack_8u_C4R_p :
            type == CV_32FC1 ? icvWarpAffineBack_32f_C1R_p :
            type == CV_32FC3 ? icvWarpAffineBack_32f_C3R_p :
            type == CV_32FC4 ? icvWarpAffineBack_32f_C4R_p : 0;
        
        if( ipp_func && CV_INTER_NN <= method && method <= CV_INTER_AREA )
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            CvRect srcroi = {0, 0, ssize.width, ssize.height};
            CvRect dstroi = {0, 0, dsize.width, dsize.height};

            // this is not the most efficient way to fill outliers
            if( flags & CV_WARP_FILL_OUTLIERS )
                cvSet( dst, fillval );

            if( ipp_func( src->data.ptr, ssize, srcstep, srcroi,
                          dst->data.ptr, dststep, dstroi,
                          dstAb.data.db, 1 << method ) >= 0 )
                EXIT;
        }
    }

    cvScalarToRawData( &fillval, fillbuf, CV_MAT_TYPE(src->type), 0 );
    ofs = (int*)cvStackAlloc( dst->cols*2*sizeof(ofs[0]) );
    for( k = 0; k < dst->cols; k++ )
    {
        ofs[2*k] = CV_FLT_TO_FIX( dst_matrix[0]*k, ICV_WARP_SHIFT );
        ofs[2*k+1] = CV_FLT_TO_FIX( dst_matrix[3]*k, ICV_WARP_SHIFT );
    }

    /*if( method == CV_INTER_LINEAR )*/
    {
        func = (CvWarpAffineFunc)bilin_tab.fn_2d[depth];
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                         dst->step, dsize, dst_matrix, cn,
                         flags & CV_WARP_FILL_OUTLIERS ? fillbuf : 0, ofs ));
    }

    __END__;
}


CV_IMPL CvMat*
cv2DRotationMatrix( CvPoint2D32f center, double angle,
                    double scale, CvMat* matrix )
{
    CV_FUNCNAME( "cvGetRotationMatrix" );

    __BEGIN__;

    double m[2][3];
    CvMat M = cvMat( 2, 3, CV_64FC1, m );
    double alpha, beta;

    if( !matrix )
        CV_ERROR( CV_StsNullPtr, "" );

    angle *= CV_PI/180;
    alpha = cos(angle)*scale;
    beta = sin(angle)*scale;

    m[0][0] = alpha;
    m[0][1] = beta;
    m[0][2] = (1-alpha)*center.x - beta*center.y;
    m[1][0] = -beta;
    m[1][1] = alpha;
    m[1][2] = beta*center.x + (1-alpha)*center.y;

    cvConvert( &M, matrix );

    __END__;

    return matrix;
}


/****************************************************************************************\
*                                    WarpPerspective                                     *
\****************************************************************************************/

#define ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( flavor, arrtype, load_macro, cast_macro )\
static CvStatus CV_STDCALL                                                  \
icvWarpPerspective_Bilinear_##flavor##_CnR(                                 \
    const arrtype* src, int step, CvSize ssize,                             \
    arrtype* dst, int dststep, CvSize dsize,                                \
    const double* matrix, int cn,                                           \
    const arrtype* fillval )                                                \
{                                                                           \
    int x, y, k;                                                            \
    float A11 = (float)matrix[0], A12 = (float)matrix[1], A13 = (float)matrix[2];\
    float A21 = (float)matrix[3], A22 = (float)matrix[4], A23 = (float)matrix[5];\
    float A31 = (float)matrix[6], A32 = (float)matrix[7], A33 = (float)matrix[8];\
                                                                            \
    step /= sizeof(src[0]);                                                 \
    dststep /= sizeof(dst[0]);                                              \
                                                                            \
    for( y = 0; y < dsize.height; y++, dst += dststep )                     \
    {                                                                       \
        float xs0 = A12*y + A13;                                            \
        float ys0 = A22*y + A23;                                            \
        float ws = A32*y + A33;                                             \
                                                                            \
        for( x = 0; x < dsize.width; x++, xs0 += A11, ys0 += A21, ws += A31 )\
        {                                                                   \
            float inv_ws = 1.f/ws;                                          \
            float xs = xs0*inv_ws;                                          \
            float ys = ys0*inv_ws;                                          \
            int ixs = cvFloor(xs);                                          \
            int iys = cvFloor(ys);                                          \
            float a = xs - ixs;                                             \
            float b = ys - iys;                                             \
            float p0, p1;                                                   \
                                                                            \
            if( (unsigned)ixs < (unsigned)(ssize.width - 1) &&              \
                (unsigned)iys < (unsigned)(ssize.height - 1) )              \
            {                                                               \
                const arrtype* ptr = src + step*iys + ixs*cn;               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = load_macro(ptr[k]) +                               \
                        a * (load_macro(ptr[k+cn]) - load_macro(ptr[k]));   \
                    p1 = load_macro(ptr[k+step]) +                          \
                        a * (load_macro(ptr[k+cn+step]) -                   \
                             load_macro(ptr[k+step]));                      \
                    dst[x*cn+k] = (arrtype)cast_macro(p0 + b*(p1 - p0));    \
                }                                                           \
            }                                                               \
            else if( (unsigned)(ixs+1) < (unsigned)(ssize.width+1) &&       \
                     (unsigned)(iys+1) < (unsigned)(ssize.height+1))        \
            {                                                               \
                int x0 = ICV_WARP_CLIP_X( ixs );                            \
                int y0 = ICV_WARP_CLIP_Y( iys );                            \
                int x1 = ICV_WARP_CLIP_X( ixs + 1 );                        \
                int y1 = ICV_WARP_CLIP_Y( iys + 1 );                        \
                const arrtype* ptr0, *ptr1, *ptr2, *ptr3;                   \
                                                                            \
                ptr0 = src + y0*step + x0*cn;                               \
                ptr1 = src + y0*step + x1*cn;                               \
                ptr2 = src + y1*step + x0*cn;                               \
                ptr3 = src + y1*step + x1*cn;                               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = load_macro(ptr0[k]) +                              \
                        a * (load_macro(ptr1[k]) - load_macro(ptr0[k]));    \
                    p1 = load_macro(ptr2[k]) +                              \
                        a * (load_macro(ptr3[k]) - load_macro(ptr2[k]));    \
                    dst[x*cn+k] = (arrtype)cast_macro(p0 + b*(p1 - p0));    \
                }                                                           \
            }                                                               \
            else if( fillval )                                              \
                for( k = 0; k < cn; k++ )                                   \
                    dst[x*cn+k] = fillval[k];                               \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_WARP_SCALE_ALPHA(x) ((x)*(1./(ICV_WARP_MASK+1)))

ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 32f, float, CV_NOP, CV_NOP )

typedef CvStatus (CV_STDCALL * CvWarpPerspectiveFunc)(
    const void* src, int srcstep, CvSize ssize,
    void* dst, int dststep, CvSize dsize,
    const double* matrix, int cn, const void* fillval );

static void icvInitWarpPerspectiveTab( CvFuncTable* bilin_tab )
{
    bilin_tab->fn_2d[CV_8U] = (void*)icvWarpPerspective_Bilinear_8u_CnR;
    bilin_tab->fn_2d[CV_16U] = (void*)icvWarpPerspective_Bilinear_16u_CnR;
    bilin_tab->fn_2d[CV_32F] = (void*)icvWarpPerspective_Bilinear_32f_CnR;
}


/////////////////////////// IPP warpperspective functions ////////////////////////////////

icvWarpPerspectiveBack_8u_C1R_t icvWarpPerspectiveBack_8u_C1R_p = 0;
icvWarpPerspectiveBack_8u_C3R_t icvWarpPerspectiveBack_8u_C3R_p = 0;
icvWarpPerspectiveBack_8u_C4R_t icvWarpPerspectiveBack_8u_C4R_p = 0;
icvWarpPerspectiveBack_32f_C1R_t icvWarpPerspectiveBack_32f_C1R_p = 0;
icvWarpPerspectiveBack_32f_C3R_t icvWarpPerspectiveBack_32f_C3R_p = 0;
icvWarpPerspectiveBack_32f_C4R_t icvWarpPerspectiveBack_32f_C4R_p = 0;

icvWarpPerspective_8u_C1R_t icvWarpPerspective_8u_C1R_p = 0;
icvWarpPerspective_8u_C3R_t icvWarpPerspective_8u_C3R_p = 0;
icvWarpPerspective_8u_C4R_t icvWarpPerspective_8u_C4R_p = 0;
icvWarpPerspective_32f_C1R_t icvWarpPerspective_32f_C1R_p = 0;
icvWarpPerspective_32f_C3R_t icvWarpPerspective_32f_C3R_p = 0;
icvWarpPerspective_32f_C4R_t icvWarpPerspective_32f_C4R_p = 0;

typedef CvStatus (CV_STDCALL * CvWarpPerspectiveBackIPPFunc)
( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
  void* dst, int dststep, CvRect dstroi,
  const double* coeffs, int interpolation );

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvWarpPerspective( const CvArr* srcarr, CvArr* dstarr,
                   const CvMat* matrix, int flags, CvScalar fillval )
{
    static CvFuncTable bilin_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvWarpPerspective" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    int type, depth, cn;
    int method = flags & 3;
    double src_matrix[9], dst_matrix[9];
    double fillbuf[4];
    CvMat A = cvMat( 3, 3, CV_64F, src_matrix ),
          invA = cvMat( 3, 3, CV_64F, dst_matrix );
    CvWarpPerspectiveFunc func;
    CvSize ssize, dsize;

    if( method == CV_INTER_NN || method == CV_INTER_AREA )
        method = CV_INTER_LINEAR;
    
    if( !inittab )
    {
        icvInitWarpPerspectiveTab( &bilin_tab );
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_IS_MAT(matrix) || CV_MAT_CN(matrix->type) != 1 ||
        CV_MAT_DEPTH(matrix->type) < CV_32F || matrix->rows != 3 || matrix->cols != 3 )
        CV_ERROR( CV_StsBadArg,
        "Transformation matrix should be 3x3 floating-point single-channel matrix" );

    if( flags & CV_WARP_INVERSE_MAP )
        cvConvertScale( matrix, &invA );
    else
    {
        cvConvertScale( matrix, &A );
        cvInvert( &A, &invA, CV_SVD );
    }

    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( cn > 4 )
        CV_ERROR( CV_BadNumChannels, "" );
    
    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);
    
    if( icvWarpPerspectiveBack_8u_C1R_p )
    {
        CvWarpPerspectiveBackIPPFunc ipp_func =
            type == CV_8UC1 ? icvWarpPerspectiveBack_8u_C1R_p :
            type == CV_8UC3 ? icvWarpPerspectiveBack_8u_C3R_p :
            type == CV_8UC4 ? icvWarpPerspectiveBack_8u_C4R_p :
            type == CV_32FC1 ? icvWarpPerspectiveBack_32f_C1R_p :
            type == CV_32FC3 ? icvWarpPerspectiveBack_32f_C3R_p :
            type == CV_32FC4 ? icvWarpPerspectiveBack_32f_C4R_p : 0;
        
        if( ipp_func && CV_INTER_NN <= method && method <= CV_INTER_AREA &&
            MIN(ssize.width,ssize.height) >= 4 && MIN(dsize.width,dsize.height) >= 4 )
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            CvStatus status;
            CvRect srcroi = {0, 0, ssize.width, ssize.height};
            CvRect dstroi = {0, 0, dsize.width, dsize.height};

            // this is not the most efficient way to fill outliers
            if( flags & CV_WARP_FILL_OUTLIERS )
                cvSet( dst, fillval );

            status = ipp_func( src->data.ptr, ssize, srcstep, srcroi,
                               dst->data.ptr, dststep, dstroi,
                               invA.data.db, 1 << method );
            if( status >= 0 )
                EXIT;

            ipp_func = type == CV_8UC1 ? icvWarpPerspective_8u_C1R_p :
                type == CV_8UC3 ? icvWarpPerspective_8u_C3R_p :
                type == CV_8UC4 ? icvWarpPerspective_8u_C4R_p :
                type == CV_32FC1 ? icvWarpPerspective_32f_C1R_p :
                type == CV_32FC3 ? icvWarpPerspective_32f_C3R_p :
                type == CV_32FC4 ? icvWarpPerspective_32f_C4R_p : 0;

            if( ipp_func )
            {
                if( flags & CV_WARP_INVERSE_MAP )
                    cvInvert( &invA, &A, CV_SVD );

                status = ipp_func( src->data.ptr, ssize, srcstep, srcroi,
                               dst->data.ptr, dststep, dstroi,
                               A.data.db, 1 << method );
                if( status >= 0 )
                    EXIT;
            }
        }
    }

    cvScalarToRawData( &fillval, fillbuf, CV_MAT_TYPE(src->type), 0 );

    /*if( method == CV_INTER_LINEAR )*/
    {
        func = (CvWarpPerspectiveFunc)bilin_tab.fn_2d[depth];
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                         dst->step, dsize, dst_matrix, cn,
                         flags & CV_WARP_FILL_OUTLIERS ? fillbuf : 0 ));
    }

    __END__;
}


/* Calculates coefficients of perspective transformation
 * which maps (xi,yi) to (ui,vi), (i=1,2,3,4):
 *
 *      c00*xi + c01*yi + c02
 * ui = ---------------------
 *      c20*xi + c21*yi + c22
 *
 *      c10*xi + c11*yi + c12
 * vi = ---------------------
 *      c20*xi + c21*yi + c22
 *
 * Coefficients are calculated by solving linear system:
 * / x0 y0  1  0  0  0 -x0*u0 -y0*u0 \ /c00\ /u0\
 * | x1 y1  1  0  0  0 -x1*u1 -y1*u1 | |c01| |u1|
 * | x2 y2  1  0  0  0 -x2*u2 -y2*u2 | |c02| |u2|
 * | x3 y3  1  0  0  0 -x3*u3 -y3*u3 |.|c10|=|u3|,
 * |  0  0  0 x0 y0  1 -x0*v0 -y0*v0 | |c11| |v0|
 * |  0  0  0 x1 y1  1 -x1*v1 -y1*v1 | |c12| |v1|
 * |  0  0  0 x2 y2  1 -x2*v2 -y2*v2 | |c20| |v2|
 * \  0  0  0 x3 y3  1 -x3*v3 -y3*v3 / \c21/ \v3/
 *
 * where:
 *   cij - matrix coefficients, c22 = 1
 */
CV_IMPL CvMat*
cvGetPerspectiveTransform( const CvPoint2D32f* src,
                          const CvPoint2D32f* dst,
                          CvMat* matrix )
{
    CV_FUNCNAME( "cvGetPerspectiveTransform" );

    __BEGIN__;

    double a[8][8];
    double b[8], x[9];

    CvMat A = cvMat( 8, 8, CV_64FC1, a );
    CvMat B = cvMat( 8, 1, CV_64FC1, b );
    CvMat X = cvMat( 8, 1, CV_64FC1, x );

    int i;

    if( !src || !dst || !matrix )
        CV_ERROR( CV_StsNullPtr, "" );

    for( i = 0; i < 4; ++i )
    {
        a[i][0] = a[i+4][3] = src[i].x;
        a[i][1] = a[i+4][4] = src[i].y;
        a[i][2] = a[i+4][5] = 1;
        a[i][3] = a[i][4] = a[i][5] =
        a[i+4][0] = a[i+4][1] = a[i+4][2] = 0;
        a[i][6] = -src[i].x*dst[i].x;
        a[i][7] = -src[i].y*dst[i].x;
        a[i+4][6] = -src[i].x*dst[i].y;
        a[i+4][7] = -src[i].y*dst[i].y;
        b[i] = dst[i].x;
        b[i+4] = dst[i].y;
    }

    cvSolve( &A, &B, &X, CV_SVD );
    x[8] = 1;
    
    X = cvMat( 3, 3, CV_64FC1, x );
    cvConvert( &X, matrix );

    __END__;

    return matrix;
}

/* Calculates coefficients of affine transformation
 * which maps (xi,yi) to (ui,vi), (i=1,2,3):
 *      
 * ui = c00*xi + c01*yi + c02
 *
 * vi = c10*xi + c11*yi + c12
 *
 * Coefficients are calculated by solving linear system:
 * / x0 y0  1  0  0  0 \ /c00\ /u0\
 * | x1 y1  1  0  0  0 | |c01| |u1|
 * | x2 y2  1  0  0  0 | |c02| |u2|
 * |  0  0  0 x0 y0  1 | |c10| |v0|
 * |  0  0  0 x1 y1  1 | |c11| |v1|
 * \  0  0  0 x2 y2  1 / |c12| |v2|
 *
 * where:
 *   cij - matrix coefficients
 */
CV_IMPL CvMat*
cvGetAffineTransform( const CvPoint2D32f * src, const CvPoint2D32f * dst, CvMat * map_matrix )
{
    CV_FUNCNAME( "cvGetAffineTransform" );

    __BEGIN__;

    CvMat mA, mX, mB;
    double A[6*6];
    double B[6];
	double x[6];
    int i;

    cvInitMatHeader(&mA, 6, 6, CV_64F, A);
    cvInitMatHeader(&mB, 6, 1, CV_64F, B);
	cvInitMatHeader(&mX, 6, 1, CV_64F, x);

	if( !src || !dst || !map_matrix )
		CV_ERROR( CV_StsNullPtr, "" );

    for( i = 0; i < 3; i++ )
    {
        int j = i*12;
        int k = i*12+6;
        A[j] = A[k+3] = src[i].x;
        A[j+1] = A[k+4] = src[i].y;
        A[j+2] = A[k+5] = 1;
        A[j+3] = A[j+4] = A[j+5] = 0;
        A[k] = A[k+1] = A[k+2] = 0;
        B[i*2] = dst[i].x;
        B[i*2+1] = dst[i].y;
    }
    cvSolve(&mA, &mB, &mX);

    mX = cvMat( 2, 3, CV_64FC1, x );
	cvConvert( &mX, map_matrix );

	__END__;
    return map_matrix;
}

/****************************************************************************************\
*                          Generic Geometric Transformation: Remap                       *
\****************************************************************************************/

#define  ICV_DEF_REMAP_BILINEAR_FUNC( flavor, arrtype, load_macro, cast_macro ) \
static CvStatus CV_STDCALL                                                      \
icvRemap_Bilinear_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                         arrtype* dst, int dststep, CvSize dsize,           \
                         const float* mapx, int mxstep,                     \
                         const float* mapy, int mystep,                     \
                         int cn, const arrtype* fillval )                   \
{                                                                           \
    int i, j, k;                                                            \
    ssize.width--;                                                          \
    ssize.height--;                                                         \
                                                                            \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(dst[0]);                                              \
    mxstep /= sizeof(mapx[0]);                                              \
    mystep /= sizeof(mapy[0]);                                              \
                                                                            \
    for( i = 0; i < dsize.height; i++, dst += dststep,                      \
                                  mapx += mxstep, mapy += mystep )          \
    {                                                                       \
        for( j = 0; j < dsize.width; j++ )                                  \
        {                                                                   \
            float _x = mapx[j], _y = mapy[j];                               \
            int ix = cvFloor(_x), iy = cvFloor(_y);                         \
                                                                            \
            if( (unsigned)ix < (unsigned)ssize.width &&                     \
                (unsigned)iy < (unsigned)ssize.height )                     \
            {                                                               \
                const arrtype* s = src + iy*srcstep + ix*cn;                \
                _x -= ix; _y -= iy;                                         \
                for( k = 0; k < cn; k++, s++ )                              \
                {                                                           \
                    float t0 = load_macro(s[0]), t1 = load_macro(s[srcstep]); \
                    t0 += _x*(load_macro(s[cn]) - t0);                      \
                    t1 += _x*(load_macro(s[srcstep + cn]) - t1);            \
                    dst[j*cn + k] = (arrtype)cast_macro(t0 + _y*(t1 - t0)); \
                }                                                           \
            }                                                               \
            else if( fillval )                                              \
                for( k = 0; k < cn; k++ )                                   \
                    dst[j*cn + k] = fillval[k];                             \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define  ICV_DEF_REMAP_BICUBIC_FUNC( flavor, arrtype, worktype,                 \
                                     load_macro, cast_macro1, cast_macro2 )     \
static CvStatus CV_STDCALL                                                      \
icvRemap_Bicubic_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize, \
                         arrtype* dst, int dststep, CvSize dsize,               \
                         const float* mapx, int mxstep,                         \
                         const float* mapy, int mystep,                         \
                         int cn, const arrtype* fillval )                       \
{                                                                               \
    int i, j, k;                                                                \
    ssize.width = MAX( ssize.width - 3, 0 );                                    \
    ssize.height = MAX( ssize.height - 3, 0 );                                  \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    mxstep /= sizeof(mapx[0]);                                                  \
    mystep /= sizeof(mapy[0]);                                                  \
                                                                                \
    for( i = 0; i < dsize.height; i++, dst += dststep,                          \
                                  mapx += mxstep, mapy += mystep )              \
    {                                                                           \
        for( j = 0; j < dsize.width; j++ )                                      \
        {                                                                       \
            int ix = cvRound(mapx[j]*(1 << ICV_WARP_SHIFT));                    \
            int iy = cvRound(mapy[j]*(1 << ICV_WARP_SHIFT));                    \
            int ifx = ix & ICV_WARP_MASK;                                       \
            int ify = iy & ICV_WARP_MASK;                                       \
            ix >>= ICV_WARP_SHIFT;                                              \
            iy >>= ICV_WARP_SHIFT;                                              \
                                                                                \
            if( (unsigned)(ix-1) < (unsigned)ssize.width &&                     \
                (unsigned)(iy-1) < (unsigned)ssize.height )                     \
            {                                                                   \
                for( k = 0; k < cn; k++ )                                       \
                {                                                               \
                    const arrtype* s = src + (iy-1)*srcstep + ix*cn + k;        \
                                                                                \
                    float t0 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    s += srcstep;                                               \
                                                                                \
                    float t1 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    s += srcstep;                                               \
                                                                                \
                    float t2 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    s += srcstep;                                               \
                                                                                \
                    float t3 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    worktype t = cast_macro1( t0*icvCubicCoeffs[ify*2 + 1] +    \
                               t1*icvCubicCoeffs[ify*2] +                       \
                               t2*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ify)*2] +  \
                               t3*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ify)*2+1] );\
                                                                                \
                    dst[j*cn + k] = cast_macro2(t);                             \
                }                                                               \
            }                                                                   \
            else if( fillval )                                                  \
                for( k = 0; k < cn; k++ )                                       \
                    dst[j*cn + k] = fillval[k];                                 \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_REMAP_BILINEAR_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_REMAP_BILINEAR_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_REMAP_BILINEAR_FUNC( 32f, float, CV_NOP, CV_NOP )

ICV_DEF_REMAP_BICUBIC_FUNC( 8u, uchar, int, CV_8TO32F, cvRound, CV_FAST_CAST_8U )
ICV_DEF_REMAP_BICUBIC_FUNC( 16u, ushort, int, CV_NOP, cvRound, CV_CAST_16U )
ICV_DEF_REMAP_BICUBIC_FUNC( 32f, float, float, CV_NOP, CV_NOP, CV_NOP )

typedef CvStatus (CV_STDCALL * CvRemapFunc)(
    const void* src, int srcstep, CvSize ssize,
    void* dst, int dststep, CvSize dsize,
    const float* mapx, int mxstep,
    const float* mapy, int mystep,
    int cn, const void* fillval );

static void icvInitRemapTab( CvFuncTable* bilinear_tab, CvFuncTable* bicubic_tab )
{
    bilinear_tab->fn_2d[CV_8U] = (void*)icvRemap_Bilinear_8u_CnR;
    bilinear_tab->fn_2d[CV_16U] = (void*)icvRemap_Bilinear_16u_CnR;
    bilinear_tab->fn_2d[CV_32F] = (void*)icvRemap_Bilinear_32f_CnR;

    bicubic_tab->fn_2d[CV_8U] = (void*)icvRemap_Bicubic_8u_CnR;
    bicubic_tab->fn_2d[CV_16U] = (void*)icvRemap_Bicubic_16u_CnR;
    bicubic_tab->fn_2d[CV_32F] = (void*)icvRemap_Bicubic_32f_CnR;
}


/******************** IPP remap functions *********************/

typedef CvStatus (CV_STDCALL * CvRemapIPPFunc)(
    const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
    const float* xmap, int xmapstep, const float* ymap, int ymapstep,
    void* dst, int dststep, CvSize dstsize, int interpolation ); 

icvRemap_8u_C1R_t icvRemap_8u_C1R_p = 0;
icvRemap_8u_C3R_t icvRemap_8u_C3R_p = 0;
icvRemap_8u_C4R_t icvRemap_8u_C4R_p = 0;

icvRemap_32f_C1R_t icvRemap_32f_C1R_p = 0;
icvRemap_32f_C3R_t icvRemap_32f_C3R_p = 0;
icvRemap_32f_C4R_t icvRemap_32f_C4R_p = 0;

/**************************************************************/

#define CV_REMAP_SHIFT 5
#define CV_REMAP_MASK ((1 << CV_REMAP_SHIFT) - 1)

#if CV_SSE2 && defined(__GNUC__)
#define align(x) __attribute__ ((aligned (x)))
#elif CV_SSE2 && (defined(__ICL) || defined _MSC_VER && _MSC_VER >= 1300)
#define align(x) __declspec(align(x))
#else
#define align(x)
#endif

static void icvRemapFixedPt_8u( const CvMat* src, CvMat* dst,
    const CvMat* xymap, const CvMat* amap, const uchar* fillval )
{
    const int TABSZ = 1 << (CV_REMAP_SHIFT*2);
    static ushort align(8) atab[TABSZ][4];
    static int inittab = 0;

    int x, y, cols = src->cols, rows = src->rows;
    const uchar* sptr0 = src->data.ptr;
    int sstep = src->step;
    uchar fv0 = fillval[0], fv1 = fillval[1], fv2 = fillval[2], fv3 = fillval[3];
    int cn = CV_MAT_CN(src->type);
#if CV_SSE2
    const uchar* sptr1 = sptr0 + sstep;
    __m128i br = _mm_set1_epi32((cols-2) + ((rows-2)<<16));
    __m128i xy2ofs = _mm_set1_epi32(1 + (sstep << 16));
    __m128i z = _mm_setzero_si128();
    int align(16) iofs0[4], iofs1[4];
#endif

    if( !inittab )
    {
        for( y = 0; y <= CV_REMAP_MASK; y++ )
            for( x = 0; x <= CV_REMAP_MASK; x++ )
            {
                int k = (y << CV_REMAP_SHIFT) + x;
                atab[k][0] = (ushort)((CV_REMAP_MASK+1 - y)*(CV_REMAP_MASK+1 - x));
                atab[k][1] = (ushort)((CV_REMAP_MASK+1 - y)*x);
                atab[k][2] = (ushort)(y*(CV_REMAP_MASK+1 - x));
                atab[k][3] = (ushort)(y*x);
            }
        inittab = 1;
    }

    for( y = 0; y < rows; y++ )
    {
        const short* xy = (const short*)(xymap->data.ptr + xymap->step*y);
        const ushort* alpha = (const ushort*)(amap->data.ptr + amap->step*y);
        uchar* dptr = (uchar*)(dst->data.ptr + dst->step*y);
        int x = 0;

        if( cn == 1 )
        {
    #if CV_SSE2
            for( ; x <= cols - 8; x += 8 )
            {
                __m128i xy0 = _mm_load_si128( (const __m128i*)(xy + x*2));
                __m128i xy1 = _mm_load_si128( (const __m128i*)(xy + x*2 + 8));
                // 0|0|0|0|... <= x0|y0|x1|y1|... < cols-1|rows-1|cols-1|rows-1|... ?
                __m128i mask0 = _mm_cmpeq_epi32(_mm_or_si128(_mm_cmpgt_epi16(z, xy0),
                                                _mm_cmpgt_epi16(xy0,br)), z);
                __m128i mask1 = _mm_cmpeq_epi32(_mm_or_si128(_mm_cmpgt_epi16(z, xy1),
                                                _mm_cmpgt_epi16(xy1,br)), z);
                __m128i ofs0 = _mm_and_si128(_mm_madd_epi16( xy0, xy2ofs ), mask0 );
                __m128i ofs1 = _mm_and_si128(_mm_madd_epi16( xy1, xy2ofs ), mask1 );
                unsigned i0, i1;
                __m128i v0, v1, v2, v3, a0, a1, b0, b1;
                _mm_store_si128( (__m128i*)iofs0, ofs0 );
                _mm_store_si128( (__m128i*)iofs1, ofs1 );
                i0 = *(ushort*)(sptr0 + iofs0[0]) + (*(ushort*)(sptr0 + iofs0[1]) << 16);
                i1 = *(ushort*)(sptr0 + iofs0[2]) + (*(ushort*)(sptr0 + iofs0[3]) << 16);
                v0 = _mm_unpacklo_epi32(_mm_cvtsi32_si128(i0), _mm_cvtsi32_si128(i1));
                i0 = *(ushort*)(sptr1 + iofs0[0]) + (*(ushort*)(sptr1 + iofs0[1]) << 16);
                i1 = *(ushort*)(sptr1 + iofs0[2]) + (*(ushort*)(sptr1 + iofs0[3]) << 16);
                v1 = _mm_unpacklo_epi32(_mm_cvtsi32_si128(i0), _mm_cvtsi32_si128(i1));
                v0 = _mm_unpacklo_epi8(v0, z);
                v1 = _mm_unpacklo_epi8(v1, z);

                a0 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)atab[alpha[x]]),
                                        _mm_loadl_epi64((__m128i*)atab[alpha[x+1]]));
                a1 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)atab[alpha[x+2]]),
                                        _mm_loadl_epi64((__m128i*)atab[alpha[x+3]]));
                b0 = _mm_unpacklo_epi64(a0, a1);
                b1 = _mm_unpackhi_epi64(a0, a1);
                v0 = _mm_madd_epi16(v0, b0);
                v1 = _mm_madd_epi16(v1, b1);
                v0 = _mm_and_si128(_mm_add_epi32(v0, v1), mask0);

                i0 = *(ushort*)(sptr0 + iofs1[0]) + (*(ushort*)(sptr0 + iofs1[1]) << 16);
                i1 = *(ushort*)(sptr0 + iofs1[2]) + (*(ushort*)(sptr0 + iofs1[3]) << 16);
                v2 = _mm_unpacklo_epi32(_mm_cvtsi32_si128(i0), _mm_cvtsi32_si128(i1));
                i0 = *(ushort*)(sptr1 + iofs1[0]) + (*(ushort*)(sptr1 + iofs1[1]) << 16);
                i1 = *(ushort*)(sptr1 + iofs1[2]) + (*(ushort*)(sptr1 + iofs1[3]) << 16);
                v3 = _mm_unpacklo_epi32(_mm_cvtsi32_si128(i0), _mm_cvtsi32_si128(i1));
                v2 = _mm_unpacklo_epi8(v2, z);
                v3 = _mm_unpacklo_epi8(v3, z);

                a0 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)atab[alpha[x+4]]),
                                        _mm_loadl_epi64((__m128i*)atab[alpha[x+5]]));
                a1 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)atab[alpha[x+6]]),
                                        _mm_loadl_epi64((__m128i*)atab[alpha[x+7]]));
                b0 = _mm_unpacklo_epi64(a0, a1);
                b1 = _mm_unpackhi_epi64(a0, a1);
                v2 = _mm_madd_epi16(v2, b0);
                v3 = _mm_madd_epi16(v3, b1);
                v2 = _mm_and_si128(_mm_add_epi32(v2, v3), mask1);

                v0 = _mm_srai_epi32(v0, CV_REMAP_SHIFT*2);
                v2 = _mm_srai_epi32(v2, CV_REMAP_SHIFT*2);
                v0 = _mm_packus_epi16(_mm_packs_epi32(v0, v2), z);
                _mm_storel_epi64( (__m128i*)(dptr + x), v0 );
            }
    #endif

            for( ; x < cols; x++ )
            {
                int xi = xy[x*2], yi = xy[x*2+1];
                if( (unsigned)yi >= (unsigned)(rows - 1) ||
                    (unsigned)xi >= (unsigned)(cols - 1))
                {
                    dptr[x] = fv0;
                }
                else
                {
                    const uchar* sptr = sptr0 + sstep*yi + xi;
                    const ushort* a = atab[alpha[x]];
                    dptr[x] = (uchar)((sptr[0]*a[0] + sptr[1]*a[1] + sptr[sstep]*a[2] +
                                      sptr[sstep+1]*a[3])>>CV_REMAP_SHIFT*2);
                }
            }
        }
        else if( cn == 3 )
        {
            for( ; x < cols; x++ )
            {
                int xi = xy[x*2], yi = xy[x*2+1];
                if( (unsigned)yi >= (unsigned)(rows - 1) ||
                    (unsigned)xi >= (unsigned)(cols - 1))
                {
                    dptr[x*3] = fv0; dptr[x*3+1] = fv1; dptr[x*3+2] = fv2;
                }
                else
                {
                    const uchar* sptr = sptr0 + sstep*yi + xi*3;
                    const ushort* a = atab[alpha[x]];
                    int v0, v1, v2;
                    v0 = (sptr[0]*a[0] + sptr[3]*a[1] +
                        sptr[sstep]*a[2] + sptr[sstep+3]*a[3])>>CV_REMAP_SHIFT*2;
                    v1 = (sptr[1]*a[0] + sptr[4]*a[1] +
                        sptr[sstep+1]*a[2] + sptr[sstep+4]*a[3])>>CV_REMAP_SHIFT*2;
                    v2 = (sptr[2]*a[0] + sptr[5]*a[1] +
                        sptr[sstep+2]*a[2] + sptr[sstep+5]*a[3])>>CV_REMAP_SHIFT*2;
                    dptr[x*3] = (uchar)v0; dptr[x*3+1] = (uchar)v1; dptr[x*3+2] = (uchar)v2;
                }
            }
        }
        else
        {
            assert( cn == 4 );
            for( ; x < cols; x++ )
            {
                int xi = xy[x*2], yi = xy[x*2+1];
                if( (unsigned)yi >= (unsigned)(rows - 1) ||
                    (unsigned)xi >= (unsigned)(cols - 1))
                {
                    dptr[x*4] = fv0; dptr[x*4+1] = fv1;
                    dptr[x*4+2] = fv2; dptr[x*4+3] = fv3;
                }
                else
                {
                    const uchar* sptr = sptr0 + sstep*yi + xi*3;
                    const ushort* a = atab[alpha[x]];
                    int v0, v1;
                    v0 = (sptr[0]*a[0] + sptr[4]*a[1] +
                        sptr[sstep]*a[2] + sptr[sstep+3]*a[3])>>CV_REMAP_SHIFT*2;
                    v1 = (sptr[1]*a[0] + sptr[5]*a[1] +
                        sptr[sstep+1]*a[2] + sptr[sstep+5]*a[3])>>CV_REMAP_SHIFT*2;
                    dptr[x*4] = (uchar)v0; dptr[x*4+1] = (uchar)v1;
                    v0 = (sptr[2]*a[0] + sptr[6]*a[1] +
                        sptr[sstep+2]*a[2] + sptr[sstep+6]*a[3])>>CV_REMAP_SHIFT*2;
                    v1 = (sptr[3]*a[0] + sptr[7]*a[1] +
                        sptr[sstep+3]*a[2] + sptr[sstep+7]*a[3])>>CV_REMAP_SHIFT*2;
                    dptr[x*4+2] = (uchar)v0; dptr[x*4+3] = (uchar)v1;
                }
            }
        }
    }
}


CV_IMPL void
cvRemap( const CvArr* srcarr, CvArr* dstarr,
         const CvArr* _mapx, const CvArr* _mapy,
         int flags, CvScalar fillval )
{
    static CvFuncTable bilinear_tab;
    static CvFuncTable bicubic_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvRemap" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat mxstub, *mapx = (CvMat*)_mapx;
    CvMat mystub, *mapy = (CvMat*)_mapy;
    int type, depth, cn;
    bool fltremap;
    int method = flags & 3;
    double fillbuf[4];
    CvSize ssize, dsize;

    if( !inittab )
    {
        icvInitRemapTab( &bilinear_tab, &bicubic_tab );
        icvInitLinearCoeffTab();
        icvInitCubicCoeffTab();
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    CV_CALL( mapx = cvGetMat( mapx, &mxstub ));
    CV_CALL( mapy = cvGetMat( mapy, &mystub ));
    
    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_TYPE(mapx->type) == CV_16SC1 && CV_MAT_TYPE(mapy->type) == CV_16SC2 )
    {
        CvMat* temp;
        CV_SWAP(mapx, mapy, temp);
    }

    if( (CV_MAT_TYPE(mapx->type) != CV_32FC1 || CV_MAT_TYPE(mapy->type) != CV_32FC1) &&
        (CV_MAT_TYPE(mapx->type) != CV_16SC2 || CV_MAT_TYPE(mapy->type) != CV_16SC1))
        CV_ERROR( CV_StsUnmatchedFormats, "Either both map arrays must have 32fC1 type, "
        "or one of them must be 16sC2 and the other one must be 16sC1" );

    if( !CV_ARE_SIZES_EQ( mapx, mapy ) || !CV_ARE_SIZES_EQ( mapx, dst ))
        CV_ERROR( CV_StsUnmatchedSizes,
        "Both map arrays and the destination array must have the same size" );

    fltremap = CV_MAT_TYPE(mapx->type) == CV_32FC1;
    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( cn > 4 )
        CV_ERROR( CV_BadNumChannels, "" );
    
    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);
    
    cvScalarToRawData( &fillval, fillbuf, CV_MAT_TYPE(src->type), 0 );

    if( !fltremap )
    {
        if( CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_8UC3 &&
            CV_MAT_TYPE(src->type) != CV_8UC4 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Only 8-bit input/output is supported by the fixed-point variant of cvRemap" );
        icvRemapFixedPt_8u( src, dst, mapx, mapy, (uchar*)fillbuf );
        EXIT;
    }

    if( icvRemap_8u_C1R_p )
    {
        CvRemapIPPFunc ipp_func =
            type == CV_8UC1 ? icvRemap_8u_C1R_p :
            type == CV_8UC3 ? icvRemap_8u_C3R_p :
            type == CV_8UC4 ? icvRemap_8u_C4R_p :
            type == CV_32FC1 ? icvRemap_32f_C1R_p :
            type == CV_32FC3 ? icvRemap_32f_C3R_p :
            type == CV_32FC4 ? icvRemap_32f_C4R_p : 0;
        
        if( ipp_func )
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            int mxstep = mapx->step ? mapx->step : CV_STUB_STEP;
            int mystep = mapy->step ? mapy->step : CV_STUB_STEP;
            CvStatus status;
            CvRect srcroi = {0, 0, ssize.width, ssize.height};

            // this is not the most efficient way to fill outliers
            if( flags & CV_WARP_FILL_OUTLIERS )
                cvSet( dst, fillval );

            status = ipp_func( src->data.ptr, ssize, srcstep, srcroi,
                               mapx->data.fl, mxstep, mapy->data.fl, mystep,
                               dst->data.ptr, dststep, dsize,
                               1 << (method == CV_INTER_NN || method == CV_INTER_LINEAR ||
                               method == CV_INTER_CUBIC ? method : CV_INTER_LINEAR) );
            if( status >= 0 )
                EXIT;
        }
    }

    {
        CvRemapFunc func = method == CV_INTER_CUBIC ?
            (CvRemapFunc)bicubic_tab.fn_2d[depth] :
            (CvRemapFunc)bilinear_tab.fn_2d[depth];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        func( src->data.ptr, src->step, ssize, dst->data.ptr, dst->step, dsize,
              mapx->data.fl, mapx->step, mapy->data.fl, mapy->step,
              cn, flags & CV_WARP_FILL_OUTLIERS ? fillbuf : 0 );
    }

    __END__;
}

CV_IMPL void
cvConvertMaps( const CvArr* arrx, const CvArr* arry,
               CvArr* arrxy, CvArr* arra )
{
    CV_FUNCNAME( "cvConvertMaps" );

    __BEGIN__;

    CvMat xstub, *mapx = cvGetMat( arrx, &xstub );
    CvMat ystub, *mapy = cvGetMat( arry, &ystub );
    CvMat xystub, *mapxy = cvGetMat( arrxy, &xystub );
    CvMat astub, *mapa = cvGetMat( arra, &astub );
    int x, y, cols = mapx->cols, rows = mapx->rows;

    CV_ASSERT( CV_ARE_SIZES_EQ(mapx, mapy) && CV_ARE_TYPES_EQ(mapx, mapy) &&
        CV_MAT_TYPE(mapx->type) == CV_32FC1 &&
        CV_ARE_SIZES_EQ(mapxy, mapx) && CV_ARE_SIZES_EQ(mapxy, mapa) &&
        CV_MAT_TYPE(mapxy->type) == CV_16SC2 &&
        CV_MAT_TYPE(mapa->type) == CV_16SC1 );

    for( y = 0; y < rows; y++ )
    {
        const float* xrow = (const float*)(mapx->data.ptr + mapx->step*y);
        const float* yrow = (const float*)(mapy->data.ptr + mapy->step*y);
        short* xy = (short*)(mapxy->data.ptr + mapxy->step*y);
        short* alpha = (short*)(mapa->data.ptr + mapa->step*y);

        for( x = 0; x < cols; x++ )
        {
            int xi = cvRound(xrow[x]*(1 << CV_REMAP_SHIFT));
            int yi = cvRound(yrow[x]*(1 << CV_REMAP_SHIFT));
            xy[x*2] = (short)(xi >> CV_REMAP_SHIFT);
            xy[x*2+1] = (short)(yi >> CV_REMAP_SHIFT);
            alpha[x] = (short)((xi & CV_REMAP_MASK) + ((yi & CV_REMAP_MASK)<<CV_REMAP_SHIFT));
        }
    }

    __END__;
}


/****************************************************************************************\
*                                   Log-Polar Transform                                  *
\****************************************************************************************/

/* now it is done via Remap; more correct implementation should use
   some super-sampling technique outside of the "fovea" circle */
CV_IMPL void
cvLogPolar( const CvArr* srcarr, CvArr* dstarr,
            CvPoint2D32f center, double M, int flags )
{
    CvMat* mapx = 0;
    CvMat* mapy = 0;
    double* exp_tab = 0;
    float* buf = 0;
    
    CV_FUNCNAME( "cvLogPolar" );

    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize ssize, dsize;

    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( M <= 0 )
        CV_ERROR( CV_StsOutOfRange, "M should be >0" );

    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);

    CV_CALL( mapx = cvCreateMat( dsize.height, dsize.width, CV_32F ));
    CV_CALL( mapy = cvCreateMat( dsize.height, dsize.width, CV_32F ));

    if( !(flags & CV_WARP_INVERSE_MAP) )
    {
        int phi, rho;
        
        CV_CALL( exp_tab = (double*)cvAlloc( dsize.width*sizeof(exp_tab[0])) );

        for( rho = 0; rho < dst->width; rho++ )
            exp_tab[rho] = exp(rho/M);
    
        for( phi = 0; phi < dsize.height; phi++ )
        {
            double cp = cos(phi*2*CV_PI/dsize.height);
            double sp = sin(phi*2*CV_PI/dsize.height);
            float* mx = (float*)(mapx->data.ptr + phi*mapx->step);
            float* my = (float*)(mapy->data.ptr + phi*mapy->step);

            for( rho = 0; rho < dsize.width; rho++ )
            {
                double r = exp_tab[rho];
                double x = r*cp + center.x;
                double y = r*sp + center.y;

                mx[rho] = (float)x;
                my[rho] = (float)y;
            }
        }
    }
    else
    {
        int x, y;
        CvMat bufx, bufy, bufp, bufa;
        double ascale = (ssize.width-1)/(2*CV_PI);

        CV_CALL( buf = (float*)cvAlloc( 4*dsize.width*sizeof(buf[0]) ));

        bufx = cvMat( 1, dsize.width, CV_32F, buf );
        bufy = cvMat( 1, dsize.width, CV_32F, buf + dsize.width );
        bufp = cvMat( 1, dsize.width, CV_32F, buf + dsize.width*2 );
        bufa = cvMat( 1, dsize.width, CV_32F, buf + dsize.width*3 );

        for( x = 0; x < dsize.width; x++ )
            bufx.data.fl[x] = (float)x - center.x;

        for( y = 0; y < dsize.height; y++ )
        {
            float* mx = (float*)(mapx->data.ptr + y*mapx->step);
            float* my = (float*)(mapy->data.ptr + y*mapy->step);
            
            for( x = 0; x < dsize.width; x++ )
                bufy.data.fl[x] = (float)y - center.y;

#if 1
            cvCartToPolar( &bufx, &bufy, &bufp, &bufa );

            for( x = 0; x < dsize.width; x++ )
                bufp.data.fl[x] += 1.f;

            cvLog( &bufp, &bufp );
            
            for( x = 0; x < dsize.width; x++ )
            {
                double rho = bufp.data.fl[x]*M;
                double phi = bufa.data.fl[x]*ascale;

                mx[x] = (float)rho;
                my[x] = (float)phi;
            }
#else
            for( x = 0; x < dsize.width; x++ )
            {
                double xx = bufx.data.fl[x];
                double yy = bufy.data.fl[x];

                double p = log(sqrt(xx*xx + yy*yy) + 1.)*M;
                double a = atan2(yy,xx);
                if( a < 0 )
                    a = 2*CV_PI + a;
                a *= ascale;

                mx[x] = (float)p;
                my[x] = (float)a;
            }
#endif
        }
    }

    cvRemap( src, dst, mapx, mapy, flags, cvScalarAll(0) );

    __END__;

    cvFree( &exp_tab );
    cvFree( &buf );
    cvReleaseMat( &mapx );
    cvReleaseMat( &mapy );
}

/* End of file. */
