// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

/*
#cgo CFLAGS: -I./opencv110/highgui/src/windows

#include "videoInput_capi.h"
*/
import "C"
import (
	"runtime"
	"unsafe"
)

// ----------------------------------------------------------------------------

const (
	VIDEO_INPUT_VERSION     = 0.200
	VIDEO_INPUT_MAX_CAMERAS = 20
)

type VideoInputFormat int

const (
	VIDEO_INPUT_NTSC_M      = VideoInputFormat(C.VI_NTSC_M)      // 0
	VIDEO_INPUT_PAL_B       = VideoInputFormat(C.VI_PAL_B)       // 1
	VIDEO_INPUT_PAL_D       = VideoInputFormat(C.VI_PAL_D)       // 2
	VIDEO_INPUT_PAL_G       = VideoInputFormat(C.VI_PAL_G)       // 3
	VIDEO_INPUT_PAL_H       = VideoInputFormat(C.VI_PAL_H)       // 4
	VIDEO_INPUT_PAL_I       = VideoInputFormat(C.VI_PAL_I)       // 5
	VIDEO_INPUT_PAL_M       = VideoInputFormat(C.VI_PAL_M)       // 6
	VIDEO_INPUT_PAL_N       = VideoInputFormat(C.VI_PAL_N)       // 7
	VIDEO_INPUT_PAL_NC      = VideoInputFormat(C.VI_PAL_NC)      // 8
	VIDEO_INPUT_SECAM_B     = VideoInputFormat(C.VI_SECAM_B)     // 9
	VIDEO_INPUT_SECAM_D     = VideoInputFormat(C.VI_SECAM_D)     // 10
	VIDEO_INPUT_SECAM_G     = VideoInputFormat(C.VI_SECAM_G)     // 11
	VIDEO_INPUT_SECAM_H     = VideoInputFormat(C.VI_SECAM_H)     // 12
	VIDEO_INPUT_SECAM_K     = VideoInputFormat(C.VI_SECAM_K)     // 13
	VIDEO_INPUT_SECAM_K1    = VideoInputFormat(C.VI_SECAM_K1)    // 14
	VIDEO_INPUT_SECAM_L     = VideoInputFormat(C.VI_SECAM_L)     // 15
	VIDEO_INPUT_NTSC_M_J    = VideoInputFormat(C.VI_NTSC_M_J)    // 16
	VIDEO_INPUT_NTSC_433    = VideoInputFormat(C.VI_NTSC_433)    // 17
	VIDEO_INPUT_NUM_FORMATS = VideoInputFormat(C.VI_NUM_FORMATS) // 18, DON'T TOUCH
)

type VideoInputMediaSubType int

const (
	VIDEO_INPUT_MEDIASUBTYPE_RGB24  = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_RGB24)  // 0
	VIDEO_INPUT_MEDIASUBTYPE_RGB32  = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_RGB32)  // 1
	VIDEO_INPUT_MEDIASUBTYPE_RGB555 = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_RGB555) // 2
	VIDEO_INPUT_MEDIASUBTYPE_RGB565 = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_RGB565) // 3
	VIDEO_INPUT_MEDIASUBTYPE_YUY2   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_YUY2)   // 4
	VIDEO_INPUT_MEDIASUBTYPE_YVYU   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_YVYU)   // 5
	VIDEO_INPUT_MEDIASUBTYPE_YUYV   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_YUYV)   // 6
	VIDEO_INPUT_MEDIASUBTYPE_IYUV   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_IYUV)   // 7
	VIDEO_INPUT_MEDIASUBTYPE_UYVY   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_UYVY)   // 8
	VIDEO_INPUT_MEDIASUBTYPE_YV12   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_YV12)   // 9
	VIDEO_INPUT_MEDIASUBTYPE_YVU9   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_YVU9)   // 10
	VIDEO_INPUT_MEDIASUBTYPE_Y411   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_Y411)   // 11
	VIDEO_INPUT_MEDIASUBTYPE_Y41P   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_Y41P)   // 12
	VIDEO_INPUT_MEDIASUBTYPE_Y211   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_Y211)   // 13
	VIDEO_INPUT_MEDIASUBTYPE_AYUV   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_AYUV)   // 14
	VIDEO_INPUT_MEDIASUBTYPE_Y800   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_Y800)   // 15
	VIDEO_INPUT_MEDIASUBTYPE_Y8     = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_Y8)     // 16
	VIDEO_INPUT_MEDIASUBTYPE_GREY   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_GREY)   // 17
	VIDEO_INPUT_MEDIASUBTYPE_MJPG   = VideoInputMediaSubType(C.VI_MEDIASUBTYPE_MJPG)   // 18
	VIDEO_INPUT_NUM_TYPES           = VideoInputMediaSubType(C.VI_NUM_TYPES)           // 19, DON'T TOUCH
)

// ----------------------------------------------------------------------------

type videoInput struct {
	vi *C.vi_device_t
	m  []IplImage
}

func newVideoInput() *videoInput {
	p := &videoInput{
		vi: C.vi_device_new(),
		m:  make([]IplImage, int(C.vi_listDevices())),
	}
	runtime.SetFinalizer(p, (*videoInput).Release)
	return p
}

func (p *videoInput) Release() {
	if p.vi != nil {
		C.vi_device_delete(p.vi)
	}
	runtime.SetFinalizer(p, nil)
	p.vi = nil
}

// ----------------------------------------------------------------------------

type VideoInput struct {
	*videoInput
}

func NewVideoInput() *VideoInput {
	return &VideoInput{
		videoInput: newVideoInput(),
	}
}

func (p *VideoInput) Release() {
	p.videoInput.Release()
	*p = VideoInput{}
}

// ----------------------------------------------------------------------------

// [static]
func (p *VideoInput) ListDevices() (names []string) {
	names = make([]string, int(C.vi_listDevices()))
	for i, _ := range names {
		names[i] = C.GoString(C.vi_getDeviceName(C.int(i)))
	}
	return
}

// [static]
// SetVerbose turns on/off console messages(default is to print messages).
func (p *VideoInput) SetVerbose(verbose bool) {
	if verbose {
		C.vi_setVerbose(1)
	} else {
		C.vi_setVerbose(0)
	}
}

// [static]
// This allows for multithreaded use of VI (default is single threaded).
// Call this before any videoInput calls.
// Note if your app has other COM calls then you should set VIs COM usage to match the other COM mode.
func (p *VideoInput) SetComMultiThreaded(multi bool) {
	if multi {
		C.vi_setComMultiThreaded(1)
	} else {
		C.vi_setComMultiThreaded(0)
	}
}

// ----------------------------------------------------------------------------

// Choose to use callback based capture - or single threaded.
func (p *VideoInput) SetUseCallback(useCallback bool) {
	if useCallback {
		C.vi_device_setUseCallback(p.vi, 1)
	} else {
		C.vi_device_setUseCallback(p.vi, 0)
	}
}

// Call before setupDevice.
// Directshow will try and get the closest possible framerate to what is requested.
func (p *VideoInput) SetIdealFramerate(deviceID int, idealFramerate int) {
	C.vi_device_setIdealFramerate(p.vi, C.int(deviceID), C.int(idealFramerate))
}

// Some devices will stop delivering frames after a while - this method
// gives you the option to try and reconnect to a device if videoInput detects
// that a device has stopped delivering frames.
// You MUST CALL isFrameNew every app loop for this to have any effect.
func (p *VideoInput) SetAutoReconnectOnFreeze(
	deviceNumber int, doReconnect bool, numMissedFramesBeforeReconnect int,
) {
	C.vi_device_setAutoReconnectOnFreeze(p.vi,
		C.int(deviceNumber), viToBoolT(doReconnect), C.int(numMissedFramesBeforeReconnect),
	)
}

// Choose one of these four to setup your device.
func (p *VideoInput) SetupDevice(deviceID int) bool {
	rv := C.vi_device_setupDevice(p.vi, C.int(deviceID))
	return rv != 0
}

// Choose one of these four to setup your device.
func (p *VideoInput) SetupDeviceAndSize(deviceID, w, h int) bool {
	rv := C.vi_device_setupDeviceAndSize(p.vi, C.int(deviceID), C.int(w), C.int(h))
	return rv != 0
}

// Only for capture cards.
// USB and Firewire cameras souldn't specify connectionID.
func (p *VideoInput) SetupDeviceCaptureCards(deviceID, connectionID int) bool {
	rv := C.vi_device_setupDeviceCaptureCards(p.vi, C.int(deviceID), C.int(connectionID))
	return rv != 0
}

// Only for capture cards.
// USB and Firewire cameras souldn't specify connectionID.
func (p *VideoInput) SetupDeviceCaptureCardsAndSize(deviceID, w, h, connectionID int) bool {
	rv := C.vi_device_setupDeviceCaptureCardsAndSize(p.vi,
		C.int(deviceID), C.int(w), C.int(h), C.int(connectionID),
	)
	return rv != 0
}

// If you need to you can set your NTSC/PAL/SECAM.
// preference here. if it is available it will be used.
// see #defines above for available formats - eg VIDEO_INPUT_NTSC_M or VIDEO_INPUT_PAL_B.
// should be called after setupDevice.
// can be called multiple times.
func (p *VideoInput) SetFormat(deviceID int, format VideoInputFormat) bool {
	rv := C.vi_device_setFormat(p.vi, C.int(deviceID), C.int(format))
	return rv != 0
}
func (p *VideoInput) SetRequestedMediaSubType(mediatype VideoInputMediaSubType) {
	C.vi_device_setRequestedMediaSubType(p.vi, C.int(mediatype))
}

// Tells you when a new frame has arrived - you should call this if you have
// specified setAutoReconnectOnFreeze to true.
func (p *VideoInput) HasNewFrame(deviceID int) bool {
	rv := C.vi_device_isFrameNew(p.vi, C.int(deviceID))
	return rv != 0
}
func (p *VideoInput) IsDeviceSetup(deviceID int) bool {
	rv := C.vi_device_isDeviceSetup(p.vi, C.int(deviceID))
	return rv != 0
}

// Returns the pixels - flipRedAndBlue toggles RGB/BGR flipping - and you can flip the image too.
func (p *VideoInput) GetPixels(deviceID int, flipRedAndBlue, flipImage bool) []byte {
	d := C.vi_device_getPixels(p.vi,
		C.int(deviceID),
		viToBoolT(flipRedAndBlue),
		viToBoolT(flipImage),
	)
	n := p.GetDataSize(deviceID)
	if d == nil || n <= 0 {
		return nil
	}
	return ((*[1 << 30]byte)(unsafe.Pointer(d)))[0:n:n]
}

func (p *VideoInput) GetFrame(deviceID int) *IplImage {
	d := p.GetPixels(deviceID, true, true)
	if d == nil {
		return nil
	}
	w, h := p.GetSize(deviceID)
	p.m[deviceID].InitHeader(w, h, 8, 3, 0, 4)
	p.m[deviceID].imageData = (*C.char)(unsafe.Pointer(&d[0]))
	return &p.m[deviceID]
}

// Or pass in a buffer for getPixels to fill returns true if successful.
func (p *VideoInput) GetPixelsWithBuffer(deviceID int, buf []byte, flipRedAndBlue, flipImage bool) bool {
	rv := C.vi_device_getPixelsWithBuffer(p.vi,
		C.int(deviceID), (*C.uint8_t)(unsafe.Pointer(&buf[0])),
		viToBoolT(flipRedAndBlue),
		viToBoolT(flipImage),
	)
	return rv != 0
}

// Launches a pop up settings window.
// For some reason in GLUT you have to call it twice each time.
func (p *VideoInput) ShowSettingsWindow(deviceID int, wait bool) {
	C.vi_device_showSettingsWindow(p.vi, C.int(deviceID), viToBoolT(wait))
}

// Manual control over settings thanks.
// These are experimental for now.
func (p *VideoInput) SetVideoSettingFilter(deviceID int,
	Property, lValue, Flags int32,
	useDefaultValue bool,
) bool {
	rv := C.vi_device_setVideoSettingFilter(p.vi,
		C.int(deviceID),
		C.long(Property), C.long(lValue), C.long(Flags),
		viToBoolT(useDefaultValue),
	)
	return rv != 0
}

func (p *VideoInput) SetVideoSettingFilterPct(deviceID int,
	Property int32, pctValue float32, Flags int32,
) bool {
	rv := C.vi_device_setVideoSettingFilterPct(p.vi,
		C.int(deviceID),
		C.long(Property), C.float(pctValue), C.long(Flags),
	)
	return rv != 0

}

func (p *VideoInput) GetVideoSettingFilter(deviceID int, Property int32) (
	min, max, SteppingDelta, currentValue, flags, defaultValue int32,
	ok bool,
) {
	var _min, _max, _SteppingDelta, _currentValue, _flags, _defaultValue C.long
	rv := C.vi_device_getVideoSettingFilter(p.vi,
		C.int(deviceID), C.long(Property),
		&_min, &_max,
		&_SteppingDelta, &_currentValue, &_flags,
		&_defaultValue,
	)
	min, max = int32(_min), int32(_max)
	SteppingDelta, currentValue, flags = int32(_SteppingDelta), int32(_currentValue), int32(_flags)
	defaultValue = int32(_defaultValue)
	ok = rv != 0
	return
}

func (p *VideoInput) SetVideoSettingCamera(deviceID int,
	Property, lValue, Flags int32,
	useDefaultValue bool,
) bool {
	rv := C.vi_device_setVideoSettingCamera(p.vi,
		C.int(deviceID),
		C.long(Property), C.long(lValue), C.long(Flags),
		viToBoolT(useDefaultValue),
	)
	return rv != 0
}

func (p *VideoInput) SetVideoSettingCameraPct(deviceID int,
	Property int32, pctValue float32, Flags int32,
) bool {
	rv := C.vi_device_setVideoSettingCameraPct(p.vi,
		C.int(deviceID),
		C.long(Property), C.float(pctValue), C.long(Flags),
	)
	return rv != 0
}

func (p *VideoInput) GetVideoSettingCamera(deviceID int, Property int32) (
	min, max, SteppingDelta, currentValue, flags, defaultValue int32,
	ok bool,
) {
	var _min, _max, _SteppingDelta, _currentValue, _flags, _defaultValue C.long
	rv := C.vi_device_getVideoSettingCamera(p.vi,
		C.int(deviceID), C.long(Property),
		&_min, &_max,
		&_SteppingDelta, &_currentValue, &_flags,
		&_defaultValue,
	)
	min, max = int32(_min), int32(_max)
	SteppingDelta, currentValue, flags = int32(_SteppingDelta), int32(_currentValue), int32(_flags)
	defaultValue = int32(_defaultValue)
	ok = rv != 0
	return

}

func (p *VideoInput) GetSize(deviceID int) (width, height int) {
	width = int(C.vi_device_getWidth(p.vi, C.int(deviceID)))
	height = int(C.vi_device_getHeight(p.vi, C.int(deviceID)))
	return
}
func (p *VideoInput) GetDataSize(deviceID int) int {
	return int(C.vi_device_getSize(p.vi, C.int(deviceID)))
}

// Completely stops and frees a device
func (p *VideoInput) StopDevice(deviceID int) {
	C.vi_device_stopDevice(p.vi, C.int(deviceID))
}

// as above but then sets it up with same settings
func (p *VideoInput) RestartDevice(deviceID int) bool {
	rv := C.vi_device_restartDevice(p.vi, C.int(deviceID))
	return rv != 0
}

// ----------------------------------------------------------------------------

func viToBoolT(v bool) C.vi_bool_t {
	if v {
		return 1
	} else {
		return 0
	}
}

// ----------------------------------------------------------------------------
