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

/****************************************************************************************\
*                                         N o r m                                        *
\****************************************************************************************/

#define ICV_NORM_CASE( _op_,                \
    _update_op_, worktype, len )            \
                                            \
    for( ; x <= (len) - 4; x += 4 )         \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x+1];           \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
                                            \
        t0 = (src)[x+2];                    \
        t1 = (src)[x+3];                    \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
    }                                       \
                                            \
    for( ; x < (len); x++ )                 \
    {                                       \
        worktype t0 = (src)[x];             \
        t0 = (worktype)_op_(t0);            \
        norm = _update_op_( norm, t0 );     \
    }


#define ICV_NORM_COI_CASE( _op_,            \
    _update_op_, worktype, len, cn )        \
                                            \
    for( ; x < (len); x++ )                 \
    {                                       \
        worktype t0 = (src)[x*(cn)];        \
        t0 = (worktype)_op_(t0);            \
        norm = _update_op_( norm, t0 );     \
    }


#define ICV_NORM_DIFF_CASE( _op_,           \
    _update_op_, worktype, len )            \
                                            \
    for( ; x <= (len) - 4; x += 4 )         \
    {                                       \
        worktype t0 = (src1)[x] - (src2)[x];\
        worktype t1 = (src1)[x+1]-(src2)[x+1];\
                                            \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
                                            \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
                                            \
        t0 = (src1)[x+2] - (src2)[x+2];     \
        t1 = (src1)[x+3] - (src2)[x+3];     \
                                            \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
                                            \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
    }                                       \
                                            \
    for( ; x < (len); x++ )                 \
    {                                       \
        worktype t0 = (src1)[x] - (src2)[x];\
        t0 = (worktype)_op_(t0);            \
        norm = _update_op_( norm, t0 );     \
    }


#define ICV_NORM_DIFF_COI_CASE( _op_, _update_op_, worktype, len, cn ) \
    for( ; x < (len); x++ )                                     \
    {                                                           \
        worktype t0 = (src1)[x*(cn)] - (src2)[x*(cn)];          \
        t0 = (worktype)_op_(t0);                                \
        norm = _update_op_( norm, t0 );                         \
    }


/*
 	The algorithm and its multiple variations below
    below accumulates the norm by blocks of size "block_size".
    Each block may span across multiple lines and it is
    not necessary aligned by row boundaries. Within a block
    the norm is accumulated to intermediate light-weight
    type (worktype). It really makes sense for 8u, 16s, 16u types
    and L1 & L2 norms, where worktype==int and normtype==int64.
    In other cases a simpler algorithm is used
*/
#define  ICV_DEF_NORM_NOHINT_BLOCK_FUNC_2D( name, _op_, _update_op_, \
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    CvSize size, double* _norm ), (src, step, size, _norm) )    \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_CASE( _op_, _update_op_, worktype, limit );\
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_NOHINT_FUNC_2D( name, _op_, _update_op_,  \
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    CvSize size, double* _norm ), (src, step, size, _norm) )    \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_CASE(_op_, _update_op_, worktype, size.width); \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


/*
   In IPP only 32f flavors of norm functions are with hint.
   For float worktype==normtype==double, thus the block algorithm,
   described above, is not necessary.
 */
#define  ICV_DEF_NORM_HINT_FUNC_2D( name, _op_, _update_op_,    \
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    CvSize size, double* _norm, CvHintAlgorithm /*hint*/ ),     \
    (src, step, size, _norm, cvAlgHintAccurate) )               \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_CASE(_op_, _update_op_, worktype, size.width); \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_NOHINT_BLOCK_FUNC_2D_COI( name, _op_,     \
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
                CvSize size, int cn, int coi, double* _norm )   \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_COI_CASE( _op_, _update_op_,               \
                               worktype, limit, cn );           \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_NOHINT_FUNC_2D_COI( name, _op_,           \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
                CvSize size, int cn, int coi, double* _norm )   \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_COI_CASE( _op_, _update_op_,                   \
                           worktype, size.width, cn );          \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_NOHINT_BLOCK_FUNC_2D( name, _op_,    \
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, CvSize size, double* _norm),\
   (src1, step1, src2, step2, size, _norm))                     \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_CASE( _op_, _update_op_,              \
                                worktype, limit );              \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D( name, _op_,          \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, CvSize size, double* _norm),\
    ( src1, step1, src2, step2, size, _norm ))                  \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_CASE( _op_, _update_op_,                  \
                            worktype, size.width );             \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_HINT_FUNC_2D( name, _op_,            \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, CvSize size, double* _norm, \
    CvHintAlgorithm /*hint*/ ),                                 \
    (src1, step1, src2, step2, size, _norm, cvAlgHintAccurate ))\
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_CASE( _op_, _update_op_,                  \
                            worktype, size.width );             \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_NOHINT_BLOCK_FUNC_2D_COI( name, _op_,\
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, CvSize size,                \
    int cn, int coi, double* _norm )                            \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_COI_CASE( _op_, _update_op_,          \
                                    worktype, limit, cn );      \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D_COI( name, _op_,      \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, CvSize size,                \
    int cn, int coi, double* _norm )                            \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_COI_CASE( _op_, _update_op_,              \
                                worktype, size.width, cn );     \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


/****************************************************************************************\
*                             N o r m   with    M A S K                                  *
\****************************************************************************************/

#define ICV_NORM_MASK_CASE( _op_,               \
        _update_op_, worktype, len )            \
{                                               \
    for( ; x <= (len) - 2; x += 2 )             \
    {                                           \
        worktype t0;                            \
        if( mask[x] )                           \
        {                                       \
            t0 = (src)[x];                      \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
        if( mask[x+1] )                         \
        {                                       \
            t0 = (src)[x+1];                    \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
    }                                           \
                                                \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src)[x];             \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define ICV_NORM_DIFF_MASK_CASE( _op_, _update_op_, worktype, len ) \
{                                               \
    for( ; x <= (len) - 2; x += 2 )             \
    {                                           \
        worktype t0;                            \
        if( mask[x] )                           \
        {                                       \
            t0 = (src1)[x] - (src2)[x];         \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
        if( mask[x+1] )                         \
        {                                       \
            t0 = (src1)[x+1] - (src2)[x+1];     \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
    }                                           \
                                                \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src1)[x] - (src2)[x];\
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define ICV_NORM_MASK_COI_CASE( _op_, _update_op_, worktype, len, cn ) \
{                                               \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src)[x*(cn)];        \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define ICV_NORM_DIFF_MASK_COI_CASE( _op_, _update_op_, worktype, len, cn )\
{                                               \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src1)[x*(cn)] - (src2)[x*(cn)];  \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define  ICV_DEF_NORM_MASK_NOHINT_BLOCK_FUNC_2D( name, _op_,    \
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    const uchar* mask, int maskstep, CvSize size, double* _norm ),\
    (src, step, mask, maskstep, size, _norm) )                  \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_MASK_CASE( _op_, _update_op_,              \
                                worktype, limit );              \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_MASK_NOHINT_FUNC_2D( name, _op_, _update_op_,\
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    const uchar* mask, int maskstep, CvSize size, double* _norm ),\
    (src, step, mask, maskstep, size, _norm) )                  \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_MASK_CASE( _op_, _update_op_,                  \
                            worktype, size.width );             \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_MASK_NOHINT_BLOCK_FUNC_2D_COI( name, _op_,\
                _update_op_, post_func, arrtype,                \
                normtype, worktype, block_size )                \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
    const uchar* mask, int maskstep, CvSize size,               \
    int cn, int coi, double* _norm )                            \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_MASK_COI_CASE( _op_, _update_op_,          \
                                    worktype, limit, cn );      \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_MASK_NOHINT_FUNC_2D_COI( name, _op_,      \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
    const uchar* mask, int maskstep, CvSize size,               \
    int cn, int coi, double* _norm )                            \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_MASK_COI_CASE( _op_, _update_op_,              \
                                worktype, size.width, cn );     \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}



#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_BLOCK_FUNC_2D( name,     \
    _op_, _update_op_, post_func, arrtype,                      \
    normtype, worktype, block_size )                            \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, double* _norm ),                 \
    (src1, step1, src2, step2, mask, maskstep, size, _norm ))   \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_MASK_CASE( _op_, _update_op_,         \
                                     worktype, limit );         \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D( name, _op_,     \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, double* _norm ),                 \
    (src1, step1, src2, step2, mask, maskstep, size, _norm ))   \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_MASK_CASE( _op_, _update_op_,             \
                                 worktype, size.width );        \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_BLOCK_FUNC_2D_COI( name, \
    _op_, _update_op_, post_func, arrtype,                      \
    normtype, worktype, block_size )                            \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, int cn, int coi, double* _norm ) \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_MASK_COI_CASE( _op_, _update_op_,     \
                                    worktype, limit, cn );      \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D_COI( name, _op_, \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, int cn, int coi, double* _norm ) \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_MASK_COI_CASE( _op_, _update_op_,         \
                                     worktype, size.width, cn );\
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


//////////////////////////////////// The macros expanded /////////////////////////////////


#define ICV_DEF_NORM_FUNC_ALL_C(flavor, _abs_, _abs_diff_, arrtype, worktype)\
                                                                            \
ICV_DEF_NORM_NOHINT_FUNC_2D( icvNorm_Inf_##flavor##_C1R,                    \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_NOHINT_FUNC_2D_COI( icvNorm_Inf_##flavor##_CnCR,               \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D( icvNormDiff_Inf_##flavor##_C1R,           \
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )               \
                                                                            \
ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D_COI( icvNormDiff_Inf_##flavor##_CnCR,      \
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )               \
                                                                            \
ICV_DEF_NORM_MASK_NOHINT_FUNC_2D( icvNorm_Inf_##flavor##_C1MR,              \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_MASK_NOHINT_FUNC_2D_COI( icvNorm_Inf_##flavor##_CnCMR,         \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D( icvNormDiff_Inf_##flavor##_C1MR,     \
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )               \
                                                                            \
ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D_COI( icvNormDiff_Inf_##flavor##_CnCMR,\
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )


ICV_DEF_NORM_FUNC_ALL_C( 8u, CV_NOP, CV_IABS, uchar, int )
ICV_DEF_NORM_FUNC_ALL_C( 16u, CV_NOP, CV_IABS, ushort, int )
ICV_DEF_NORM_FUNC_ALL_C( 16s, CV_IABS, CV_IABS, short, int )
// there is no protection from overflow
// (otherwise we had to do everything in int64's or double's)
ICV_DEF_NORM_FUNC_ALL_C( 32s, CV_IABS, CV_IABS, int, int )
ICV_DEF_NORM_FUNC_ALL_C( 32f, fabs, fabs, float, double )
ICV_DEF_NORM_FUNC_ALL_C( 64f, fabs, fabs, double, double )

#define ICV_DEF_NORM_FUNC_ALL_L1( flavor, _abs_, _abs_diff_, hintp_func, nohint_func,\
                                  arrtype, normtype, worktype, block_size )         \
                                                                                    \
ICV_DEF_NORM_##hintp_func##_FUNC_2D( icvNorm_L1_##flavor##_C1R,                     \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_##nohint_func##_FUNC_2D_COI( icvNorm_L1_##flavor##_CnCR,               \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_DIFF_##hintp_func##_FUNC_2D( icvNormDiff_L1_##flavor##_C1R,            \
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )           \
                                                                                    \
ICV_DEF_NORM_DIFF_##nohint_func##_FUNC_2D_COI( icvNormDiff_L1_##flavor##_CnCR,      \
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )           \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D( icvNorm_L1_##flavor##_C1MR,              \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D_COI( icvNorm_L1_##flavor##_CnCMR,         \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D( icvNormDiff_L1_##flavor##_C1MR,     \
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )           \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D_COI( icvNormDiff_L1_##flavor##_CnCMR,\
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )


ICV_DEF_NORM_FUNC_ALL_L1( 8u, CV_NOP, CV_IABS, NOHINT_BLOCK, NOHINT_BLOCK,
                          uchar, int64, int, 1 << 23 )
ICV_DEF_NORM_FUNC_ALL_L1( 16u, CV_NOP, CV_IABS, NOHINT_BLOCK, NOHINT_BLOCK,
                          ushort, int64, int, 1 << 15 )
ICV_DEF_NORM_FUNC_ALL_L1( 16s, CV_IABS, CV_IABS, NOHINT_BLOCK, NOHINT_BLOCK,
                          short, int64, int, 1 << 15 )
// there is no protection from overflow on abs() stage.
// (otherwise we had to do everything in int64's or double's)
ICV_DEF_NORM_FUNC_ALL_L1( 32s, fabs, fabs, NOHINT, NOHINT,
                          int, double, double, INT_MAX )
ICV_DEF_NORM_FUNC_ALL_L1( 32f, fabs, fabs, HINT, NOHINT,
                          float, double, double, INT_MAX )
ICV_DEF_NORM_FUNC_ALL_L1( 64f, fabs, fabs, NOHINT, NOHINT,
                          double, double, double, INT_MAX )


#define ICV_DEF_NORM_FUNC_ALL_L2( flavor, hintp_func, nohint_func, arrtype,         \
                                  normtype, worktype, block_size, sqr_macro )       \
                                                                                    \
ICV_DEF_NORM_##hintp_func##_FUNC_2D( icvNorm_L2_##flavor##_C1R,                     \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_##nohint_func##_FUNC_2D_COI( icvNorm_L2_##flavor##_CnCR,               \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_##hintp_func##_FUNC_2D( icvNormDiff_L2_##flavor##_C1R,            \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_##nohint_func##_FUNC_2D_COI( icvNormDiff_L2_##flavor##_CnCR,      \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D( icvNorm_L2_##flavor##_C1MR,              \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D_COI( icvNorm_L2_##flavor##_CnCMR,         \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D( icvNormDiff_L2_##flavor##_C1MR,     \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D_COI( icvNormDiff_L2_##flavor##_CnCMR,\
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )


ICV_DEF_NORM_FUNC_ALL_L2( 8u, NOHINT_BLOCK, NOHINT_BLOCK, uchar,
                          int64, int, 1 << 15, CV_SQR_8U )
ICV_DEF_NORM_FUNC_ALL_L2( 16u, NOHINT, NOHINT, ushort,
                          double, double, INT_MAX, CV_SQR )
ICV_DEF_NORM_FUNC_ALL_L2( 16s, NOHINT, NOHINT, short,
                          double, double, INT_MAX, CV_SQR )
// there is no protection from overflow on abs() stage.
// (otherwise we had to do everything in int64's or double's)
ICV_DEF_NORM_FUNC_ALL_L2( 32s, NOHINT, NOHINT, int,
                          double, double, INT_MAX, CV_SQR )
ICV_DEF_NORM_FUNC_ALL_L2( 32f, HINT, NOHINT, float,
                          double, double, INT_MAX, CV_SQR )
ICV_DEF_NORM_FUNC_ALL_L2( 64f, NOHINT, NOHINT, double,
                          double, double, INT_MAX, CV_SQR )


#define ICV_DEF_INIT_NORM_TAB_2D( FUNCNAME, FLAG )              \
static void icvInit##FUNCNAME##FLAG##Table( CvFuncTable* tab )  \
{                                                               \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u_##FLAG;       \
    tab->fn_2d[CV_8S] = 0;                                      \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##FLAG;     \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##FLAG;     \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##FLAG;     \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##FLAG;     \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##FLAG;     \
}

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, C1R )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, C1R )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, C1R )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, C1R )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, C1R )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, C1R )

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, CnCR )

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, C1MR )

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, CnCMR )


static void icvInitNormTabs( CvFuncTable* norm_tab, CvFuncTable* normmask_tab )
{
    icvInitNorm_InfC1RTable( &norm_tab[0] );
    icvInitNorm_L1C1RTable( &norm_tab[1] );
    icvInitNorm_L2C1RTable( &norm_tab[2] );
    icvInitNormDiff_InfC1RTable( &norm_tab[3] );
    icvInitNormDiff_L1C1RTable( &norm_tab[4] );
    icvInitNormDiff_L2C1RTable( &norm_tab[5] );

    icvInitNorm_InfCnCRTable( &norm_tab[6] );
    icvInitNorm_L1CnCRTable( &norm_tab[7] );
    icvInitNorm_L2CnCRTable( &norm_tab[8] );
    icvInitNormDiff_InfCnCRTable( &norm_tab[9] );
    icvInitNormDiff_L1CnCRTable( &norm_tab[10] );
    icvInitNormDiff_L2CnCRTable( &norm_tab[11] );

    icvInitNorm_InfC1MRTable( &normmask_tab[0] );
    icvInitNorm_L1C1MRTable( &normmask_tab[1] );
    icvInitNorm_L2C1MRTable( &normmask_tab[2] );
    icvInitNormDiff_InfC1MRTable( &normmask_tab[3] );
    icvInitNormDiff_L1C1MRTable( &normmask_tab[4] );
    icvInitNormDiff_L2C1MRTable( &normmask_tab[5] );

    icvInitNorm_InfCnCMRTable( &normmask_tab[6] );
    icvInitNorm_L1CnCMRTable( &normmask_tab[7] );
    icvInitNorm_L2CnCMRTable( &normmask_tab[8] );
    icvInitNormDiff_InfCnCMRTable( &normmask_tab[9] );
    icvInitNormDiff_L1CnCMRTable( &normmask_tab[10] );
    icvInitNormDiff_L2CnCMRTable( &normmask_tab[11] );
}


CV_IMPL  double
cvNorm( const void* imgA, const void* imgB, int normType, const void* mask )
{
    static CvFuncTable norm_tab[12];
    static CvFuncTable normmask_tab[12];
    static int inittab = 0;

    double  norm = 0, norm_diff = 0;

    CV_FUNCNAME("cvNorm");

    __BEGIN__;

    int type, depth, cn, is_relative;
    CvSize size;
    CvMat stub1, *mat1 = (CvMat*)imgB;
    CvMat stub2, *mat2 = (CvMat*)imgA;
    int mat2_flag = CV_MAT_CONT_FLAG;
    int mat1_step, mat2_step, mask_step = 0;
    int coi = 0, coi2 = 0;

    if( !mat1 )
    {
        mat1 = mat2;
        mat2 = 0;
    }

    is_relative = mat2 && (normType & CV_RELATIVE);
    normType &= ~CV_RELATIVE;

    switch( normType )
    {
    case CV_C:
    case CV_L1:
    case CV_L2:
    case CV_DIFF_C:
    case CV_DIFF_L1:
    case CV_DIFF_L2:
        normType = (normType & 7) >> 1;
        break;
    default:
        CV_ERROR( CV_StsBadFlag, "" );
    }

    /* light variant */
    if( CV_IS_MAT(mat1) && (!mat2 || CV_IS_MAT(mat2)) && !mask )
    {
        if( mat2 )
        {
            if( !CV_ARE_TYPES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );

            if( !CV_ARE_SIZES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );

            mat2_flag = mat2->type;
        }

        size = cvGetMatSize( mat1 );
        type = CV_MAT_TYPE(mat1->type);
        depth = CV_MAT_DEPTH(type);
        cn = CV_MAT_CN(type);

        if( CV_IS_MAT_CONT( mat1->type & mat2_flag ))
        {
            size.width *= size.height;

            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE && normType == 2 /* CV_L2 */ )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = mat1->data.fl;
                    int size0 = size.width *= cn;
                
                    if( !mat2 || is_relative )
                    {
                        do
                        {
                            double t = src1data[size.width-1];
                            norm += t*t;
                        }
                        while( --size.width );
                    }

                    if( mat2 )
                    {
                        const float* src2data = mat2->data.fl;
                        size.width = size0;

                        do
                        {
                            double t = src1data[size.width-1] - src2data[size.width-1];
                            norm_diff += t*t;
                        }
                        while( --size.width );

                        if( is_relative )
                            norm = norm_diff/(norm + DBL_EPSILON);
                        else
                            norm = norm_diff;
                    }
                    norm = sqrt(norm);
                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = mat1->data.db;
                    int size0 = size.width *= cn;

                    if( !mat2 || is_relative )
                    {
                        do
                        {
                            double t = src1data[size.width-1];
                            norm += t*t;
                        }
                        while( --size.width );
                    }

                    if( mat2 )
                    {
                        const double* src2data = mat2->data.db;
                        size.width = size0;

                        do
                        {
                            double t = src1data[size.width-1] - src2data[size.width-1];
                            norm_diff += t*t;
                        }
                        while( --size.width );

                        if( is_relative )
                            norm = norm_diff/(norm + DBL_EPSILON);
                        else
                            norm = norm_diff;
                    }
                    norm = sqrt(norm);
                    EXIT;
                }
            }
            size.height = 1;
            mat1_step = mat2_step = CV_STUB_STEP;
        }
        else
        {
            mat1_step = mat1->step;
            mat2_step = mat2 ? mat2->step : 0;
        }
    }
    else if( !CV_IS_MATND(mat1) && !CV_IS_MATND(mat2) )
    {
        CV_CALL( mat1 = cvGetMat( mat1, &stub1, &coi ));
        
        if( mat2 )
        {
            CV_CALL( mat2 = cvGetMat( mat2, &stub2, &coi2 ));

            if( !CV_ARE_TYPES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );

            if( !CV_ARE_SIZES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );

            if( coi != coi2 && CV_MAT_CN( mat1->type ) > 1 )
                CV_ERROR( CV_BadCOI, "" );

            mat2_flag = mat2->type;
        }

        size = cvGetMatSize( mat1 );
        type = CV_MAT_TYPE(mat1->type);
        depth = CV_MAT_DEPTH(type);
        cn = CV_MAT_CN(type);
        mat1_step = mat1->step;
        mat2_step = mat2 ? mat2->step : 0;

        if( !mask && CV_IS_MAT_CONT( mat1->type & mat2_flag ))
        {
            size.width *= size.height;
            size.height = 1;
            mat1_step = mat2_step = CV_STUB_STEP;
        }
    }
    else
    {
        CvArr* arrs[] = { mat1, mat2 };
        CvMatND stubs[2];
        CvNArrayIterator iterator;
        int pass_hint;

        if( !inittab )
        {
            icvInitNormTabs( norm_tab, normmask_tab );
            inittab = 1;
        }

        if( mask )
            CV_ERROR( CV_StsBadMask,
            "This operation on multi-dimensional arrays does not support mask" );

        CV_CALL( cvInitNArrayIterator( 1 + (mat2 != 0), arrs, 0, stubs, &iterator ));

        type = CV_MAT_TYPE(iterator.hdr[0]->type);
        depth = CV_MAT_DEPTH(type);
        iterator.size.width *= CV_MAT_CN(type);

        pass_hint = normType != 0 && (depth == CV_32F); 

        if( !mat2 || is_relative )
        {
            if( !pass_hint )
            {
                CvFunc2D_1A1P func;
        
                CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P)norm_tab[normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.size, &temp ));
                    norm += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_1A1P1I func;

                CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P1I)norm_tab[normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.size, &temp, cvAlgHintAccurate ));
                    norm += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }
        }

        if( mat2 )
        {
            if( !pass_hint )
            {
                CvFunc2D_2A1P func;
                CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P)norm_tab[3 + normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.size, &temp ));
                    norm_diff += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_2A1P1I func;
                CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P1I)norm_tab[3 + normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.size, &temp, cvAlgHintAccurate ));
                    norm_diff += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }

            if( is_relative )
                norm = norm_diff/(norm + DBL_EPSILON);
            else
                norm = norm_diff;
        }
        EXIT;
    }

    if( !inittab )
    {
        icvInitNormTabs( norm_tab, normmask_tab );
        inittab = 1;
    }

    if( !mask )
    {
        if( cn == 1 || coi == 0 )
        {
            int pass_hint = depth == CV_32F && normType != 0;
            size.width *= cn;

            if( !mat2 || is_relative )
            {
                if( !pass_hint )
                {
                    CvFunc2D_1A1P func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P)norm_tab[normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, size, &norm ));
                }
                else
                {
                    CvFunc2D_1A1P1I func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P1I)norm_tab[normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, size, &norm, cvAlgHintAccurate ));
                }
            }
        
            if( mat2 )
            {
                if( !pass_hint )
                {
                    CvFunc2D_2A1P func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P)norm_tab[3 + normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                     size, &norm_diff ));
                }
                else
                {
                    CvFunc2D_2A1P1I func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P1I)norm_tab[3 + normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                     size, &norm_diff, cvAlgHintAccurate ));
                }

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
        else
        {
            if( !mat2 || is_relative )
            {
                CvFunc2DnC_1A1P func;
                CV_GET_FUNC_PTR( func, (CvFunc2DnC_1A1P)norm_tab[6 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step, size, cn, coi, &norm ));
            }
        
            if( mat2 )
            {
                CvFunc2DnC_2A1P func;
                CV_GET_FUNC_PTR( func, (CvFunc2DnC_2A1P)norm_tab[9 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                 size, cn, coi, &norm_diff ));

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
    }
    else
    {
        CvMat maskstub, *matmask = (CvMat*)mask;

        if( CV_MAT_CN(type) > 1 && coi == 0 )
            CV_ERROR( CV_StsBadArg, "" );

        CV_CALL( matmask = cvGetMat( matmask, &maskstub ));

        if( !CV_IS_MASK_ARR( matmask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat1, matmask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        
        mask_step = matmask->step;

        if( CV_IS_MAT_CONT( mat1->type & mat2_flag & matmask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            mat1_step = mat2_step = mask_step = CV_STUB_STEP;
        }

        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            if( !mat2 || is_relative )
            {
                CvFunc2D_2A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2D_2A1P)normmask_tab[normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step,
                                 matmask->data.ptr, mask_step, size, &norm ));
            }
        
            if( mat2 )
            {
                CvFunc2D_3A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2D_3A1P)normmask_tab[3 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                 matmask->data.ptr, mask_step, size, &norm_diff ));

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
        else
        {
            if( !mat2 || is_relative )
            {
                CvFunc2DnC_2A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2DnC_2A1P)normmask_tab[6 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step,
                                 matmask->data.ptr, mask_step,
                                 size, cn, coi, &norm ));
            }
        
            if( mat2 )
            {
                CvFunc2DnC_3A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2DnC_3A1P)normmask_tab[9 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step,
                                 mat2->data.ptr, mat2_step,
                                 matmask->data.ptr, mask_step,
                                 size, cn, coi, &norm_diff ));

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
    }

    __END__;

    return norm;
}

/* End of file. */
