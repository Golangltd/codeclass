// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "_highgui.h"
#include "videoInput.h"

class CvCaptureCAM_DShow : public CvCapture
{
public:
	CvCaptureCAM_DShow();
	virtual ~CvCaptureCAM_DShow();

	virtual bool open( int index );
	virtual void close();
	virtual double getProperty(int);
	virtual bool setProperty(int, double);
	virtual bool grabFrame();
	virtual IplImage* retrieveFrame();
	virtual int getCaptureDomain() { return CV_CAP_DSHOW; } // Return the type of the capture object: CV_CAP_VFW, etc...

protected:
	void init();

	int index, width, height, fourcc;
	IplImage* frame;
	static videoInput VI;
};


struct SuppressVideoInputMessages {
	SuppressVideoInputMessages() {
		videoInput::setVerbose(false);
	}
};

static SuppressVideoInputMessages do_it;
videoInput CvCaptureCAM_DShow::VI;

CvCaptureCAM_DShow::CvCaptureCAM_DShow()
{
	index = -1;
	frame = 0;
	width = height = fourcc = -1;
}

CvCaptureCAM_DShow::~CvCaptureCAM_DShow()
{
	close();
}

void CvCaptureCAM_DShow::close()
{
	if( index >= 0 ) {
		VI.stopDevice(index);
		index = -1;
		cvReleaseImage(&frame);
	}
	width = height = -1;
}

// Initialize camera input
bool CvCaptureCAM_DShow::open( int _index )
{
	int try_index = _index;
	int devices = 0;

	close();
	devices = VI.listDevices(true);
	if (devices == 0) return false;
	try_index = try_index < 0 ? 0 : (try_index > devices-1 ? devices-1 : try_index);

	VI.setupDevice(try_index);
	if( !VI.isDeviceSetup(try_index) ) {
		return false;
	}

	index = try_index;
	return true;
}

bool CvCaptureCAM_DShow::grabFrame()
{
	return true;
}


IplImage* CvCaptureCAM_DShow::retrieveFrame()
{
	if( !frame || VI.getWidth(index) != frame->width || VI.getHeight(index) != frame->height )
	{
		if (frame) {
			cvReleaseImage( &frame );
		}
		int w = VI.getWidth(index), h = VI.getHeight(index);
		frame = cvCreateImage( cvSize(w,h), IPL_DEPTH_8U, 3 );
	}

	VI.getPixels( index, (uchar*)frame->imageData, false, true );
	return frame;
}

double CvCaptureCAM_DShow::getProperty( int property_id )
{

	long min_value,max_value,stepping_delta,current_value,flags,defaultValue;

	// image format proprrties
	switch( property_id ) {
	case CV_CAP_PROP_FRAME_WIDTH:
		return VI.getWidth(index);

	case CV_CAP_PROP_FRAME_HEIGHT:
		return VI.getHeight(index);
	}

	// video filter properties
	//switch( property_id ) {
	//case CV_CAP_PROP_BRIGHTNESS:
	//case CV_CAP_PROP_CONTRAST:
	//case CV_CAP_PROP_HUE:
	//case CV_CAP_PROP_SATURATION:
	//case CV_CAP_PROP_SHARPNESS:
	//case CV_CAP_PROP_GAMMA:
	//case CV_CAP_PROP_MONOCROME:
	//case CV_CAP_PROP_WHITE_BALANCE_BLUE_U:
	//case CV_CAP_PROP_BACKLIGHT:
	//case CV_CAP_PROP_GAIN:
	//	if (VI.getVideoSettingFilter(
	//		index,VI.getVideoPropertyFromCV(property_id),
	//		min_value, max_value, stepping_delta, current_value,
	//		flags, defaultValue
	//	)) {
	//		return (double)current_value;
	//	}
	//}

	// camera properties
	//switch( property_id ) {
	//case CV_CAP_PROP_PAN:
	//case CV_CAP_PROP_TILT:
	//case CV_CAP_PROP_ROLL:
	//case CV_CAP_PROP_ZOOM:
	//case CV_CAP_PROP_EXPOSURE:
	//case CV_CAP_PROP_IRIS:
	//case CV_CAP_PROP_FOCUS:
	//	if (VI.getVideoSettingCamera(
	//		index,VI.getCameraPropertyFromCV(property_id),
	//		min_value, max_value, stepping_delta, current_value,
	//		flags, defaultValue
	//	)) {
	//		return (double)current_value;
	//	}
	//}

	// unknown parameter or value not available
	return -1;
}

bool CvCaptureCAM_DShow::setProperty( int property_id, double value )
{
	// image capture properties
	bool handled = false;
	switch( property_id ) {
	case CV_CAP_PROP_FRAME_WIDTH:
		width = cvRound(value);
		handled = true;
		break;

	case CV_CAP_PROP_FRAME_HEIGHT:
		height = cvRound(value);
		handled = true;
		break;

	case CV_CAP_PROP_FOURCC:
		fourcc = cvRound(value);
		if ( fourcc < 0 ) {
			// following cvCreateVideo usage will pop up caprturepindialog here if fourcc=-1
			// TODO - how to create a capture pin dialog
		}
		handled = true;
		break;
	}

	if ( handled ) {
		// a stream setting
		if( width > 0 && height > 0 ) {
			if( width != VI.getWidth(index) || height != VI.getHeight(index) )//|| fourcc != VI.getFourcc(index) )
			{
				VI.stopDevice(index);
				VI.setupDevice(index, width, height);
			}
			width = height = fourcc = -1;
			return VI.isDeviceSetup(index);
		}
		return true;
	}

	// show video/camera filter dialog
	if ( property_id == CV_CAP_PROP_SETTINGS ) {
		VI.showSettingsWindow(index, true);
		return true;
	}

	//video Filter properties
	//switch( property_id ) {
	//case CV_CAP_PROP_BRIGHTNESS:
	//case CV_CAP_PROP_CONTRAST:
	//case CV_CAP_PROP_HUE:
	//case CV_CAP_PROP_SATURATION:
	//case CV_CAP_PROP_SHARPNESS:
	//case CV_CAP_PROP_GAMMA:
	//case CV_CAP_PROP_MONOCROME:
	//case CV_CAP_PROP_WHITE_BALANCE_BLUE_U:
	//case CV_CAP_PROP_BACKLIGHT:
	//case CV_CAP_PROP_GAIN:
	//	return VI.setVideoSettingFilter(
	//		index, VI.getVideoPropertyFromCV(property_id), (long)value
	//	);
	//}

	//camera properties
	//switch( property_id ) {
	//case CV_CAP_PROP_PAN:
	//case CV_CAP_PROP_TILT:
	//case CV_CAP_PROP_ROLL:
	//case CV_CAP_PROP_ZOOM:
	//case CV_CAP_PROP_EXPOSURE:
	//case CV_CAP_PROP_IRIS:
	//case CV_CAP_PROP_FOCUS:
	//	return VI.setVideoSettingCamera(
	//		index, VI.getCameraPropertyFromCV(property_id), (long)value
	//	);
	//}

	return false;
}


CvCapture* cvCreateCameraCapture_DShow( int index )
{
	CvCaptureCAM_DShow* capture = new CvCaptureCAM_DShow;

	try {
		if( capture->open( index )) return capture;
	} catch(...) {
		delete capture;
		throw;
	}

	delete capture;
	return 0;
}
