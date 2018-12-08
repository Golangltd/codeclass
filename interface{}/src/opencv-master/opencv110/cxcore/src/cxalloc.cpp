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

// default <malloc>
static void*
icvDefaultAlloc( size_t size, void* )
{
    char *ptr, *ptr0 = (char*)malloc(
        (size_t)(size + CV_MALLOC_ALIGN*((size >= 4096) + 1) + sizeof(char*)));

    if( !ptr0 )
        return 0;

    // align the pointer
    ptr = (char*)cvAlignPtr(ptr0 + sizeof(char*) + 1, CV_MALLOC_ALIGN);
    *(char**)(ptr - sizeof(char*)) = ptr0;

    return ptr;
}


// default <free>
static int
icvDefaultFree( void* ptr, void* )
{
    // Pointer must be aligned by CV_MALLOC_ALIGN
    if( ((size_t)ptr & (CV_MALLOC_ALIGN-1)) != 0 )
        return CV_BADARG_ERR;
    free( *((char**)ptr - 1) );

    return CV_OK;
}


// pointers to allocation functions, initially set to default
static CvAllocFunc p_cvAlloc = icvDefaultAlloc;
static CvFreeFunc p_cvFree = icvDefaultFree;
static void* p_cvAllocUserData = 0;

CV_IMPL void cvSetMemoryManager( CvAllocFunc alloc_func, CvFreeFunc free_func, void* userdata )
{
    CV_FUNCNAME( "cvSetMemoryManager" );

    __BEGIN__;
    
    if( (alloc_func == 0) ^ (free_func == 0) )
        CV_ERROR( CV_StsNullPtr, "Either both pointers should be NULL or none of them");

    p_cvAlloc = alloc_func ? alloc_func : icvDefaultAlloc;
    p_cvFree = free_func ? free_func : icvDefaultFree;
    p_cvAllocUserData = userdata;

    __END__;
}


CV_IMPL  void*  cvAlloc( size_t size )
{
    void* ptr = 0;
    
    CV_FUNCNAME( "cvAlloc" );

    __BEGIN__;

    if( (size_t)size > CV_MAX_ALLOC_SIZE )
        CV_ERROR( CV_StsOutOfRange,
                  "Negative or too large argument of cvAlloc function" );

    ptr = p_cvAlloc( size, p_cvAllocUserData );
    if( !ptr )
        CV_ERROR( CV_StsNoMem, "Out of memory" );

    __END__;

    return ptr;
}


CV_IMPL  void  cvFree_( void* ptr )
{
    CV_FUNCNAME( "cvFree_" );

    __BEGIN__;

    if( ptr )
    {
        CVStatus status = p_cvFree( ptr, p_cvAllocUserData );
        if( status < 0 )
            CV_ERROR( status, "Deallocation error" );
    }

    __END__;
}

/* End of file. */
