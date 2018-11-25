// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "_highgui.h"

#include "jpge.h"
#include "jpgd.h"
#include "lodepng.h"
#include "EasyBMP.h"

#include <string>

// ----------------------------------------------------------------------------

static std::string icvLowerString(const std::string& str) {
	std::string s = str;
	for(std::string::iterator i = s.begin(); i != s.end(); ++i) {
		if ('A' <= *i && *i <= 'Z') *i += 'a' - 'A';
	}
	return s;
}

static bool icvHasPrefixString(const std::string& str, const std::string& prefix)
{
	return str.size() >= prefix.size() &&
		str.compare(0, prefix.size(), prefix) == 0;
}

static bool icvHasSuffixString(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() &&
		str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}


static bool icvLoadFileData(const char* name, std::string* data) {
	FILE* fp = fopen(name, "rb");
	if(!fp) return false;

	fseek(fp, 0, SEEK_END);
	data->resize(ftell(fp));

	rewind(fp);
	int n = fread((void*)data->data(), 1, data->size(), fp);
	fclose(fp);
	return (n == data->size());
}

static bool icvSaveFileData(const char* name, const char* data, int size) {
	FILE* fp = fopen(name, "wb");
	if(!fp) return false;

	int n = fwrite((void*)data, 1, size, fp);
	fclose(fp);
	return (n == size);
	
}

// ----------------------------------------------------------------------------

static IplImage* icvDecodeJpg(const std::string& data, int iscolor)
{
	int channels = iscolor? 3: 1;
	int width, height, actual_comps;
	unsigned char * d = jpgd::decompress_jpeg_image_from_memory(
		(const unsigned char*)data.data(), data.size(),
		&width, &height, &actual_comps, channels
	);
	if(d == NULL) {
		return NULL;
	}

	IplImage* m = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, channels);
	memcpy(m->imageData, d, width*height*channels);
	m->widthStep = width*channels;
	free(d);

	cvConvertImage(m, m, CV_CVTIMG_SWAP_RB);
	return m;
}

static bool icvEncodeJpg(
	std::string* dst, const char* data, int size,
	int width, int height, int channels, int quality /* =90 */,
	int width_step /* =0 */
) {
	if(dst == NULL || data == NULL || size <= 0) {
		return false;
	}
	if(width <= 0 || height <= 0) {
		return false;
	}
	if(channels != 1 && channels != 3) {
		return false;
	}
	if(quality <= 0 || quality > 100) {
		return false;
	}

	if(width_step < width*channels) {
		width_step = width*channels;
	}

	std::string tmp;
	const char* pSrcData = data;

	jpge::params comp_params;
	if(channels == 3) {
		comp_params.m_subsampling = jpge::H2V2;   // RGB
		comp_params.m_quality = quality;

		if(width_step > width*channels) {
			tmp.resize(width*height*3);
			for(int i = 0; i < height; ++i) {
				char* ppTmp = (char*)tmp.data() + i*width*channels;
				char* ppSrc = (char*)data + i*width_step;
				memcpy(ppTmp, ppSrc, width*channels);
			}
			pSrcData = tmp.data();
		}
	} else {
		comp_params.m_subsampling = jpge::Y_ONLY; // Gray
		comp_params.m_quality = quality;

		tmp.resize(width*height*3);
		for(int i = 0; i < height; ++i) {
			char* ppTmp = (char*)tmp.data() + i*width*3;
			char* ppSrc = (char*)data + i*width_step;
			for(int j = 0; j < width; ++j) {
				ppTmp[j*3+0] = ppSrc[j];
				ppTmp[j*3+1] = ppSrc[j];
				ppTmp[j*3+2] = ppSrc[j];
			}
		}
		channels = 3;
		pSrcData = tmp.data();
	}

	int buf_size = ((width*height*3)>1024)? (width*height*3): 1024;
	dst->resize(buf_size);
	bool rv = compress_image_to_jpeg_file_in_memory(
		(void*)dst->data(), buf_size, width, height, channels,
		(const jpge::uint8*)pSrcData,
		comp_params
	);
	if(!rv) {
		dst->clear();
		return false;
	}

	dst->resize(buf_size);
	return true;
}

// ----------------------------------------------------------------------------

static bool icvDecodePng32(std::string* dst, const char* data, int size, int* width, int* height) {
	if(dst == NULL || data == NULL || size <= 0) {
		return false;
	}
	if(width == NULL || height == NULL) {
		return false;
	}

	unsigned char* img;
	unsigned w, h;

	unsigned err = lodepng_decode32(&img, &w, &h, (const unsigned char*)data, size);
	if(err != 0) return false;

	dst->assign((const char*)img, w*h*4);
	free(img);

	*width = w;
	*height = h;
	return true;
}

static bool icvDecodePng24(std::string* dst, const char* data, int size, int* width, int* height) {
	if(dst == NULL || data == NULL || size <= 0) {
		return false;
	}
	if(width == NULL || height == NULL) {
		return false;
	}

	unsigned char* img;
	unsigned w, h;

	unsigned err = lodepng_decode32(&img, &w, &h, (const unsigned char*)data, size);
	if(err != 0) return false;

	dst->assign((const char*)img, w*h*3);
	free(img);

	*width = w;
	*height = h;
	return true;
}

static bool icvEncodePng32(std::string* dst, const char* data, int size, int width, int height, int width_step/*=0*/) {
	if(dst == NULL || data == NULL || size <= 0) {
		return false;
	}
	if(width <= 0 || height <= 0) {
		return false;
	}

	if(width_step < width*4) {
		width_step = width*4;
	}

	std::string tmp;
	const char* pSrcData = data;

	if(width_step > width*4) {
		tmp.resize(width*height*4);
		for(int i = 0; i < height; ++i) {
			char* ppTmp = (char*)tmp.data() + i*width*4;
			char* ppSrc = (char*)data + i*width_step;
			memcpy(ppTmp, ppSrc, width*4);
		}
		pSrcData = tmp.data();
	}

	unsigned char* png;
	size_t pngsize;

	unsigned err = lodepng_encode32(&png, &pngsize, (const unsigned char*)pSrcData, width, height);
	if(err != 0) return false;

	dst->assign((const char*)png, pngsize);
	free(png);

	return true;
}

static bool icvEncodePng24(std::string* dst, const char* data, int size, int width, int height, int width_step/*=0*/) {
	if(dst == NULL || data == NULL || size <= 0) {
		return false;
	}
	if(width <= 0 || height <= 0) {
		return false;
	}

	if(width_step < width*3) {
		width_step = width*3;
	}

	std::string tmp;
	const char* pSrcData = data;

	if(width_step > width*3) {
		tmp.resize(width*height*3);
		for(int i = 0; i < height; ++i) {
			char* ppTmp = (char*)tmp.data() + i*width*3;
			char* ppSrc = (char*)data + i*width_step;
			memcpy(ppTmp, ppSrc, width*3);
		}
		pSrcData = tmp.data();
	}

	unsigned char* png;
	size_t pngsize;

	unsigned err = lodepng_encode24(&png, &pngsize, (const unsigned char*)pSrcData, width, height);
	if(err != 0) return false;

	dst->assign((const char*)png, pngsize);
	free(png);

	return true;
}

// ----------------------------------------------------------------------------

CV_IMPL
int cvHaveImageReader( const char* filename )
{
	std::string name = icvLowerString(filename);

	if(icvHasSuffixString(name, ".jpg")) return 1;
	if(icvHasSuffixString(name, ".jpeg")) return 1;
	if(icvHasSuffixString(name, ".png")) return 1;
	if(icvHasSuffixString(name, ".bmp")) return 0; // todo

	return 0;
}

CV_IMPL
int cvHaveImageWriter( const char* filename )
{
	std::string name = icvLowerString(filename);

	if(icvHasSuffixString(name, ".jpg")) return 1;
	if(icvHasSuffixString(name, ".jpeg")) return 1;
	if(icvHasSuffixString(name, ".png")) return 1;
	if(icvHasSuffixString(name, ".bmp")) return 0; // todo

	return 0;
}

CV_IMPL
IplImage* cvLoadImage( const char* filename, int iscolor )
{
	if(!filename || !filename[0]) {
		return NULL;
	}

	std::string data;
	if(!icvLoadFileData(filename, &data)) {
		return NULL;
	}

	std::string lowerName = icvLowerString(filename);
	if(icvHasSuffixString(lowerName, ".jpg")) {
		return icvDecodeJpg(data, iscolor);
	}
	if(icvHasSuffixString(lowerName, ".jpeg")) {
		return icvDecodeJpg(data, iscolor);
	}

	return NULL;
}

CV_IMPL
CvMat* cvLoadImageM( const char* filename, int iscolor )
{
	if(!filename || !filename[0]) {
		return NULL;
	}
	return NULL;//cvLoadImageM_goproxy(filename, iscolor);
}

CV_IMPL
int cvSaveImage( const char* filename, const CvArr* arr )
{
	if(!filename || !filename[0] || !arr) {
		return 0;
	}

    if(!CV_IS_IMAGE(arr)) {
		return 0;
	}
	IplImage* img = (IplImage*)arr;
	std::string data;

	if(img->depth != IPL_DEPTH_8U) {
		return 0;
	}

	std::string lowerName = icvLowerString(filename);
	if(icvHasSuffixString(lowerName, ".jpg") || icvHasSuffixString(lowerName, ".jpeg")) {
		cvConvertImage(img, img, CV_CVTIMG_SWAP_RB);
		if(!icvEncodeJpg(&data,
			img->imageData, img->imageSize,
			img->width, img->height, img->nChannels,
			90, img->widthStep
		)) {
			cvConvertImage(img, img, CV_CVTIMG_SWAP_RB);
			return 0;
		}
		cvConvertImage(img, img, CV_CVTIMG_SWAP_RB);
	}

	if(!icvSaveFileData(filename, data.data(), data.size())) {
		return 0;
	}

	return 1;
}

// ----------------------------------------------------------------------------

