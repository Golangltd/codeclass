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
//  C++ classes for image and matrices
//
// */

#include "_cxcore.h"

/////////////////////////////// CvImage implementation //////////////////////////////////

static CvLoadImageFunc load_image = 0;
static CvLoadImageMFunc load_image_m = 0;
static CvSaveImageFunc save_image = 0;
static CvShowImageFunc show_image = 0;

static bool
icvIsXmlOrYaml( const char* filename )
{
    const char* suffix = strrchr( filename, '.' );
    return suffix &&
        (strcmp( suffix, ".xml" ) == 0 ||
        strcmp( suffix, ".Xml" ) == 0 ||
        strcmp( suffix, ".XML" ) == 0 ||
        strcmp( suffix, ".yml" ) == 0 ||
        strcmp( suffix, ".Yml" ) == 0 ||
        strcmp( suffix, ".YML" ) == 0 ||
        strcmp( suffix, ".yaml" ) == 0 ||
        strcmp( suffix, ".Yaml" ) == 0 ||
        strcmp( suffix, ".YAML" ) == 0);
}


static IplImage*
icvRetrieveImage( void* obj )
{
    IplImage* img = 0;

    CV_FUNCNAME( "icvRetrieveImage" );

    __BEGIN__;

    if( CV_IS_IMAGE(obj) )
        img = (IplImage*)obj;
    else if( CV_IS_MAT(obj) )
    {
        CvMat* m = (CvMat*)obj;
        CV_CALL( img = cvCreateImageHeader( cvSize(m->cols,m->rows),
                        CV_MAT_DEPTH(m->type), CV_MAT_CN(m->type) ));
        cvSetData( img, m->data.ptr, m->step );
        img->imageDataOrigin = (char*)m->refcount;
        m->data.ptr = 0; m->step = 0;
        cvReleaseMat( &m );
    }
    else if( obj )
    {
        cvRelease( &obj );
        CV_ERROR( CV_StsUnsupportedFormat, "The object is neither an image, nor a matrix" );
    }

    __END__;

    return img;
}


bool CvImage::load( const char* filename, const char* imgname, int color )
{
    IplImage* img = 0;

    CV_FUNCNAME( "CvImage::read" );

    __BEGIN__;

    if( icvIsXmlOrYaml(filename) )
    {
        img = icvRetrieveImage(cvLoad(filename,0,imgname));
        if( (img->nChannels > 1) != (color == 0) )
            CV_ERROR( CV_StsNotImplemented,
            "RGB<->Grayscale conversion is not implemented for images stored in XML/YAML" );
        /*{
            IplImage* temp_img = 0;
            CV_CALL( temp_img = cvCreateImage( cvGetSize(img), img->depth, color > 0 ? 3 : 1 ));
            cvCvtColor( img, temp_img, color > 0 ? CV_GRAY2BGR : CV_BGR2GRAY );
            cvReleaseImage( &img );
            img = temp_img;
        }*/
    }
    else
    {
        if( load_image )
            img = load_image( filename, color );
        else
            CV_ERROR( CV_StsNotImplemented,
            "Loading an image stored in such a format requires HigGUI.\n"
            "Link it to your program and call any function from it\n" );
    }

    attach( img );

    __END__;

    return img != 0;
}


bool CvImage::read( CvFileStorage* fs, const char* mapname, const char* imgname )
{
    void* obj = 0;
    IplImage* img = 0;

    if( mapname )
    {
        CvFileNode* mapnode = cvGetFileNodeByName( fs, 0, mapname );
        if( !mapnode )
            obj = cvReadByName( fs, mapnode, imgname );
    }
    else
        obj = cvReadByName( fs, 0, imgname );

    img = icvRetrieveImage(obj);
    attach( img );
    return img != 0;
}


bool CvImage::read( CvFileStorage* fs, const char* seqname, int idx )
{
    void* obj = 0;
    IplImage* img = 0;
    CvFileNode* seqnode = seqname ?
        cvGetFileNodeByName( fs, 0, seqname ) : cvGetRootFileNode(fs,0);

    if( seqnode && CV_NODE_IS_SEQ(seqnode->tag) )
        obj = cvRead( fs, (CvFileNode*)cvGetSeqElem( seqnode->data.seq, idx ));
    img = icvRetrieveImage(obj);
    attach( img );
    return img != 0;
}


void CvImage::save( const char* filename, const char* imgname )
{
    CV_FUNCNAME( "CvImage::write" );
    __BEGIN__;

    if( !image )
        return;
    if( icvIsXmlOrYaml( filename ) )
        cvSave( filename, image, imgname );
    else
    {
        if( save_image )
            save_image( filename, image );
        else
            CV_ERROR( CV_StsNotImplemented,
            "Saving an image in such a format requires HigGUI.\n"
            "Link it to your program and call any function from it\n" );
    }

    __END__;
}


void CvImage::write( CvFileStorage* fs, const char* imgname )
{
    if( image )
        cvWrite( fs, imgname, image );
}


void CvImage::show( const char* window_name )
{
    CV_FUNCNAME( "CvMatrix::show" );

    __BEGIN__;

    if( image )
    {
        if( !show_image )
            CV_ERROR( CV_StsNotImplemented,
            "CvImage::show method requires HighGUI.\n"
            "Link it to your program and call any function from it\n" );
        show_image( window_name, image );
    }

    __END__;
}


/////////////////////////////// CvMatrix implementation //////////////////////////////////

CvMatrix::CvMatrix( int rows, int cols, int type, CvMemStorage* storage, bool alloc_data )
{
    if( storage )
    {
        matrix = (CvMat*)cvMemStorageAlloc( storage, sizeof(*matrix) );
        cvInitMatHeader( matrix, rows, cols, type, alloc_data ?
            cvMemStorageAlloc( storage, rows*cols*CV_ELEM_SIZE(type) ) : 0 );
    }
    else
        matrix = 0;
}

static CvMat*
icvRetrieveMatrix( void* obj )
{
    CvMat* m = 0;

    CV_FUNCNAME( "icvRetrieveMatrix" );

    __BEGIN__;

    if( CV_IS_MAT(obj) )
        m = (CvMat*)obj;
    else if( CV_IS_IMAGE(obj) )
    {
        IplImage* img = (IplImage*)obj;
        CvMat hdr, *src = cvGetMat( img, &hdr );
        CV_CALL( m = cvCreateMat( src->rows, src->cols, src->type ));
        CV_CALL( cvCopy( src, m ));
        cvReleaseImage( &img );
    }
    else if( obj )
    {
        cvRelease( &obj );
        CV_ERROR( CV_StsUnsupportedFormat, "The object is neither an image, nor a matrix" );
    }

    __END__;

    return m;
}


bool CvMatrix::load( const char* filename, const char* matname, int color )
{
    CvMat* m = 0;

    CV_FUNCNAME( "CvMatrix::read" );

    __BEGIN__;

    if( icvIsXmlOrYaml(filename) )
    {
        m = icvRetrieveMatrix(cvLoad(filename,0,matname));

        if( (CV_MAT_CN(m->type) > 1) != (color == 0) )
            CV_ERROR( CV_StsNotImplemented,
            "RGB<->Grayscale conversion is not implemented for matrices stored in XML/YAML" );
        /*{
            CvMat* temp_mat;
            CV_CALL( temp_mat = cvCreateMat( m->rows, m->cols,
                CV_MAKETYPE(CV_MAT_DEPTH(m->type), color > 0 ? 3 : 1 )));
            cvCvtColor( m, temp_mat, color > 0 ? CV_GRAY2BGR : CV_BGR2GRAY );
            cvReleaseMat( &m );
            m = temp_mat;
        }*/
    }
    else
    {
        if( load_image_m )
            m = load_image_m( filename, color );
        else
            CV_ERROR( CV_StsNotImplemented,
            "Loading an image stored in such a format requires HigGUI.\n"
            "Link it to your program and call any function from it\n" );
    }

    set( m, false );

    __END__;

    return m != 0;
}


bool CvMatrix::read( CvFileStorage* fs, const char* mapname, const char* matname )
{
    void* obj = 0;
    CvMat* m = 0;

    if( mapname )
    {
        CvFileNode* mapnode = cvGetFileNodeByName( fs, 0, mapname );
        if( !mapnode )
            obj = cvReadByName( fs, mapnode, matname );
    }
    else
        obj = cvReadByName( fs, 0, matname );

    m = icvRetrieveMatrix(obj);
    set( m, false );
    return m != 0;
}


bool CvMatrix::read( CvFileStorage* fs, const char* seqname, int idx )
{
    void* obj = 0;
    CvMat* m = 0;
    CvFileNode* seqnode = seqname ?
        cvGetFileNodeByName( fs, 0, seqname ) : cvGetRootFileNode(fs,0);

    if( seqnode && CV_NODE_IS_SEQ(seqnode->tag) )
        obj = cvRead( fs, (CvFileNode*)cvGetSeqElem( seqnode->data.seq, idx ));
    m = icvRetrieveMatrix(obj);
    set( m, false );
    return m != 0;
}


void CvMatrix::save( const char* filename, const char* matname )
{
    CV_FUNCNAME( "CvMatrix::write" );
    __BEGIN__;

    if( !matrix )
        return;
    if( icvIsXmlOrYaml( filename ) )
        cvSave( filename, matrix, matname );
    else
    {
        if( save_image )
            save_image( filename, matrix );
        else
            CV_ERROR( CV_StsNotImplemented,
            "Saving a matrixe in such a format requires HigGUI.\n"
            "Link it to your program and call any function from it\n" );
    }

    __END__;
}


void CvMatrix::write( CvFileStorage* fs, const char* matname )
{
    if( matrix )
        cvWrite( fs, matname, matrix );
}


void CvMatrix::show( const char* window_name )
{
    CV_FUNCNAME( "CvMatrix::show" );

    __BEGIN__;

    if( matrix )
    {
        if( !show_image )
            CV_ERROR( CV_StsNotImplemented,
            "CvMatrix::show method requires HighGUI.\n"
            "Link it to your program and call any function from it\n" );
        show_image( window_name, matrix );
    }

    __END__;
}


CV_IMPL int
cvSetImageIOFunctions( CvLoadImageFunc _load_image, CvLoadImageMFunc _load_image_m,
                       CvSaveImageFunc _save_image, CvShowImageFunc _show_image )
{
    load_image = _load_image;
    load_image_m = _load_image_m;
    save_image = _save_image;
    show_image = _show_image;
    return 1;
}


/*void main(void)
{
    CvImage a(cvSize(300,200),8,3), b(cvSize(300,200),8,3);
    CvRNG rng = cvRNG(-1);

    CV_SET_IMAGE_IO_FUNCTIONS();

    cvNamedWindow( "test", 1 );
    //cvZero( a );
    cvZero( b );
    cvRandArr( &rng, a, CV_RAND_UNI, cvScalarAll(0), cvScalarAll(100) );
    cvCircle( b, cvPoint(100,100), 70, cvScalar(0,255,0), -1, CV_AA, 0 );
    cvAdd( a, b, a );
    a.show( "test" );

    cvWaitKey();
}*/

/* End of file. */

