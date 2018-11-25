// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef VIDEO_INPUT_CAPI_H_
#define VIDEO_INPUT_CAPI_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------

typedef int vi_bool_t;

// videoInput defines
#define VI_VERSION   0.200
#define VI_MAX_CAMERAS  20
#define VI_NUM_TYPES    19 //DON'T TOUCH
#define VI_NUM_FORMATS  18 //DON'T TOUCH

// defines for setPhyCon - tuner is not as well supported as composite and s-video
#define VI_COMPOSITE 0
#define VI_S_VIDEO   1
#define VI_TUNER     2
#define VI_USB       3
#define VI_1394      4

// defines for formats
#define VI_NTSC_M    0
#define VI_PAL_B     1
#define VI_PAL_D     2
#define VI_PAL_G     3
#define VI_PAL_H     4
#define VI_PAL_I     5
#define VI_PAL_M     6
#define VI_PAL_N     7
#define VI_PAL_NC    8
#define VI_SECAM_B   9
#define VI_SECAM_D  10
#define VI_SECAM_G  11
#define VI_SECAM_H  12
#define VI_SECAM_K  13
#define VI_SECAM_K1 14
#define VI_SECAM_L  15
#define VI_NTSC_M_J 16
#define VI_NTSC_433 17

// added by gameover
#define VI_MEDIASUBTYPE_RGB24   0
#define VI_MEDIASUBTYPE_RGB32   1
#define VI_MEDIASUBTYPE_RGB555  2
#define VI_MEDIASUBTYPE_RGB565  3
#define VI_MEDIASUBTYPE_YUY2    4
#define VI_MEDIASUBTYPE_YVYU    5
#define VI_MEDIASUBTYPE_YUYV    6
#define VI_MEDIASUBTYPE_IYUV    7
#define VI_MEDIASUBTYPE_UYVY    8
#define VI_MEDIASUBTYPE_YV12    9
#define VI_MEDIASUBTYPE_YVU9    10
#define VI_MEDIASUBTYPE_Y411    11
#define VI_MEDIASUBTYPE_Y41P    12
#define VI_MEDIASUBTYPE_Y211    13
#define VI_MEDIASUBTYPE_AYUV    14
#define VI_MEDIASUBTYPE_Y800    15
#define VI_MEDIASUBTYPE_Y8      16
#define VI_MEDIASUBTYPE_GREY    17
#define VI_MEDIASUBTYPE_MJPG    18

// ----------------------------------------------------------------------------

// Turns off console messages - default is to print messages
void vi_setVerbose(vi_bool_t isVerbose);

// This allows for multithreaded use of VI (default is single threaded).
// Call this before any videoInput calls. 
// Note if your app has other COM calls then you should set VIs COM usage to match the other COM mode.
void vi_setComMultiThreaded(vi_bool_t isMulti);

// Enumerate devices.
int vi_listDevices();
const char* vi_getDeviceName(int deviceId);
int vi_getDeviceIdFromName(const char* name);

// ----------------------------------------------------------------------------

typedef struct vi_device_t vi_device_t;

vi_device_t* vi_device_new();
void vi_device_delete(vi_device_t* p);

// Choose to use callback based capture - or single threaded.
void vi_device_setUseCallback(vi_device_t* p, vi_bool_t useCallback);

// Call before setupDevice.
// Directshow will try and get the closest possible framerate to what is requested.
void vi_device_setIdealFramerate(vi_device_t* p, int deviceID, int idealFramerate);

// Some devices will stop delivering frames after a while - this method
// gives you the option to try and reconnect to a device if videoInput detects
// that a device has stopped delivering frames.
// You MUST CALL isFrameNew every app loop for this to have any effect.
void vi_device_setAutoReconnectOnFreeze(vi_device_t* p,
	int deviceNumber, vi_bool_t doReconnect, int numMissedFramesBeforeReconnect
);

// Choose one of these four to setup your device.
vi_bool_t vi_device_setupDevice(vi_device_t* p, int deviceID);
vi_bool_t vi_device_setupDeviceAndSize(vi_device_t* p, int deviceID, int w, int h);

// These two are only for capture cards.
// USB and Firewire cameras souldn't specify connection.
vi_bool_t vi_device_setupDeviceCaptureCards(vi_device_t* p,
	int deviceID, int connection
);
vi_bool_t vi_device_setupDeviceCaptureCardsAndSize(vi_device_t* p,
	int deviceID, int w, int h, int connection
);

// If you need to you can set your NTSC/PAL/SECAM
// preference here. if it is available it will be used.
// see #defines above for available formats - eg VI_NTSC_M or VI_PAL_B
// should be called after setupDevice
// can be called multiple times
vi_bool_t vi_device_setFormat(vi_device_t* p, int deviceNumber, int format);
void vi_device_setRequestedMediaSubType(vi_device_t* p, int mediatype); // added by gameover

// Tells you when a new frame has arrived - you should call this if you have
// specified setAutoReconnectOnFreeze to true
vi_bool_t vi_device_isFrameNew(vi_device_t* p, int deviceID);

vi_bool_t vi_device_isDeviceSetup(vi_device_t* p, int deviceID);

// Returns the pixels - flipRedAndBlue toggles RGB/BGR flipping - and you can flip the image too
uint8_t* vi_device_getPixels(vi_device_t* p,
	int deviceID,
	vi_bool_t flipRedAndBlue,
	vi_bool_t flipImage
);

// Or pass in a buffer for getPixels to fill returns true if successful.
vi_bool_t vi_device_getPixelsWithBuffer(vi_device_t* p,
	int id, uint8_t* pixelsBuffer,
	vi_bool_t flipRedAndBlue,
	vi_bool_t flipImage
);

// Launches a pop up settings window
// For some reason in GLUT you have to call it twice each time.
void vi_device_showSettingsWindow(vi_device_t* p, int deviceID, vi_bool_t waitFor);

// Manual control over settings thanks.....
// These are experimental for now.
vi_bool_t vi_device_setVideoSettingFilter(vi_device_t* p,
	int deviceID,
	long Property, long lValue, long Flags,
	vi_bool_t useDefaultValue
);
vi_bool_t vi_device_setVideoSettingFilterPct(vi_device_t* p,
	int deviceID, long Property, float pctValue, long Flags
);
vi_bool_t vi_device_getVideoSettingFilter(vi_device_t* p,
	int deviceID, long Property,
	long* min, long* max,
	long* SteppingDelta, long* currentValue, long* flags,
	long* defaultValue
);

vi_bool_t vi_device_setVideoSettingCamera(vi_device_t* p,
	int deviceID,
	long Property, long lValue, long Flags,
	vi_bool_t useDefaultValue
);
vi_bool_t vi_device_setVideoSettingCameraPct(vi_device_t* p,
	int deviceID,
	long Property, float pctValue, long Flags
);
vi_bool_t vi_device_getVideoSettingCamera(vi_device_t* p,
	int deviceID, long Property,
	long* min, long* max,
	long* SteppingDelta, long* currentValue,
	long* flags,
	long* defaultValue
);

// Get width, height and number of pixels
int vi_device_getWidth(vi_device_t* p, int deviceID);
int vi_device_getHeight(vi_device_t* p, int deviceID);
int vi_device_getSize(vi_device_t* p, int deviceID);

// Completely stops and frees a device
void vi_device_stopDevice(vi_device_t* p, int deviceID);

//as above but then sets it up with same settings
vi_bool_t vi_device_restartDevice(vi_device_t* p, int deviceID);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif	// VIDEO_INPUT_CAPI_H_
