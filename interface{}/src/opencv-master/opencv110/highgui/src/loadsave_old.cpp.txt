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
//  Loading and saving IPL images.
//

#include "_highgui.h"
#include "grfmts.h"

#if 0
/****************************************************************************************\
*                              Path class (list of search folders)                       *
\****************************************************************************************/

class  CvFilePath
{
public:
    CvFilePath();
    ~CvFilePath();

    // preprocess folder or file name - calculate its length,
    // check for invalid symbols in the name and substitute
    // all backslashes with simple slashes.
    // the result is put into the specified buffer
    static int Preprocess( const char* filename, char* buffer );

    // add folder to the path
    bool Add( const char* path );

    // clear the path
    void Clear();

    // return the path - string, where folders are separated by ';'
    const char* Get() const { return m_path; };

    // find the file in the path
    const char* Find( const char* filename, char* buffer ) const;

    // return the first folder from the path
    // the returned string is not terminated by '\0'!!!
    // its length is returned via len parameter
    const char* First( int& len ) const;

    // return the folder, next in the path after the specified folder.
    // see also note to First() method
    const char* Next( const char* folder, int& len ) const;

protected:

    char* m_path;
    int m_maxsize;
    int m_len;
};


void CvFilePath::Clear()
{
    delete[] m_path;
    m_maxsize = m_len = 0;
}


CvFilePath::CvFilePath()
{
    m_path = 0;
    m_maxsize = m_len = 0;
}


CvFilePath::~CvFilePath()
{
    Clear();
}


bool  CvFilePath::Add( const char* path )
{
    char buffer[_MAX_PATH + 1];
    int len = Preprocess( path, buffer );

    if( len < 0 )
        return false;

    if( m_len + len + 3 // +1 for one more ';',
                        // another +1 for possible additional '/',
                        // and the last +1 is for '\0'
                      > m_maxsize )
    {
        int new_size = (m_len + len + 3 + 1023) & -1024;
        char* new_path = new char[new_size];

        if( m_path )
        {
            memcpy( new_path, m_path, m_len );
            delete[] m_path;
        }

        m_path = new_path;
        m_maxsize = new_size;
    }

    m_path[m_len++] = ';';
    memcpy( m_path + m_len, buffer, len );
    m_len += len;

    if( m_path[m_len] != '/' )
        m_path[m_len++] = '/';

    m_path[m_len] = '\0'; // '\0' is not counted in m_len.

    return true;
}


const char* CvFilePath::First( int& len ) const
{
    const char* path = (const char*)(m_path ? m_path : "");
    const char* path_end = path;

    while( *path_end && *path_end != ';' )
        path_end++;

    len = path_end - path;
    return path;
}


const char* CvFilePath::Next( const char* folder, int& len ) const
{
    if( !folder || folder < m_path || folder >= m_path + m_len )
        return 0;

    folder = strchr( folder, ';' );
    if( folder )
    {
        const char* folder_end = ++folder;
        while( *folder_end && *folder_end != ';' )
            folder_end++;

        len = folder_end - folder;
    }

    return folder;
}


const char* CvFilePath::Find( const char* filename, char* buffer ) const
{
    char path0[_MAX_PATH + 1];
    int len = Preprocess( filename, path0 );
    int folder_len = 0;
    const char* folder = First( folder_len );
    char* name_only = 0;
    char* name = path0;
    FILE* f = 0;

    if( len < 0 )
        return 0;

    do
    {
        if( folder_len + len <= _MAX_PATH )
        {
            memcpy( buffer, folder, folder_len );
            strcpy( buffer + folder_len, name );

            f = fopen( buffer, "rb" );
            if( f )
                break;
        }

        if( name != name_only )
        {
            name_only = strrchr( path0, '/' );
            if( !name_only )
                name_only = path0;
            else
                name_only++;
            len = strlen( name_only );
            name = name_only;
        }
    }
    while( (folder = Next( folder, folder_len )) != 0 );

    filename = 0;

    if( f )
    {
        filename = (const char*)buffer;
        fclose(f);
    }

    return filename;
}


int CvFilePath::Preprocess( const char* str, char* buffer )
{
    int i;

    if( !str || !buffer )
        return -1;

    for( i = 0; i <= _MAX_PATH; i++ )
    {
        buffer[i] = str[i];

        if( isalnum(str[i])) // fast check to skip most of characters
            continue;

        if( str[i] == '\0' )
            break;

        if( str[i] == '\\' ) // convert back slashes to simple slashes
                             // (for Win32-*NIX compatibility)
            buffer[i] = '/';

        if (str[i] == '*' || str[i] == '?' || str[i] == '\"' ||
            str[i] == '>' || str[i] == '<' ||
            str[i] == ';' || /* used as a separator in the path */
        #ifndef WIN32
            str[i] == ',' || str[i] == '%' ||
        #endif
            str[i] == '|')
            return -1;
    }

    return i <= _MAX_PATH ? i : -1;
}
#endif

/****************************************************************************************\
*                              Image Readers & Writers Class                             *
\****************************************************************************************/

class  CvImageFilters
{
public:

    CvImageFilters();
    ~CvImageFilters();

    GrFmtReader* FindReader( const char* filename ) const;
    GrFmtWriter* FindWriter( const char* filename ) const;

    //const CvFilePath& Path() const { return (const CvFilePath&)m_path; };
    //CvFilePath& Path() { return m_path; };

protected:

    GrFmtFactoriesList*  m_factories;
};


CvImageFilters::CvImageFilters()
{
    m_factories = new GrFmtFactoriesList;

#ifdef HAVE_IMAGEIO
    m_factories->AddFactory( new GrFmtImageIO() );
#endif
    m_factories->AddFactory( new GrFmtBmp() );
    m_factories->AddFactory( new GrFmtJpeg() );
    m_factories->AddFactory( new GrFmtSunRaster() );
    m_factories->AddFactory( new GrFmtPxM() );
    m_factories->AddFactory( new GrFmtTiff() );
#ifdef HAVE_PNG
    m_factories->AddFactory( new GrFmtPng() );
#endif
#ifdef HAVE_JASPER
    m_factories->AddFactory( new GrFmtJpeg2000() );
#endif
#ifdef HAVE_ILMIMF
    m_factories->AddFactory( new GrFmtExr() );
#endif
}


CvImageFilters::~CvImageFilters()
{
    delete m_factories;
}


GrFmtReader* CvImageFilters::FindReader( const char* filename ) const
{
    return m_factories->FindReader( filename );
}


GrFmtWriter* CvImageFilters::FindWriter( const char* filename ) const
{
    return m_factories->FindWriter( filename );
}

/****************************************************************************************\
*                         HighGUI loading & saving function implementation               *
\****************************************************************************************/

static int icvSetCXCOREBindings(void)
{
    return CV_SET_IMAGE_IO_FUNCTIONS();
}

int cxcore_bindings_initialized = icvSetCXCOREBindings();

// global image I/O filters
static CvImageFilters  g_Filters;

#if 0
CV_IMPL void
cvAddSearchPath( const char* path )
{
    CV_FUNCNAME( "cvAddSearchPath" );

    __BEGIN__;

    if( !path || strlen(path) == 0 )
        CV_ERROR( CV_StsNullPtr, "Null path" );

    g_Filters.AddPath( path );

    __END__;
}
#endif

CV_IMPL int
cvHaveImageReader( const char* filename )
{
    GrFmtReader* reader = g_Filters.FindReader( filename );
    if( reader ) {
        delete reader;
        return 1;
    }
    return 0;
}

CV_IMPL int cvHaveImageWriter( const char* filename )
{
    GrFmtWriter* writer = g_Filters.FindWriter( filename );
    if( writer ) {
        delete writer;
        return 1;
    }
    return 0;
}

static void*
icvLoadImage( const char* filename, int flags, bool load_as_matrix )
{
    GrFmtReader* reader = 0;
    IplImage* image = 0;
    CvMat hdr, *matrix = 0;
    int depth = 8;

    CV_FUNCNAME( "cvLoadImage" );

    __BEGIN__;

    CvSize size;
    int iscolor;
    int cn;

    if( !filename || strlen(filename) == 0 )
        CV_ERROR( CV_StsNullPtr, "null filename" );

    reader = g_Filters.FindReader( filename );
    if( !reader )
        EXIT;

    if( !reader->ReadHeader() )
        EXIT;

    size.width = reader->GetWidth();
    size.height = reader->GetHeight();

    if( flags == -1 )
        iscolor = reader->IsColor();
    else
    {
        if( (flags & CV_LOAD_IMAGE_COLOR) != 0 ||
           ((flags & CV_LOAD_IMAGE_ANYCOLOR) != 0 && reader->IsColor()) )
            iscolor = 1;
        else
            iscolor = 0;

        if( (flags & CV_LOAD_IMAGE_ANYDEPTH) != 0 )
        {
            reader->UseNativeDepth(true);
            depth = reader->GetDepth();
        }
    }

    cn = iscolor ? 3 : 1;

    if( load_as_matrix )
    {
        int type;
        if(reader->IsFloat() && depth != 8)
            type = CV_32F;
        else
            type = ( depth <= 8 ) ? CV_8U : ( depth <= 16 ) ? CV_16U : CV_32S;
        CV_CALL( matrix = cvCreateMat( size.height, size.width, CV_MAKETYPE(type, cn) ));
    }
    else
    {
        int type;
        if(reader->IsFloat() && depth != 8)
            type = IPL_DEPTH_32F;
        else
            type = ( depth <= 8 ) ? IPL_DEPTH_8U : ( depth <= 16 ) ? IPL_DEPTH_16U : IPL_DEPTH_32S;
        CV_CALL( image = cvCreateImage( size, type, cn ));
        matrix = cvGetMat( image, &hdr );
    }

    if( !reader->ReadData( matrix->data.ptr, matrix->step, iscolor ))
    {
        if( load_as_matrix )
            cvReleaseMat( &matrix );
        else
            cvReleaseImage( &image );
        EXIT;
    }

    __END__;

    delete reader;

    if( cvGetErrStatus() < 0 )
    {
        if( load_as_matrix )
            cvReleaseMat( &matrix );
        else
            cvReleaseImage( &image );
    }

    return load_as_matrix ? (void*)matrix : (void*)image;
}


CV_IMPL IplImage*
cvLoadImage( const char* filename, int iscolor )
{
    return (IplImage*)icvLoadImage( filename, iscolor, false );
}

CV_IMPL CvMat*
cvLoadImageM( const char* filename, int iscolor )
{
    return (CvMat*)icvLoadImage( filename, iscolor, true );
}


CV_IMPL int
cvSaveImage( const char* filename, const CvArr* arr )
{
    int origin = 0;
    GrFmtWriter* writer = 0;
    CvMat *temp = 0, *temp2 = 0;

    CV_FUNCNAME( "cvSaveImage" );

    __BEGIN__;

    CvMat stub, *image;
    int channels, ipl_depth;

    if( !filename || strlen(filename) == 0 )
        CV_ERROR( CV_StsNullPtr, "null filename" );

    CV_CALL( image = cvGetMat( arr, &stub ));

    if( CV_IS_IMAGE( arr ))
        origin = ((IplImage*)arr)->origin;

    channels = CV_MAT_CN( image->type );
    if( channels != 1 && channels != 3 && channels != 4 )
        CV_ERROR( CV_BadNumChannels, "" );

    writer = g_Filters.FindWriter( filename );
    if( !writer )
        CV_ERROR( CV_StsError, "could not find a filter for the specified extension" );

    if( origin )
    {
        CV_CALL( temp = cvCreateMat(image->rows, image->cols, image->type) );
        CV_CALL( cvFlip( image, temp, 0 ));
        image = temp;
    }

    ipl_depth = cvCvToIplDepth(image->type);

    if( !writer->IsFormatSupported(ipl_depth) )
    {
        assert( writer->IsFormatSupported(IPL_DEPTH_8U) );
        CV_CALL( temp2 = cvCreateMat(image->rows,
            image->cols, CV_MAKETYPE(CV_8U,channels)) );
        CV_CALL( cvConvertImage( image, temp2 ));
        image = temp2;
        ipl_depth = IPL_DEPTH_8U;
    }

    if( !writer->WriteImage( image->data.ptr, image->step, image->width,
                             image->height, ipl_depth, channels ))
        CV_ERROR( CV_StsError, "could not save the image" );

    __END__;

    delete writer;
    cvReleaseMat( &temp );
    cvReleaseMat( &temp2 );

    return cvGetErrStatus() >= 0;
}

/* End of file. */
