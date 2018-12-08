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
//  Filling CvMat/IplImage instances with random numbers
//
// */

#include "_cxcore.h"


///////////////////////////// Functions Declaration //////////////////////////////////////

/*
   Multiply-with-carry generator is used here:
   temp = ( A*X(n) + carry )
   X(n+1) = temp mod (2^32)
   carry = temp / (2^32)
*/
#define  ICV_RNG_NEXT(x)    ((uint64)(unsigned)(x)*1554115554 + ((x) >> 32))
#define  ICV_CVT_FLT(x)     (((unsigned)(x) >> 9)|CV_1F)
#define  ICV_1D             CV_BIG_INT(0x3FF0000000000000)
#define  ICV_CVT_DBL(x)     (((uint64)(unsigned)(x) << 20)|((x) >> 44)|ICV_1D)

/***************************************************************************************\
*                           Pseudo-Random Number Generators (PRNGs)                     *
\***************************************************************************************/

#define ICV_IMPL_RAND_BITS( flavor, arrtype, cast_macro )               \
static CvStatus CV_STDCALL                                              \
icvRandBits_##flavor##_C1R( arrtype* arr, int step, CvSize size,        \
                            uint64* state, const int* param )           \
{                                                                       \
    uint64 temp = *state;                                               \
    int small_flag = (param[12]|param[13]|param[14]|param[15]) <= 255;  \
    step /= sizeof(arr[0]);                                             \
                                                                        \
    for( ; size.height--; arr += step )                                 \
    {                                                                   \
        int i, k = 3;                                                   \
        const int* p = param;                                           \
                                                                        \
        if( !small_flag )                                               \
        {                                                               \
            for( i = 0; i <= size.width - 4; i += 4 )                   \
            {                                                           \
                unsigned t0, t1;                                        \
                                                                        \
                temp = ICV_RNG_NEXT(temp);                              \
                t0 = ((unsigned)temp & p[i + 12]) + p[i];               \
                temp = ICV_RNG_NEXT(temp);                              \
                t1 = ((unsigned)temp & p[i + 13]) + p[i+1];             \
                arr[i] = cast_macro((int)t0);                           \
                arr[i+1] = cast_macro((int)t1);                         \
                                                                        \
                temp = ICV_RNG_NEXT(temp);                              \
                t0 = ((unsigned)temp & p[i + 14]) + p[i+2];             \
                temp = ICV_RNG_NEXT(temp);                              \
                t1 = ((unsigned)temp & p[i + 15]) + p[i+3];             \
                arr[i+2] = cast_macro((int)t0);                         \
                arr[i+3] = cast_macro((int)t1);                         \
                                                                        \
                if( !--k )                                              \
                {                                                       \
                    k = 3;                                              \
                    p -= 12;                                            \
                }                                                       \
            }                                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            for( i = 0; i <= size.width - 4; i += 4 )                   \
            {                                                           \
                unsigned t0, t1, t;                                     \
                                                                        \
                temp = ICV_RNG_NEXT(temp);                              \
                t = (unsigned)temp;                                     \
                t0 = (t & p[i + 12]) + p[i];                            \
                t1 = ((t >> 8) & p[i + 13]) + p[i+1];                   \
                arr[i] = cast_macro((int)t0);                           \
                arr[i+1] = cast_macro((int)t1);                         \
                                                                        \
                t0 = ((t >> 16) & p[i + 14]) + p[i + 2];                \
                t1 = ((t >> 24) & p[i + 15]) + p[i + 3];                \
                arr[i+2] = cast_macro((int)t0);                         \
                arr[i+3] = cast_macro((int)t1);                         \
                                                                        \
                if( !--k )                                              \
                {                                                       \
                    k = 3;                                              \
                    p -= 12;                                            \
                }                                                       \
            }                                                           \
        }                                                               \
                                                                        \
        for( ; i < size.width; i++ )                                    \
        {                                                               \
            unsigned t0;                                                \
            temp = ICV_RNG_NEXT(temp);                                  \
                                                                        \
            t0 = ((unsigned)temp & p[i + 12]) + p[i];                   \
            arr[i] = cast_macro((int)t0);                               \
        }                                                               \
    }                                                                   \
                                                                        \
    *state = temp;                                                      \
    return CV_OK;                                                       \
}


#define ICV_IMPL_RAND( flavor, arrtype, worktype, cast_macro1, cast_macro2 )\
static CvStatus CV_STDCALL                                              \
icvRand_##flavor##_C1R( arrtype* arr, int step, CvSize size,            \
                        uint64* state, const double* param )            \
{                                                                       \
    uint64 temp = *state;                                               \
    step /= sizeof(arr[0]);                                             \
                                                                        \
    for( ; size.height--; arr += step )                                 \
    {                                                                   \
        int i, k = 3;                                                   \
        const double* p = param;                                        \
                                                                        \
        for( i = 0; i <= size.width - 4; i += 4 )                       \
        {                                                               \
            worktype f0, f1;                                            \
            Cv32suf t0, t1;                                             \
                                                                        \
            temp = ICV_RNG_NEXT(temp);                                  \
            t0.u = ICV_CVT_FLT(temp);                                   \
            temp = ICV_RNG_NEXT(temp);                                  \
            t1.u = ICV_CVT_FLT(temp);                                   \
            f0 = cast_macro1( t0.f * p[i + 12] + p[i] );                \
            f1 = cast_macro1( t1.f * p[i + 13] + p[i + 1] );            \
            arr[i] = cast_macro2(f0);                                   \
            arr[i+1] = cast_macro2(f1);                                 \
                                                                        \
            temp = ICV_RNG_NEXT(temp);                                  \
            t0.u = ICV_CVT_FLT(temp);                                   \
            temp = ICV_RNG_NEXT(temp);                                  \
            t1.u = ICV_CVT_FLT(temp);                                   \
            f0 = cast_macro1( t0.f * p[i + 14] + p[i + 2] );            \
            f1 = cast_macro1( t1.f * p[i + 15] + p[i + 3] );            \
            arr[i+2] = cast_macro2(f0);                                 \
            arr[i+3] = cast_macro2(f1);                                 \
                                                                        \
            if( !--k )                                                  \
            {                                                           \
                k = 3;                                                  \
                p -= 12;                                                \
            }                                                           \
        }                                                               \
                                                                        \
        for( ; i < size.width; i++ )                                    \
        {                                                               \
            worktype f0;                                                \
            Cv32suf t0;                                                 \
                                                                        \
            temp = ICV_RNG_NEXT(temp);                                  \
            t0.u = ICV_CVT_FLT(temp);                                   \
            f0 = cast_macro1( t0.f * p[i + 12] + p[i] );                \
            arr[i] = cast_macro2(f0);                                   \
        }                                                               \
    }                                                                   \
                                                                        \
    *state = temp;                                                      \
    return CV_OK;                                                       \
}


static CvStatus CV_STDCALL
icvRand_64f_C1R( double* arr, int step, CvSize size,
                 uint64* state, const double* param )
{
    uint64 temp = *state;
    step /= sizeof(arr[0]);

    for( ; size.height--; arr += step )
    {
        int i, k = 3;
        const double* p = param;

        for( i = 0; i <= size.width - 4; i += 4 )
        {
            double f0, f1;
            Cv64suf t0, t1;

            temp = ICV_RNG_NEXT(temp);
            t0.u = ICV_CVT_DBL(temp);
            temp = ICV_RNG_NEXT(temp);
            t1.u = ICV_CVT_DBL(temp);
            f0 = t0.f * p[i + 12] + p[i];
            f1 = t1.f * p[i + 13] + p[i + 1];
            arr[i] = f0;
            arr[i+1] = f1;

            temp = ICV_RNG_NEXT(temp);
            t0.u = ICV_CVT_DBL(temp);
            temp = ICV_RNG_NEXT(temp);
            t1.u = ICV_CVT_DBL(temp);
            f0 = t0.f * p[i + 14] + p[i + 2];
            f1 = t1.f * p[i + 15] + p[i + 3];
            arr[i+2] = f0;
            arr[i+3] = f1;

            if( !--k )
            {
                k = 3;
                p -= 12;
            }
        }

        for( ; i < size.width; i++ )
        {
            double f0;
            Cv64suf t0;

            temp = ICV_RNG_NEXT(temp);
            t0.u = ICV_CVT_DBL(temp);
            f0 = t0.f * p[i + 12] + p[i];
            arr[i] = f0;
        }
    }

    *state = temp;
    return CV_OK;
}


/***************************************************************************************\
    The code below implements algorithm from the paper:

    G. Marsaglia and W.W. Tsang,
    The Monty Python method for generating random variables,
    ACM Transactions on Mathematical Software, Vol. 24, No. 3,
    Pages 341-350, September, 1998.
\***************************************************************************************/

static CvStatus CV_STDCALL
icvRandn_0_1_32f_C1R( float* arr, int len, uint64* state )
{
    uint64 temp = *state;
    int i;
    temp = ICV_RNG_NEXT(temp);

    for( i = 0; i < len; i++ )
    {
        double x, y, v, ax, bx;

        for(;;)
        {
            x = ((int)temp)*1.167239e-9;
            temp = ICV_RNG_NEXT(temp);
            ax = fabs(x);
            v = 2.8658 - ax*(2.0213 - 0.3605*ax);
            y = ((unsigned)temp)*2.328306e-10;
            temp = ICV_RNG_NEXT(temp);

            if( y < v || ax < 1.17741 )
                break;

            bx = x;
            x = bx > 0 ? 0.8857913*(2.506628 - ax) : -0.8857913*(2.506628 - ax);
            
            if( y > v + 0.0506 )
                break;

            if( log(y) < .6931472 - .5*bx*bx )
            {
                x = bx;
                break;
            }

            if( log(1.8857913 - y) < .5718733-.5*x*x )
                break;

            do
            {
                v = ((int)temp)*4.656613e-10;
                x = -log(fabs(v))*.3989423;
                temp = ICV_RNG_NEXT(temp);
                y = -log(((unsigned)temp)*2.328306e-10);
                temp = ICV_RNG_NEXT(temp);
            }
            while( y+y < x*x );

            x = v > 0 ? 2.506628 + x : -2.506628 - x;
            break;
        }

        arr[i] = (float)x;
    }
    *state = temp;
    return CV_OK;
}


#define RAND_BUF_SIZE  96


#define ICV_IMPL_RANDN( flavor, arrtype, worktype, cast_macro1, cast_macro2 )   \
static CvStatus CV_STDCALL                                                      \
icvRandn_##flavor##_C1R( arrtype* arr, int step, CvSize size,                   \
                         uint64* state, const double* param )                   \
{                                                                               \
    float buffer[RAND_BUF_SIZE];                                                \
    step /= sizeof(arr[0]);                                                     \
                                                                                \
    for( ; size.height--; arr += step )                                         \
    {                                                                           \
        int i, j, len = RAND_BUF_SIZE;                                          \
                                                                                \
        for( i = 0; i < size.width; i += RAND_BUF_SIZE )                        \
        {                                                                       \
            int k = 3;                                                          \
            const double* p = param;                                            \
                                                                                \
            if( i + len > size.width )                                          \
                len = size.width - i;                                           \
                                                                                \
            icvRandn_0_1_32f_C1R( buffer, len, state );                         \
                                                                                \
            for( j = 0; j <= len - 4; j += 4 )                                  \
            {                                                                   \
                worktype f0, f1;                                                \
                                                                                \
                f0 = cast_macro1( buffer[j]*p[j+12] + p[j] );                   \
                f1 = cast_macro1( buffer[j+1]*p[j+13] + p[j+1] );               \
                arr[i+j] = cast_macro2(f0);                                     \
                arr[i+j+1] = cast_macro2(f1);                                   \
                                                                                \
                f0 = cast_macro1( buffer[j+2]*p[j+14] + p[j+2] );               \
                f1 = cast_macro1( buffer[j+3]*p[j+15] + p[j+3] );               \
                arr[i+j+2] = cast_macro2(f0);                                   \
                arr[i+j+3] = cast_macro2(f1);                                   \
                                                                                \
                if( --k == 0 )                                                  \
                {                                                               \
                    k = 3;                                                      \
                    p -= 12;                                                    \
                }                                                               \
            }                                                                   \
                                                                                \
            for( ; j < len; j++ )                                               \
            {                                                                   \
                worktype f0 = cast_macro1( buffer[j]*p[j+12] + p[j] );          \
                arr[i+j] = cast_macro2(f0);                                     \
            }                                                                   \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_IMPL_RAND_BITS( 8u, uchar, CV_CAST_8U )
ICV_IMPL_RAND_BITS( 16u, ushort, CV_CAST_16U )
ICV_IMPL_RAND_BITS( 16s, short, CV_CAST_16S )
ICV_IMPL_RAND_BITS( 32s, int, CV_CAST_32S )

ICV_IMPL_RAND( 8u, uchar, int, cvFloor, CV_CAST_8U )
ICV_IMPL_RAND( 16u, ushort, int, cvFloor, CV_CAST_16U )
ICV_IMPL_RAND( 16s, short, int, cvFloor, CV_CAST_16S )
ICV_IMPL_RAND( 32s, int, int, cvFloor, CV_CAST_32S )
ICV_IMPL_RAND( 32f, float, float, CV_CAST_32F, CV_NOP )

ICV_IMPL_RANDN( 8u, uchar, int, cvRound, CV_CAST_8U )
ICV_IMPL_RANDN( 16u, ushort, int, cvRound, CV_CAST_16U )
ICV_IMPL_RANDN( 16s, short, int, cvRound, CV_CAST_16S )
ICV_IMPL_RANDN( 32s, int, int, cvRound, CV_CAST_32S )
ICV_IMPL_RANDN( 32f, float, float, CV_CAST_32F, CV_NOP )
ICV_IMPL_RANDN( 64f, double, double, CV_CAST_64F, CV_NOP )

static void icvInitRandTable( CvFuncTable* fastrng_tab,
                              CvFuncTable* rng_tab,
                              CvFuncTable* normal_tab )
{
    fastrng_tab->fn_2d[CV_8U] = (void*)icvRandBits_8u_C1R;
    fastrng_tab->fn_2d[CV_8S] = 0;
    fastrng_tab->fn_2d[CV_16U] = (void*)icvRandBits_16u_C1R;
    fastrng_tab->fn_2d[CV_16S] = (void*)icvRandBits_16s_C1R;
    fastrng_tab->fn_2d[CV_32S] = (void*)icvRandBits_32s_C1R;

    rng_tab->fn_2d[CV_8U] = (void*)icvRand_8u_C1R;
    rng_tab->fn_2d[CV_8S] = 0;
    rng_tab->fn_2d[CV_16U] = (void*)icvRand_16u_C1R;
    rng_tab->fn_2d[CV_16S] = (void*)icvRand_16s_C1R;
    rng_tab->fn_2d[CV_32S] = (void*)icvRand_32s_C1R;
    rng_tab->fn_2d[CV_32F] = (void*)icvRand_32f_C1R;
    rng_tab->fn_2d[CV_64F] = (void*)icvRand_64f_C1R;

    normal_tab->fn_2d[CV_8U] = (void*)icvRandn_8u_C1R;
    normal_tab->fn_2d[CV_8S] = 0;
    normal_tab->fn_2d[CV_16U] = (void*)icvRandn_16u_C1R;
    normal_tab->fn_2d[CV_16S] = (void*)icvRandn_16s_C1R;
    normal_tab->fn_2d[CV_32S] = (void*)icvRandn_32s_C1R;
    normal_tab->fn_2d[CV_32F] = (void*)icvRandn_32f_C1R;
    normal_tab->fn_2d[CV_64F] = (void*)icvRandn_64f_C1R;
}


CV_IMPL void
cvRandArr( CvRNG* rng, CvArr* arr, int disttype, CvScalar param1, CvScalar param2 )
{
    static CvFuncTable rng_tab[2], fastrng_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvRandArr" );

    __BEGIN__;

    int is_nd = 0;
    CvMat stub, *mat = (CvMat*)arr;
    int type, depth, channels;
    double dparam[2][12];
    int iparam[2][12];
    void* param = dparam;
    int i, fast_int_mode = 0;
    int mat_step = 0;
    CvSize size;
    CvFunc2D_1A2P func = 0;
    CvMatND stub_nd;
    CvNArrayIterator iterator_state, *iterator = 0;

    if( !inittab )
    {
        icvInitRandTable( &fastrng_tab, &rng_tab[CV_RAND_UNI],
                          &rng_tab[CV_RAND_NORMAL] );
        inittab = 1;
    }

    if( !rng )
        CV_ERROR( CV_StsNullPtr, "Null pointer to RNG state" );

    if( CV_IS_MATND(mat) )
    {
        iterator = &iterator_state;
        CV_CALL( cvInitNArrayIterator( 1, &arr, 0, &stub_nd, iterator ));
        type = CV_MAT_TYPE(iterator->hdr[0]->type);
        size = iterator->size;
        is_nd = 1;
    }
    else
    {
        if( !CV_IS_MAT(mat))
        {
            int coi = 0;
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "COI is not supported" );
        }

        type = CV_MAT_TYPE( mat->type );
        size = cvGetMatSize( mat );
        mat_step = mat->step;

        if( mat->height > 1 && CV_IS_MAT_CONT( mat->type ))
        {
            size.width *= size.height;
            mat_step = CV_STUB_STEP;
            size.height = 1;
        }
    }

    depth = CV_MAT_DEPTH( type );
    channels = CV_MAT_CN( type );
    size.width *= channels;

    if( disttype == CV_RAND_UNI )
    {
        if( depth <= CV_32S )
        {
            for( i = 0, fast_int_mode = 1; i < channels; i++ )
            {
                int t0 = iparam[0][i] = cvCeil( param1.val[i] );
                int t1 = iparam[1][i] = cvFloor( param2.val[i] ) - t0;
                double diff = param1.val[i] - param2.val[i];

                fast_int_mode &= INT_MIN <= diff && diff <= INT_MAX && (t1 & (t1 - 1)) == 0;
            }
        }

        if( fast_int_mode )
        {
            for( i = 0; i < channels; i++ )
                iparam[1][i]--;
        
            for( ; i < 12; i++ )
            {
                int t0 = iparam[0][i - channels];
                int t1 = iparam[1][i - channels];

                iparam[0][i] = t0;
                iparam[1][i] = t1;
            }

            CV_GET_FUNC_PTR( func, (CvFunc2D_1A2P)(fastrng_tab.fn_2d[depth]));
            param = iparam;
        }
        else
        {
            for( i = 0; i < channels; i++ )
            {
                double t0 = param1.val[i];
                double t1 = param2.val[i];

                dparam[0][i] = t0 - (t1 - t0);
                dparam[1][i] = t1 - t0;
            }

            CV_GET_FUNC_PTR( func, (CvFunc2D_1A2P)(rng_tab[0].fn_2d[depth]));
        }
    }
    else if( disttype == CV_RAND_NORMAL )
    {
        for( i = 0; i < channels; i++ )
        {
            double t0 = param1.val[i];
            double t1 = param2.val[i];

            dparam[0][i] = t0;
            dparam[1][i] = t1;
        }

        CV_GET_FUNC_PTR( func, (CvFunc2D_1A2P)(rng_tab[1].fn_2d[depth]));
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "Unknown distribution type" );
    }

    if( !fast_int_mode )
    {
        for( i = channels; i < 12; i++ )
        {
            double t0 = dparam[0][i - channels];
            double t1 = dparam[1][i - channels];

            dparam[0][i] = t0;
            dparam[1][i] = t1;
        }
    }

    if( !is_nd )
    {
        IPPI_CALL( func( mat->data.ptr, mat_step, size, rng, param ));
    }
    else
    {
        do
        {
            IPPI_CALL( func( iterator->ptr[0], CV_STUB_STEP, size, rng, param ));
        }
        while( cvNextNArraySlice( iterator ));
    }

    __END__;
}

/* End of file. */
