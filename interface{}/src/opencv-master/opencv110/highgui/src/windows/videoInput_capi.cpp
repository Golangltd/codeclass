// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "videoInput.h"
#include "videoInput_capi.h"

// ----------------------------------------------------------------------------

// Turns off console messages - default is to print messages
void vi_setVerbose(vi_bool_t isVerbose) {
	videoInput::setVerbose(isVerbose != 0);
}

// This allows for multithreaded use of VI (default is single threaded).
// Call this before any videoInput calls. 
// Note if your app has other COM calls then you should set VIs COM usage to match the other COM mode.
void vi_setComMultiThreaded(vi_bool_t isMulti) {
	videoInput::setComMultiThreaded(isMulti != 0);
}

// Enumerate devices.
int vi_listDevices() {
	return videoInput::listDevices(true);
}

const char* vi_getDeviceName(int deviceId) {
	return videoInput::getDeviceName(deviceId);
}

int vi_getDeviceIdFromName(const char* name) {
	return videoInput::getDeviceIDFromName(name);
}

// ----------------------------------------------------------------------------

struct vi_device_t: public videoInput {};

vi_device_t* vi_device_new() {
	return new vi_device_t;
}
void vi_device_delete(vi_device_t* p) {
	delete p;
}

// Choose to use callback based capture - or single threaded.
void vi_device_setUseCallback(vi_device_t* p, vi_bool_t useCallback) {
	p->setUseCallback(useCallback);
}

// Call before setupDevice.
// Directshow will try and get the closest possible framerate to what is requested.
void vi_device_setIdealFramerate(vi_device_t* p, int deviceID, int idealFramerate) {
	p->setIdealFramerate(deviceID, idealFramerate);
}

// Some devices will stop delivering frames after a while - this method
// gives you the option to try and reconnect to a device if videoInput detects
// that a device has stopped delivering frames.
// You MUST CALL isFrameNew every app loop for this to have any effect.
void vi_device_setAutoReconnectOnFreeze(vi_device_t* p,
	int deviceNumber, vi_bool_t doReconnect, int numMissedFramesBeforeReconnect
) {
	p->setAutoReconnectOnFreeze(deviceNumber, doReconnect, numMissedFramesBeforeReconnect);
}

// Choose one of these four to setup your device.
vi_bool_t vi_device_setupDevice(vi_device_t* p, int deviceID) {
	return p->setupDevice(deviceID);
}
vi_bool_t vi_device_setupDeviceAndSize(vi_device_t* p, int deviceID, int w, int h) {
	return p->setupDevice(deviceID, w, h);
}

// These two are only for capture cards.
// USB and Firewire cameras souldn't specify connection.
vi_bool_t vi_device_setupDeviceCaptureCards(vi_device_t* p,
	int deviceID, int connection
) {
	return p->setupDevice(deviceID, connection);
	
}
vi_bool_t vi_device_setupDeviceCaptureCardsAndSize(vi_device_t* p,
	int deviceID, int w, int h, int connection
) {
	return p->setupDevice(deviceID, w, h, connection);
}

// If you need to you can set your NTSC/PAL/SECAM
// preference here. if it is available it will be used.
// see #defines above for available formats - eg VI_NTSC_M or VI_PAL_B
// should be called after setupDevice
// can be called multiple times
vi_bool_t vi_device_setFormat(vi_device_t* p, int deviceNumber, int format) {
	return p->setFormat(deviceNumber, format);
}
void vi_device_setRequestedMediaSubType(vi_device_t* p, int mediatype){ // added by gameover
	p->setRequestedMediaSubType(mediatype);
}

// Tells you when a new frame has arrived - you should call this if you have
// specified setAutoReconnectOnFreeze to true
vi_bool_t vi_device_isFrameNew(vi_device_t* p, int deviceID) {
	return p->isFrameNew(deviceID);
}

vi_bool_t vi_device_isDeviceSetup(vi_device_t* p, int deviceID) {
	return p->isDeviceSetup(deviceID);
}

// Returns the pixels - flipRedAndBlue toggles RGB/BGR flipping - and you can flip the image too
uint8_t* vi_device_getPixels(vi_device_t* p,
	int deviceID,
	vi_bool_t flipRedAndBlue,
	vi_bool_t flipImage
) {
	return p->getPixels(deviceID, bool(flipRedAndBlue), bool(flipImage));
}

// Or pass in a buffer for getPixels to fill returns true if successful.
vi_bool_t vi_device_getPixelsWithBuffer(vi_device_t* p,
	int id, uint8_t* pixelsBuffer,
	vi_bool_t flipRedAndBlue,
	vi_bool_t flipImage
) {
	return p->getPixels(
		id, (unsigned char*)pixelsBuffer,
		bool(flipRedAndBlue),
		bool(flipImage)
	);
}

// Launches a pop up settings window
// For some reason in GLUT you have to call it twice each time.
void vi_device_showSettingsWindow(vi_device_t* p, int deviceID, vi_bool_t waitFor) {
	p->showSettingsWindow(deviceID, waitFor);
}

// Manual control over settings thanks.....
// These are experimental for now.
vi_bool_t vi_device_setVideoSettingFilter(vi_device_t* p,
	int deviceID,
	long Property, long lValue, long Flags,
	vi_bool_t useDefaultValue
) {
	return p->setVideoSettingFilter(
		deviceID, Property, lValue, Flags, bool(useDefaultValue)
	);
}
vi_bool_t vi_device_setVideoSettingFilterPct(vi_device_t* p,
	int deviceID, long Property, float pctValue, long Flags
) {
	return p->setVideoSettingFilterPct(deviceID, Property, pctValue, Flags);
}
vi_bool_t vi_device_getVideoSettingFilter(vi_device_t* p,
	int deviceID, long Property,
	long* _min, long* _max,
	long* _SteppingDelta, long* _currentValue, long* _flags,
	long* _defaultValue
) {
	long min, max;
	long SteppingDelta, currentValue, flags;
	long defaultValue;
	bool rv = p->getVideoSettingFilter(
		deviceID, Property,
		min, max,
		SteppingDelta, currentValue, flags,
		defaultValue
	);
	if(_min) *_min = min;
	if(_max) *_max = max;

	if(_SteppingDelta) *_SteppingDelta = SteppingDelta;
	if(_currentValue) *_currentValue = currentValue;
	if(_flags) *_flags = flags;

	if(_defaultValue) *_defaultValue = defaultValue;

	return rv;
}

vi_bool_t vi_device_setVideoSettingCamera(vi_device_t* p,
	int deviceID,
	long Property, long lValue, long Flags,
	vi_bool_t useDefaultValue
) {
	return p->setVideoSettingCamera(
		deviceID, Property, lValue, Flags,
		useDefaultValue
	);
}
vi_bool_t vi_device_setVideoSettingCameraPct(vi_device_t* p,
	int deviceID,
	long Property, float pctValue, long Flags
) {
	return p->setVideoSettingCameraPct(
		deviceID, Property, pctValue, Flags
	);
}
vi_bool_t vi_device_getVideoSettingCamera(vi_device_t* p,
	int deviceID, long Property,
	long* _min, long* _max,
	long* _SteppingDelta, long* _currentValue,
	long* _flags,
	long* _defaultValue
) {
	long min, max;
	long SteppingDelta, currentValue;
	long flags;
	long defaultValue;
	bool rv = p->getVideoSettingCamera(
		deviceID, Property,
		min, max,
		SteppingDelta, currentValue,
		flags,
		defaultValue
	);

	if(_min) *_min = min;
	if(_max) *_max = max;

	if(_SteppingDelta) *_SteppingDelta = SteppingDelta;
	if(_currentValue) *_currentValue = currentValue;

	if(_flags) *_flags = flags;
	if(_defaultValue) *_defaultValue = defaultValue;

	return rv;
}

// Get width, height and number of pixels
int vi_device_getWidth(vi_device_t* p, int deviceID) {
	return p->getWidth(deviceID);
}
int vi_device_getHeight(vi_device_t* p, int deviceID) {
	return p->getHeight(deviceID);
}
int vi_device_getSize(vi_device_t* p, int deviceID) {
	return p->getSize(deviceID);
}

// Completely stops and frees a device
void vi_device_stopDevice(vi_device_t* p, int deviceID) {
	p->stopDevice(deviceID);
}

//as above but then sets it up with same settings
vi_bool_t vi_device_restartDevice(vi_device_t* p, int deviceID) {
	return p->restartDevice(deviceID);
}

// ----------------------------------------------------------------------------

