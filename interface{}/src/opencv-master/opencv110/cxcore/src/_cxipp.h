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

#ifndef _CXCORE_IPP_H_
#define _CXCORE_IPP_H_

/****************************************************************************************\
*                                      Copy/Set                                          *
\****************************************************************************************/

/* temporary disable ipp zero and copy functions as they affect subsequent functions' performance */
IPCVAPI_EX( CvStatus, icvCopy_8u_C1R, "ippiCopy_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,
                  ( const uchar* src, int src_step,
                    uchar* dst, int dst_step, CvSize size ))

IPCVAPI_EX( CvStatus, icvSetByte_8u_C1R, "ippiSet_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,
                  ( uchar value, uchar* dst, int dst_step, CvSize size ))

IPCVAPI_EX( CvStatus, icvCvt_32f64f, "ippsConvert_32f64f",
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const float* src, double* dst, int len ))
IPCVAPI_EX( CvStatus, icvCvt_64f32f, "ippsConvert_64f32f",
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const double* src, float* dst, int len ))

#define IPCV_COPYSET( flavor, arrtype, scalartype )                                 \
IPCVAPI_EX( CvStatus, icvCopy##flavor, "ippiCopy" #flavor,                          \
                                    CV_PLUGINS1(CV_PLUGIN_IPPI),                    \
                                   ( const arrtype* src, int srcstep,               \
                                     arrtype* dst, int dststep, CvSize size,        \
                                     const uchar* mask, int maskstep ))             \
IPCVAPI_EX( CvStatus, icvSet##flavor, "ippiSet" #flavor,                            \
                                    0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,              \
                                  ( arrtype* dst, int dststep,                      \
                                    const uchar* mask, int maskstep,                \
                                    CvSize size, const arrtype* scalar ))

IPCV_COPYSET( _8u_C1MR, uchar, int )
IPCV_COPYSET( _16s_C1MR, ushort, int )
IPCV_COPYSET( _8u_C3MR, uchar, int )
IPCV_COPYSET( _8u_C4MR, int, int )
IPCV_COPYSET( _16s_C3MR, ushort, int )
IPCV_COPYSET( _16s_C4MR, int64, int64 )
IPCV_COPYSET( _32f_C3MR, int, int )
IPCV_COPYSET( _32f_C4MR, int, int )
IPCV_COPYSET( _64s_C3MR, int64, int64 )
IPCV_COPYSET( _64s_C4MR, int64, int64 )


/****************************************************************************************\
*                                       Arithmetics                                      *
\****************************************************************************************/

#define IPCV_BIN_ARITHM( name )                                     \
IPCVAPI_EX( CvStatus, icv##name##_8u_C1R,                           \
    "ippi" #name "_8u_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
( const uchar* src1, int srcstep1, const uchar* src2, int srcstep2, \
  uchar* dst, int dststep, CvSize size, int scalefactor ))          \
IPCVAPI_EX( CvStatus, icv##name##_16u_C1R,                          \
    "ippi" #name "_16u_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
( const ushort* src1, int srcstep1, const ushort* src2, int srcstep2,\
  ushort* dst, int dststep, CvSize size, int scalefactor ))         \
IPCVAPI_EX( CvStatus, icv##name##_16s_C1R,                          \
    "ippi" #name "_16s_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
( const short* src1, int srcstep1, const short* src2, int srcstep2, \
  short* dst, int dststep, CvSize size, int scalefactor ))          \
IPCVAPI_EX( CvStatus, icv##name##_32s_C1R,                          \
    "ippi" #name "_32s_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const int* src1, int srcstep1, const int* src2, int srcstep2,     \
  int* dst, int dststep, CvSize size ))                             \
IPCVAPI_EX( CvStatus, icv##name##_32f_C1R,                          \
    "ippi" #name "_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const float* src1, int srcstep1, const float* src2, int srcstep2, \
  float* dst, int dststep, CvSize size ))                           \
IPCVAPI_EX( CvStatus, icv##name##_64f_C1R,                          \
    "ippi" #name "_64f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const double* src1, int srcstep1, const double* src2, int srcstep2,\
  double* dst, int dststep, CvSize size ))


IPCV_BIN_ARITHM( Add )
IPCV_BIN_ARITHM( Sub )

#undef IPCV_BIN_ARITHM

/****************************************************************************************\
*                                     Logical operations                                 *
\****************************************************************************************/

#define IPCV_LOGIC( name )                                              \
IPCVAPI_EX( CvStatus, icv##name##_8u_C1R,                               \
    "ippi" #name "_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,           \
( const uchar* src1, int srcstep1, const uchar* src2, int srcstep2,     \
  uchar* dst, int dststep, CvSize size ))

IPCV_LOGIC( And )
IPCV_LOGIC( Or )
IPCV_LOGIC( Xor )

#undef IPCV_LOGIC

IPCVAPI_EX( CvStatus, icvNot_8u_C1R, "ippiNot_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
( const uchar* src, int step1, uchar* dst, int step, CvSize size ))

/****************************************************************************************\
*                                Image Statistics                                        *
\****************************************************************************************/

///////////////////////////////////////// Mean //////////////////////////////////////////

#define IPCV_DEF_MEAN_MASK( flavor, srctype )           \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C1MR,          \
"ippiMean_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV), \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))            \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C2MR,          \
"ippiMean_" #flavor "_C2MR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/, \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))            \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C3MR,          \
"ippiMean_" #flavor "_C3MR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/, \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))            \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C4MR,          \
"ippiMean_" #flavor "_C4MR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/, \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))

IPCV_DEF_MEAN_MASK( 8u, uchar )
IPCV_DEF_MEAN_MASK( 16u, ushort )
IPCV_DEF_MEAN_MASK( 16s, short )
IPCV_DEF_MEAN_MASK( 32s, int )
IPCV_DEF_MEAN_MASK( 32f, float )
IPCV_DEF_MEAN_MASK( 64f, double )

#undef IPCV_DEF_MEAN_MASK

//////////////////////////////////// Mean_StdDev ////////////////////////////////////////

#undef IPCV_MEAN_SDV_PLUGIN
#define ICV_MEAN_SDV_PLUGIN 0 /* CV_PLUGINS1(IPPCV) */

#define IPCV_DEF_MEAN_SDV( flavor, srctype )                                \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C1R,                        \
"ippiMean_StdDev_" #flavor "_C1R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C2R,                        \
"ippiMean_StdDev_" #flavor "_C2R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C3R,                        \
"ippiMean_StdDev_" #flavor "_C3R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C4R,                        \
"ippiMean_StdDev_" #flavor "_C4R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
                                                                            \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C1MR,                       \
"ippiMean_StdDev_" #flavor "_C1MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,                                          \
  const uchar* mask, int maskStep,                                          \
  CvSize size, double* mean, double* sdv ))                                 \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C2MR,                       \
"ippiMean_StdDev_" #flavor "_C2MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,  const uchar* mask, int maskStep,        \
  CvSize size, double* mean, double* sdv ))                                 \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C3MR,                       \
"ippiMean_StdDev_" #flavor "_C3MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,                                          \
  const uchar* mask, int maskStep,                                          \
  CvSize size, double* mean, double* sdv ))                                 \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C4MR,                       \
"ippiMean_StdDev_" #flavor "_C4MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,                                          \
  const uchar* mask, int maskStep,                                          \
  CvSize size, double* mean, double* sdv ))

IPCV_DEF_MEAN_SDV( 8u, uchar )
IPCV_DEF_MEAN_SDV( 16u, ushort )
IPCV_DEF_MEAN_SDV( 16s, short )
IPCV_DEF_MEAN_SDV( 32s, int )
IPCV_DEF_MEAN_SDV( 32f, float )
IPCV_DEF_MEAN_SDV( 64f, double )

#undef IPCV_DEF_MEAN_SDV
#undef IPCV_MEAN_SDV_PLUGIN

//////////////////////////////////// MinMaxIndx /////////////////////////////////////////

#define IPCV_DEF_MIN_MAX_LOC( flavor, srctype, extrtype, plugin ) \
IPCVAPI_EX( CvStatus, icvMinMaxIndx_##flavor##_C1R,             \
"ippiMinMaxIndx_" #flavor "_C1R", plugin,                       \
( const srctype* img, int imgstep,                              \
  CvSize size, extrtype* minVal, extrtype* maxVal,              \
  CvPoint* minLoc, CvPoint* maxLoc ))                           \
                                                                \
IPCVAPI_EX( CvStatus, icvMinMaxIndx_##flavor##_C1MR,            \
"ippiMinMaxIndx_" #flavor "_C1MR", plugin,                      \
( const srctype* img, int imgstep,                              \
  const uchar* mask, int maskStep,                              \
  CvSize size, extrtype* minVal, extrtype* maxVal,              \
  CvPoint* minLoc, CvPoint* maxLoc ))

IPCV_DEF_MIN_MAX_LOC( 8u, uchar, float, CV_PLUGINS1(CV_PLUGIN_IPPCV) )
IPCV_DEF_MIN_MAX_LOC( 16u, ushort, float, 0 )
IPCV_DEF_MIN_MAX_LOC( 16s, short, float, CV_PLUGINS1(CV_PLUGIN_IPPCV) )
IPCV_DEF_MIN_MAX_LOC( 32s, int, double, 0 )
#if !defined WIN64 && (defined WIN32 || defined __i386__)
IPCV_DEF_MIN_MAX_LOC( 32f, int, float, CV_PLUGINS1(CV_PLUGIN_IPPCV) )
#else
IPCV_DEF_MIN_MAX_LOC( 32f, int, float, 0 )
#endif
IPCV_DEF_MIN_MAX_LOC( 64f, int64, double, 0 )

#undef IPCV_DEF_MIN_MAX_LOC

////////////////////////////////////////// Sum //////////////////////////////////////////

#define IPCV_DEF_SUM_NOHINT( flavor, srctype, plugin )                      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C1R,                                \
            "ippiSum_" #flavor "_C1R", plugin,                              \
            ( const srctype* img, int imgstep, CvSize size, double* sum ))  \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C2R,                                \
           "ippiSum_" #flavor "_C2R", plugin,                               \
            ( const srctype* img, int imgstep, CvSize size, double* sum ))  \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C3R,                                \
           "ippiSum_" #flavor "_C3R", plugin,                               \
            ( const srctype* img, int imgstep, CvSize size, double* sum ))  \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C4R,                                \
           "ippiSum_" #flavor "_C4R", plugin,                               \
            ( const srctype* img, int imgstep, CvSize size, double* sum ))

#define IPCV_DEF_SUM_HINT( flavor, srctype )                                \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C1R,                                \
            "ippiSum_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C2R,                                \
           "ippiSum_" #flavor "_C2R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C3R,                                \
           "ippiSum_" #flavor "_C3R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C4R,                                \
           "ippiSum_" #flavor "_C4R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))

IPCV_DEF_SUM_NOHINT( 8u, uchar, CV_PLUGINS1(CV_PLUGIN_IPPI) )
IPCV_DEF_SUM_NOHINT( 16s, short, CV_PLUGINS1(CV_PLUGIN_IPPI) )
IPCV_DEF_SUM_NOHINT( 16u, ushort, 0 )
IPCV_DEF_SUM_NOHINT( 32s, int, 0 )
IPCV_DEF_SUM_HINT( 32f, float )
IPCV_DEF_SUM_NOHINT( 64f, double, 0 )

#undef IPCV_DEF_SUM_NOHINT
#undef IPCV_DEF_SUM_HINT

////////////////////////////////////////// CountNonZero /////////////////////////////////

#define IPCV_DEF_NON_ZERO( flavor, srctype )                            \
IPCVAPI_EX( CvStatus, icvCountNonZero_##flavor##_C1R,                   \
    "ippiCountNonZero_" #flavor "_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,   \
    ( const srctype* img, int imgstep, CvSize size, int* nonzero ))

IPCV_DEF_NON_ZERO( 8u, uchar )
IPCV_DEF_NON_ZERO( 16s, ushort )
IPCV_DEF_NON_ZERO( 32s, int )
IPCV_DEF_NON_ZERO( 32f, int )
IPCV_DEF_NON_ZERO( 64f, int64 )

#undef IPCV_DEF_NON_ZERO

////////////////////////////////////////// Norms /////////////////////////////////

#define IPCV_DEF_NORM_NOHINT_C1( flavor, srctype, plugin )                              \
IPCVAPI_EX( CvStatus, icvNorm_Inf_##flavor##_C1R,                                       \
            "ippiNorm_Inf_" #flavor "_C1R", plugin,                                     \
            ( const srctype* img, int imgstep, CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNorm_L1_##flavor##_C1R,                                        \
           "ippiNorm_L1_" #flavor "_C1R", plugin,                                       \
            ( const srctype* img, int imgstep, CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNorm_L2_##flavor##_C1R,                                        \
           "ippiNorm_L2_" #flavor "_C1R", plugin,                                       \
            ( const srctype* img, int imgstep, CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_Inf_##flavor##_C1R,                                   \
           "ippiNormDiff_Inf_" #flavor "_C1R", plugin,                                  \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L1_##flavor##_C1R,                                    \
           "ippiNormDiff_L1_" #flavor "_C1R", plugin,                                   \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L2_##flavor##_C1R,                                    \
           "ippiNormDiff_L2_" #flavor "_C1R", plugin,                                   \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               CvSize size, double* norm ))

#define IPCV_DEF_NORM_HINT_C1( flavor, srctype )                                        \
IPCVAPI_EX( CvStatus, icvNorm_Inf_##flavor##_C1R,                                       \
            "ippiNorm_Inf_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                \
                                        ( const srctype* img, int imgstep,              \
                                          CvSize size, double* norm ))                  \
IPCVAPI_EX( CvStatus, icvNorm_L1_##flavor##_C1R,                                        \
           "ippiNorm_L1_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                  \
                                        ( const srctype* img, int imgstep,              \
                                          CvSize size, double* norm, CvHintAlgorithm )) \
IPCVAPI_EX( CvStatus, icvNorm_L2_##flavor##_C1R,                                        \
           "ippiNorm_L2_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                  \
                                        ( const srctype* img, int imgstep,              \
                                          CvSize size, double* norm, CvHintAlgorithm )) \
IPCVAPI_EX( CvStatus, icvNormDiff_Inf_##flavor##_C1R,                                   \
           "ippiNormDiff_Inf_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),             \
                                        ( const srctype* img1, int imgstep1,            \
                                          const srctype* img2, int imgstep2,            \
                                          CvSize size, double* norm ))                  \
IPCVAPI_EX( CvStatus, icvNormDiff_L1_##flavor##_C1R,                                    \
           "ippiNormDiff_L1_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),              \
                                        ( const srctype* img1, int imgstep1,            \
                                          const srctype* img2, int imgstep2,            \
                                          CvSize size, double* norm, CvHintAlgorithm )) \
IPCVAPI_EX( CvStatus, icvNormDiff_L2_##flavor##_C1R,                                    \
           "ippiNormDiff_L2_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),              \
                                        ( const srctype* img1, int imgstep1,            \
                                          const srctype* img2, int imgstep2,            \
                                          CvSize size, double* norm, CvHintAlgorithm ))

#define IPCV_DEF_NORM_MASK_C1( flavor, srctype, plugin )                                \
IPCVAPI_EX( CvStatus, icvNorm_Inf_##flavor##_C1MR,                                      \
           "ippiNorm_Inf_" #flavor "_C1MR", plugin,                                     \
                                             ( const srctype* img, int imgstep,         \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNorm_L1_##flavor##_C1MR,                                       \
            "ippiNorm_L1_" #flavor "_C1MR", plugin,                                     \
                                             ( const srctype* img, int imgstep,         \
                                                const uchar* mask, int maskstep,        \
                                                CvSize size, double* norm ))            \
IPCVAPI_EX( CvStatus, icvNorm_L2_##flavor##_C1MR,                                       \
           "ippiNorm_L2_" #flavor "_C1MR", plugin,                                      \
                                             ( const srctype* img, int imgstep,         \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_Inf_##flavor##_C1MR,                                  \
           "ippiNormDiff_Inf_" #flavor "_C1MR", plugin,                                 \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L1_##flavor##_C1MR,                                   \
           "ippiNormDiff_L1_" #flavor "_C1MR", plugin,                                  \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L2_##flavor##_C1MR,                                   \
           "ippiNormDiff_L2_" #flavor "_C1MR", plugin,                                  \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))

IPCV_DEF_NORM_NOHINT_C1( 8u, uchar, CV_PLUGINS1(CV_PLUGIN_IPPI) )
IPCV_DEF_NORM_MASK_C1( 8u, uchar, CV_PLUGINS1(CV_PLUGIN_IPPCV) )

IPCV_DEF_NORM_NOHINT_C1( 16u, ushort, 0 )
IPCV_DEF_NORM_MASK_C1( 16u, ushort, 0 )

IPCV_DEF_NORM_NOHINT_C1( 16s, short, CV_PLUGINS1(CV_PLUGIN_IPPI) )
IPCV_DEF_NORM_MASK_C1( 16s, short, CV_PLUGINS1(CV_PLUGIN_IPPCV) )

IPCV_DEF_NORM_NOHINT_C1( 32s, int, 0 )
IPCV_DEF_NORM_MASK_C1( 32s, int, 0 )

IPCV_DEF_NORM_HINT_C1( 32f, float )
IPCV_DEF_NORM_MASK_C1( 32f, float, CV_PLUGINS1(CV_PLUGIN_IPPCV) )

IPCV_DEF_NORM_NOHINT_C1( 64f, double, 0 )
IPCV_DEF_NORM_MASK_C1( 64f, double, 0 )

#undef IPCV_DEF_NORM_HONINT_C1
#undef IPCV_DEF_NORM_HINT_C1
#undef IPCV_DEF_NORM_MASK_C1


////////////////////////////////////// AbsDiff ///////////////////////////////////////////

#define IPCV_ABS_DIFF( flavor, arrtype )                    \
IPCVAPI_EX( CvStatus, icvAbsDiff_##flavor##_C1R,            \
"ippiAbsDiff_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),\
( const arrtype* src1, int srcstep1,                        \
  const arrtype* src2, int srcstep2,                        \
  arrtype* dst, int dststep, CvSize size ))

IPCV_ABS_DIFF( 8u, uchar )
IPCV_ABS_DIFF( 16u, ushort )
IPCV_ABS_DIFF( 16s, short )
IPCV_ABS_DIFF( 32s, int )
IPCV_ABS_DIFF( 32f, float )
IPCV_ABS_DIFF( 64f, double )

#undef IPCV_ABS_DIFF

////////////////////////////// Comparisons //////////////////////////////////////////

#define IPCV_CMP( arrtype, flavor )                                                 \
IPCVAPI_EX( CvStatus, icvCompare_##flavor##_C1R,                                    \
            "ippiCompare_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),             \
            ( const arrtype* src1, int srcstep1, const arrtype* src2, int srcstep2, \
              arrtype* dst, int dststep, CvSize size, int cmp_op ))                 \
IPCVAPI_EX( CvStatus, icvCompareC_##flavor##_C1R,                                   \
            "ippiCompareC_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),            \
            ( const arrtype* src1, int srcstep1, arrtype scalar,                    \
              arrtype* dst, int dststep, CvSize size, int cmp_op ))                 \
IPCVAPI_EX( CvStatus, icvThreshold_GT_##flavor##_C1R,                               \
            "ippiThreshold_GT_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
            ( const arrtype* pSrc, int srcstep, arrtype* pDst, int dststep,         \
              CvSize size, arrtype threshold ))                                     \
IPCVAPI_EX( CvStatus, icvThreshold_LT_##flavor##_C1R,                               \
            "ippiThreshold_LT_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
            ( const arrtype* pSrc, int srcstep, arrtype* pDst, int dststep,         \
              CvSize size, arrtype threshold ))
IPCV_CMP( uchar, 8u )
IPCV_CMP( short, 16s )
IPCV_CMP( float, 32f )
#undef IPCV_CMP

/****************************************************************************************\
*                                       Utilities                                        *
\****************************************************************************************/

////////////////////////////// Copy Pixel <-> Plane /////////////////////////////////

#define IPCV_PIX_PLANE( flavor, arrtype )                                           \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C2P2R,                                     \
    "ippiCopy_" #flavor "_C2P2R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,                 \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C3P3R,                                     \
    "ippiCopy_" #flavor "_C3P3R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C4P4R,                                     \
    "ippiCopy_" #flavor "_C4P4R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_CnC1CR,                                    \
    "ippiCopy_" #flavor "_CnC1CR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,               \
    ( const arrtype* src, int srcstep, arrtype* dst, int dststep,                   \
      CvSize size, int cn, int coi ))                                               \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C1CnCR,                                    \
    "ippiCopy_" #flavor "_CnC1CR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,               \
    ( const arrtype* src, int srcstep, arrtype* dst, int dststep,                   \
      CvSize size, int cn, int coi ))                                               \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P2C2R,                                     \
    "ippiCopy_" #flavor "_P2C2R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,                 \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P3C3R,                                     \
    "ippiCopy_" #flavor "_P3C3R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P4C4R,                                     \
    "ippiCopy_" #flavor "_P4C4R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))

IPCV_PIX_PLANE( 8u, uchar )
IPCV_PIX_PLANE( 16s, ushort )
IPCV_PIX_PLANE( 32f, int )
IPCV_PIX_PLANE( 64f, int64 )

#undef IPCV_PIX_PLANE

/****************************************************************************************/
/*                            Math routines and RNGs                                    */
/****************************************************************************************/

IPCVAPI_EX( CvStatus, icvInvSqrt_32f, "ippsInvSqrt_32f_A21",
           CV_PLUGINS1(CV_PLUGIN_IPPVM),
           ( const float* src, float* dst, int len ))
IPCVAPI_EX( CvStatus, icvSqrt_32f, "ippsSqrt_32f_A21, ippsSqrt_32f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const float* src, float* dst, int len ))
IPCVAPI_EX( CvStatus, icvInvSqrt_64f, "ippsInvSqrt_64f_A50",
           CV_PLUGINS1(CV_PLUGIN_IPPVM),
           ( const double* src, double* dst, int len ))
IPCVAPI_EX( CvStatus, icvSqrt_64f, "ippsSqrt_64f_A50, ippsSqrt_64f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const double* src, double* dst, int len ))

IPCVAPI_EX( CvStatus, icvLog_32f, "ippsLn_32f_A21, ippsLn_32f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const float *x, float *y, int n ) )
IPCVAPI_EX( CvStatus, icvLog_64f, "ippsLn_64f_A50, ippsLn_64f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const double *x, double *y, int n ) )
IPCVAPI_EX( CvStatus, icvExp_32f, "ippsExp_32f_A21, ippsExp_32f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const float *x, float *y, int n ) )
IPCVAPI_EX( CvStatus, icvExp_64f, "ippsExp_64f_A50, ippsExp_64f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const double *x, double *y, int n ) )
IPCVAPI_EX( CvStatus, icvFastArctan_32f, "ippibFastArctan_32f",
           CV_PLUGINS1(CV_PLUGIN_IPPCV),
           ( const float* y, const float* x, float* angle, int len ))

/****************************************************************************************/
/*                                  Error handling functions                            */
/****************************************************************************************/

IPCVAPI_EX( CvStatus, icvCheckArray_32f_C1R,
           "ippiCheckArray_32f_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,
           ( const float* src, int srcstep,
             CvSize size, int flags,
             double min_val, double max_val ))

IPCVAPI_EX( CvStatus, icvCheckArray_64f_C1R,
           "ippiCheckArray_64f_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,
           ( const double* src, int srcstep,
             CvSize size, int flags,
             double min_val, double max_val ))

/****************************************************************************************/
/*                    Affine transformations of matrix/image elements                   */
/****************************************************************************************/

#define IPCV_TRANSFORM( suffix, ipp_suffix, cn )                \
IPCVAPI_EX( CvStatus, icvColorTwist##suffix##_C##cn##R,         \
        "ippiColorTwist" #ipp_suffix "_C" #cn                   \
        "R,ippiColorTwist" #ipp_suffix "_C" #cn "R",            \
        CV_PLUGINS2(CV_PLUGIN_IPPI, CV_PLUGIN_IPPCC),           \
        ( const void* src, int srcstep, void* dst, int dststep, \
          CvSize roisize, const float* twist_matrix ))

IPCV_TRANSFORM( _8u, 32f_8u, 3 )
IPCV_TRANSFORM( _16u, 32f_16u, 3 )
IPCV_TRANSFORM( _16s, 32f_16s, 3 )
IPCV_TRANSFORM( _32f, _32f, 3 )
IPCV_TRANSFORM( _32f, _32f, 4 )

#undef IPCV_TRANSFORM

#define IPCV_TRANSFORM_N1( suffix )                             \
IPCVAPI_EX( CvStatus, icvColorToGray##suffix,                   \
        "ippiColorToGray" #suffix ",ippiColorToGray" #suffix,   \
        CV_PLUGINS2(CV_PLUGIN_IPPI,CV_PLUGIN_IPPCC),            \
        ( const void* src, int srcstep, void* dst, int dststep, \
          CvSize roisize, const float* coeffs ))

IPCV_TRANSFORM_N1( _8u_C3C1R )
IPCV_TRANSFORM_N1( _16u_C3C1R )
IPCV_TRANSFORM_N1( _16s_C3C1R )
IPCV_TRANSFORM_N1( _32f_C3C1R )
IPCV_TRANSFORM_N1( _8u_AC4C1R )
IPCV_TRANSFORM_N1( _16u_AC4C1R )
IPCV_TRANSFORM_N1( _16s_AC4C1R )
IPCV_TRANSFORM_N1( _32f_AC4C1R )

#undef IPCV_TRANSFORM_N1

/****************************************************************************************/
/*                  Matrix routines from BLAS/LAPACK compatible libraries               */
/****************************************************************************************/

IPCVAPI_C_EX( void, icvBLAS_GEMM_32f, "sgemm, mkl_sgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_64f, "dgemm, mkl_dgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_32fc, "cgemm, mkl_cgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_64fc, "zgemm, mkl_zgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))


#define IPCV_DFT( init_flavor, fwd_flavor, inv_flavor )                                 \
IPCVAPI_EX( CvStatus, icvDFTInitAlloc_##init_flavor, "ippsDFTInitAlloc_" #init_flavor,  \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( void**, int, int, CvHintAlgorithm ))         \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTFree_##init_flavor, "ippsDFTFree_" #init_flavor,            \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( void* ))                                     \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTGetBufSize_##init_flavor, "ippsDFTGetBufSize_" #init_flavor,\
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const void* spec, int* buf_size ))           \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTFwd_##fwd_flavor, "ippsDFTFwd_" #fwd_flavor,                \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const void* src, void* dst,                  \
            const void* spec, void* buffer ))                                           \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTInv_##inv_flavor, "ippsDFTInv_" #inv_flavor,                \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const void* src, void* dst,                  \
            const void* spec, void* buffer ))

IPCV_DFT( C_32fc, CToC_32fc, CToC_32fc )
IPCV_DFT( R_32f, RToPack_32f, PackToR_32f )
IPCV_DFT( C_64fc, CToC_64fc, CToC_64fc )
IPCV_DFT( R_64f, RToPack_64f, PackToR_64f )
#undef IPCV_DFT

#endif /*_CXCORE_IPP_H_*/

