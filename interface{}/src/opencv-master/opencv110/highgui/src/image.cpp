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

//
// Image.cpp: implementation of the CvvImage class.
//
//////////////////////////////////////////////////////////////////////

#include "_highgui.h"

#if defined __cplusplus && (!defined WIN32 || !defined __GNUC__)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CvvImage::CvvImage()
{
    m_img = 0;
}

void CvvImage::Destroy()
{
    cvReleaseImage( &m_img );
}

CvvImage::~CvvImage()
{
    Destroy();
}

bool  CvvImage::Create( int w, int h, int bpp, int origin )
{
    const unsigned max_img_size = 10000;

    if( (bpp != 8 && bpp != 24 && bpp != 32) ||
        (unsigned)w >=  max_img_size || (unsigned)h >= max_img_size ||
        (origin != IPL_ORIGIN_TL && origin != IPL_ORIGIN_BL))
    {
        assert(0); // most probably, it is a programming error
        return false;
    }
    
    if( !m_img || Bpp() != bpp || m_img->width != w || m_img->height != h )
    {
        if( m_img && m_img->nSize == sizeof(IplImage))
            Destroy();
    
        /* prepare IPL header */
        m_img = cvCreateImage( cvSize( w, h ), IPL_DEPTH_8U, bpp/8 );
    }

    if( m_img )
        m_img->origin = origin == 0 ? IPL_ORIGIN_TL : IPL_ORIGIN_BL;

    return m_img != 0;
}

void  CvvImage::CopyOf( CvvImage& image, int desired_color )
{
    IplImage* img = image.GetImage();
    if( img )
    {
        CopyOf( img, desired_color );
    }
}


#define HG_IS_IMAGE(img)                                                  \
    ((img) != 0 && ((const IplImage*)(img))->nSize == sizeof(IplImage) && \
    ((IplImage*)img)->imageData != 0)


void  CvvImage::CopyOf( IplImage* img, int desired_color )
{
    if( HG_IS_IMAGE(img) )
    {
        int color = desired_color;
        CvSize size = cvGetSize( img ); 

        if( color < 0 )
            color = img->nChannels > 1;

        if( Create( size.width, size.height,
                    (!color ? 1 : img->nChannels > 1 ? img->nChannels : 3)*8,
                    img->origin ))
        {
            cvConvertImage( img, m_img, 0 );
        }
    }
}


bool  CvvImage::Load( const char* filename, int desired_color )
{
    IplImage* img = cvLoadImage( filename, desired_color );
    if( !img )
        return false;

    CopyOf( img, desired_color );
    cvReleaseImage( &img );

    return true;
}


bool  CvvImage::LoadRect( const char* filename,
                          int desired_color, CvRect r )
{
    if( r.width < 0 || r.height < 0 ) return false;

    IplImage* img = cvLoadImage( filename, desired_color );
    if( !img )
        return false;

    if( r.width == 0 || r.height == 0 )
    {
        r.width = img->width;
        r.height = img->height;
        r.x = r.y = 0;
    }

    if( r.x > img->width || r.y > img->height ||
        r.x + r.width < 0 || r.y + r.height < 0 )
    {
        cvReleaseImage( &img );
        return false;
    }

    /* truncate r to source image */
    if( r.x < 0 )
    {
        r.width += r.x;
        r.x = 0;
    }
    if( r.y < 0 )
    {
        r.height += r.y;
        r.y = 0;
    }

    if( r.x + r.width > img->width )
        r.width = img->width - r.x;

    if( r.y + r.height > img->height )
        r.height = img->height - r.y;
    
    cvSetImageROI( img, r );
    CopyOf( img, desired_color );

    cvReleaseImage( &img );
    return true;
}


bool  CvvImage::Save( const char* filename )
{
    if( !m_img )
        return false;
    cvSaveImage( filename, m_img );
    return true;
}


void  CvvImage::Show( const char* window )
{
    if( m_img )
        cvShowImage( window, m_img );
}


#ifdef WIN32

void  CvvImage::Show( HDC dc, int x, int y, int w, int h, int from_x, int from_y )
{
    if( m_img && m_img->depth == IPL_DEPTH_8U )
    {
        uchar buffer[sizeof(BITMAPINFOHEADER) + 1024];
        BITMAPINFO* bmi = (BITMAPINFO*)buffer;
        int bmp_w = m_img->width, bmp_h = m_img->height;

        FillBitmapInfo( bmi, bmp_w, bmp_h, Bpp(), m_img->origin );

        from_x = MIN( MAX( from_x, 0 ), bmp_w - 1 );
        from_y = MIN( MAX( from_y, 0 ), bmp_h - 1 );

        int sw = MAX( MIN( bmp_w - from_x, w ), 0 );
        int sh = MAX( MIN( bmp_h - from_y, h ), 0 );

        SetDIBitsToDevice(
              dc, x, y, sw, sh, from_x, from_y, from_y, sh,
              m_img->imageData + from_y*m_img->widthStep,
              bmi, DIB_RGB_COLORS );
    }
}


void  CImage::DrawToHDC( HDC hDCDst, RECT* pDstRect ) 
{
    if( pDstRect && m_img && m_img->depth == IPL_DEPTH_8U && m_img->imageData )
    {
        uchar buffer[sizeof(BITMAPINFOHEADER) + 1024];
        BITMAPINFO* bmi = (BITMAPINFO*)buffer;
        int bmp_w = m_img->width, bmp_h = m_img->height;

        CvRect roi = cvGetImageROI( m_img );
        CvRect dst = RectToCvRect( *pDstRect );

        if( roi.width == dst.width && roi.height == dst.height )
        {
            Show( hDCDst, dst.x, dst.y, dst.width, dst.height, roi.x, roi.y );
            return;
        }
    
        if( roi.width > dst.width )
        {
            SetStretchBltMode(
                   hDCDst,           // handle to device context
                   HALFTONE );
        }
        else
        {
            SetStretchBltMode(
                   hDCDst,           // handle to device context
                   COLORONCOLOR );
        }

        FillBitmapInfo( bmi, bmp_w, bmp_h, Bpp(), m_img->origin );

        ::StretchDIBits(
            hDCDst,
            dst.x, dst.y, dst.width, dst.height,
            roi.x, roi.y, roi.width, roi.height,
            m_img->imageData, bmi, DIB_RGB_COLORS, SRCCOPY );
    }
}

#endif

void  CvvImage::Fill( int color )
{
    cvSet( m_img, cvScalar(color&255,(color>>8)&255,(color>>16)&255,(color>>24)&255) );
}

#endif

/* End of file. */
