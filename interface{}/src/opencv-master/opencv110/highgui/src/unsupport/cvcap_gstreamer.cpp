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
// Copyright (C) 2008, Nils Hasler, all rights reserved.
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

// Author: Nils Hasler <hasler@mpi-inf.mpg.de>
//
//         Max-Planck-Institut Informatik
//
// this implementation was inspired by gnash's gstreamer interface

//
// use GStreamer to read a video
//

#include <unistd.h>
#include <string.h>
#include <gst/gst.h>
#include "_highgui.h"
#include "gstappsink.h"

#ifdef NDEBUG
#define CV_WARN(message)
#else
#define CV_WARN(message) fprintf(stderr, "warning: %s (%s:%d)\n", message, __FILE__, __LINE__)
#endif

static bool isInited = false;

typedef struct CvCapture_GStreamer
{
	/// method call table
	int			type;	// one of [1394, v4l2, v4l, file]

	GstElement	       *pipeline;
	GstElement	       *source;
	GstElement	       *decodebin;
	GstElement	       *colour;
	GstElement	       *appsink;

	GstBuffer	       *buffer;

	GstCaps		       *caps;	// filter caps inserted right after the source

	IplImage	       *frame;
} CvCapture_GStreamer;

static void icvClose_GStreamer(CvCapture_GStreamer *cap)
{
	if(cap->pipeline) {
		gst_element_set_state(GST_ELEMENT(cap->pipeline), GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(cap->pipeline));
	}

	if(cap->buffer)
		gst_buffer_unref(cap->buffer);

	if(cap->frame)
		cvReleaseImage(&cap->frame);

	if(cap->caps)
		gst_caps_unref(cap->caps);
}

static void icvHandleMessage(CvCapture_GStreamer *cap)
{
	GstBus* bus = gst_element_get_bus(cap->pipeline);

	while(gst_bus_have_pending(bus)) {
		GstMessage* msg = gst_bus_pop(bus);

//		printf("Got %s message\n", GST_MESSAGE_TYPE_NAME(msg));

		switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_STATE_CHANGED:
			GstState oldstate, newstate, pendstate;
			gst_message_parse_state_changed(msg, &oldstate, &newstate, &pendstate);
//			printf("state changed from %d to %d (%d)\n", oldstate, newstate, pendstate);
			break;
		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;
			gst_message_parse_error(msg, &err, &debug);

			fprintf(stderr, "GStreamer Plugin: Embedded video playback halted; module %s reported: %s\n",
				  gst_element_get_name(GST_MESSAGE_SRC (msg)), err->message);

			g_error_free(err);
			g_free(debug);

			gst_element_set_state(cap->pipeline, GST_STATE_NULL);

			break;
			}
		case GST_MESSAGE_EOS:
//			CV_WARN("NetStream has reached the end of the stream.");

			break;
		default:
//			CV_WARN("unhandled message\n");
			break;
		}

		gst_message_unref(msg);
	}

	gst_object_unref(GST_OBJECT(bus));
}

//
// start the pipeline, grab a buffer, and pause again
//
static int icvGrabFrame_GStreamer(CvCapture_GStreamer *cap)
{
	if(!cap->pipeline)
		return 0;

	if(gst_app_sink_is_eos(GST_APP_SINK(cap->appsink))) {
		//printf("end of stream\n");
		return 0;
	}

	if(cap->buffer)
		gst_buffer_unref(cap->buffer);

	icvHandleMessage(cap);

	if(!gst_app_sink_get_queue_length(GST_APP_SINK(cap->appsink))) {
//		printf("no buffers queued, starting pipeline\n");

		if(gst_element_set_state(GST_ELEMENT(cap->pipeline), GST_STATE_PLAYING) ==
		GST_STATE_CHANGE_FAILURE) {
			icvHandleMessage(cap);
			return 0;
		}

// 		icvHandleMessage(cap);
//
// 		// check whether stream contains an acceptable video stream
// 		GstPad *sinkpad = gst_element_get_pad(cap->colour, "sink");
// 		if(!GST_PAD_IS_LINKED(sinkpad)) {
// 			gst_object_unref(sinkpad);
// 			fprintf(stderr, "GStreamer: Pipeline is NOT ready. Format unknown?\n");
// 			return 0;
// 		}
// 		gst_object_unref(sinkpad);

// 		printf("pulling preroll\n");
//
// 		if(!gst_app_sink_pull_preroll(GST_APP_SINK(cap->appsink))) {
// 			printf("no preroll\n");
// 			return 0;
// 		}

//		printf("pulling buffer\n");

		cap->buffer = gst_app_sink_pull_buffer(GST_APP_SINK(cap->appsink));

//		printf("pausing pipeline\n");

		if(gst_element_set_state(GST_ELEMENT(cap->pipeline), GST_STATE_PAUSED) ==
		GST_STATE_CHANGE_FAILURE) {
			icvHandleMessage(cap);
			return 0;
		}

//		printf("pipeline paused\n");
	} else {
//		printf("peeking buffer, %d buffers in queue\n",
//		       gst_app_sink_get_queue_length(GST_APP_SINK(cap->appsink)));
		cap->buffer = gst_app_sink_peek_buffer(GST_APP_SINK(cap->appsink));
	}

	if(!cap->buffer)
		return 0;

//	printf("pulled buffer %p\n", cap->buffer);

	return 1;
}

//
// decode buffer
//
static IplImage *icvRetrieveFrame_GStreamer(CvCapture_GStreamer *cap)
{
	if(!cap->buffer)
		return 0;

//	printf("getting buffercaps\n");

	GstCaps* caps = gst_buffer_get_caps(cap->buffer);

	assert(gst_caps_get_size(caps) == 1);

	GstStructure* structure = gst_caps_get_structure(caps, 0);

	gint bpp, endianness, redmask, greenmask, bluemask;

	if(!gst_structure_get_int(structure, "bpp", &bpp) ||
	   !gst_structure_get_int(structure, "endianness", &endianness) ||
	   !gst_structure_get_int(structure, "red_mask", &redmask) ||
	   !gst_structure_get_int(structure, "green_mask", &greenmask) ||
	   !gst_structure_get_int(structure, "blue_mask", &bluemask)) {
		printf("missing essential information in buffer caps, %s\n", gst_caps_to_string(caps));
		return 0;
	}

	printf("buffer has %d bpp, endianness %d, rgb %x %x %x, %s\n", bpp, endianness, redmask, greenmask, bluemask, gst_caps_to_string(caps));

	if(!redmask || !greenmask || !bluemask)
		return 0;

	if(!cap->frame) {
		gint height, width;

		if(!gst_structure_get_int(structure, "width", &width) ||
		   !gst_structure_get_int(structure, "height", &height))
			return 0;

//		printf("creating frame %dx%d\n", width, height);

		cap->frame = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
	}

	gst_caps_unref(caps);

	unsigned char *data = GST_BUFFER_DATA(cap->buffer);

	printf("generating shifts\n");

	IplImage *frame = cap->frame;
	unsigned nbyte = bpp >> 3;
	unsigned redshift, blueshift, greenshift;
	unsigned mask = redmask;
	for(redshift = 0, mask = redmask; (mask & 1) == 0; mask >>= 1, redshift++)
		;
	for(greenshift = 0, mask = greenmask; (mask & 1) == 0; mask >>= 1, greenshift++)
		;
	for(blueshift = 0, mask = bluemask; (mask & 1) == 0; mask >>= 1, blueshift++)
		;

	printf("shifts: %u %u %u\n", redshift, greenshift, blueshift);

	for(int r = 0; r < frame->height; r++) {
		for(int c = 0; c < frame->width; c++, data += nbyte) {
			int at = r * frame->widthStep + c * 3;
			frame->imageData[at] = ((*((gint *)data)) & redmask) >> redshift;
			frame->imageData[at+1] = ((*((gint *)data)) & greenmask) >> greenshift;
			frame->imageData[at+2] = ((*((gint *)data)) & bluemask) >> blueshift;
		}
	}

//	printf("converted buffer\n");

	gst_buffer_unref(cap->buffer);
	cap->buffer = 0;

	return cap->frame;
}

static double icvGetProperty_GStreamer(CvCapture_GStreamer *cap, int id)
{
	GstFormat format;
	//GstQuery q;
	gint64 value;

	if(!cap->pipeline) {
		CV_WARN("GStreamer: no pipeline");
		return 0;
	}

	switch(id) {
	case CV_CAP_PROP_POS_MSEC:
		format = GST_FORMAT_TIME;
		if(!gst_element_query_position(cap->pipeline, &format, &value)) {
			CV_WARN("GStreamer: unable to query position of stream");
			return 0;
		}
		return value * 1e-6; // nano seconds to milli seconds
	case CV_CAP_PROP_POS_FRAMES:
		format = GST_FORMAT_BUFFERS;
		if(!gst_element_query_position(cap->pipeline, &format, &value)) {
			CV_WARN("GStreamer: unable to query position of stream");
			return 0;
		}
		return value;
	case CV_CAP_PROP_POS_AVI_RATIO:
		format = GST_FORMAT_PERCENT;
		if(!gst_element_query_position(cap->pipeline, &format, &value)) {
			CV_WARN("GStreamer: unable to query position of stream");
			return 0;
		}
//		printf("value %llu %llu %g\n", value, GST_FORMAT_PERCENT_MAX, ((double) value) / GST_FORMAT_PERCENT_MAX);
		return ((double) value) / GST_FORMAT_PERCENT_MAX;
	case CV_CAP_PROP_FRAME_WIDTH:
	case CV_CAP_PROP_FRAME_HEIGHT:
	case CV_CAP_PROP_FPS:
	case CV_CAP_PROP_FOURCC:
		break;
	case CV_CAP_PROP_FRAME_COUNT:
		format = GST_FORMAT_BUFFERS;
		if(!gst_element_query_duration(cap->pipeline, &format, &value)) {
			CV_WARN("GStreamer: unable to query position of stream");
			return 0;
		}
		return value;
	case CV_CAP_PROP_FORMAT:
	case CV_CAP_PROP_MODE:
	case CV_CAP_PROP_BRIGHTNESS:
	case CV_CAP_PROP_CONTRAST:
	case CV_CAP_PROP_SATURATION:
	case CV_CAP_PROP_HUE:
	case CV_CAP_PROP_GAIN:
	case CV_CAP_PROP_CONVERT_RGB:
		break;
	default:
		CV_WARN("GStreamer: unhandled property");
		break;
	}
	return 0;
}

static void icvRestartPipeline(CvCapture_GStreamer *cap)
{
	CV_FUNCNAME("icvRestartPipeline");

	__BEGIN__;

	printf("restarting pipeline, going to ready\n");

	if(gst_element_set_state(GST_ELEMENT(cap->pipeline), GST_STATE_READY) ==
	   GST_STATE_CHANGE_FAILURE) {
		CV_ERROR(CV_StsError, "GStreamer: unable to start pipeline\n");
		return;
	}

	printf("ready, relinking\n");

	gst_element_unlink(cap->source, cap->decodebin);
	printf("filtering with %s\n", gst_caps_to_string(cap->caps));
	gst_element_link_filtered(cap->source, cap->decodebin, cap->caps);

	printf("relinked, pausing\n");

	if(gst_element_set_state(GST_ELEMENT(cap->pipeline), GST_STATE_PAUSED) ==
	   GST_STATE_CHANGE_FAILURE) {
		CV_ERROR(CV_StsError, "GStreamer: unable to start pipeline\n");
		return;
	}

	printf("state now paused\n");

 	__END__;
}

static void icvSetFilter(CvCapture_GStreamer *cap, const char *property, int type, int v1, int v2)
{
	printf("setting cap %p %s %d %d %d\n", cap->caps, property, type, v1, v2);

	if(!cap->caps) {
		if(type == G_TYPE_INT)
			cap->caps = gst_caps_new_simple("video/x-raw-rgb", property, type, v1, NULL);
		else
			cap->caps = gst_caps_new_simple("video/x-raw-rgb", property, type, v1, v2, NULL);
	} else {
		printf("caps before setting %s\n", gst_caps_to_string(cap->caps));
		if(type == G_TYPE_INT)
			gst_caps_set_simple(cap->caps, "video/x-raw-rgb", property, type, v1, NULL);
		else
			gst_caps_set_simple(cap->caps, "video/x-raw-rgb", property, type, v1, v2, NULL);
	}

	icvRestartPipeline(cap);
}

static void icvRemoveFilter(CvCapture_GStreamer *cap, const char *filter)
{
	if(!cap->caps)
		return;

	GstStructure *s = gst_caps_get_structure(cap->caps, 0);
	gst_structure_remove_field(s, filter);

	icvRestartPipeline(cap);
}

static int icvSetProperty_GStreamer(CvCapture_GStreamer *cap, int id, double value)
{
	GstFormat format;
	GstSeekFlags flags;

	if(!cap->pipeline) {
		CV_WARN("GStreamer: no pipeline");
		return 0;
	}

	switch(id) {
	case CV_CAP_PROP_POS_MSEC:
		format = GST_FORMAT_TIME;
		flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_ACCURATE);
		if(!gst_element_seek_simple(GST_ELEMENT(cap->pipeline), format,
					    flags, (gint64) (value * GST_MSECOND))) {
			CV_WARN("GStreamer: unable to seek");
		}
		break;
	case CV_CAP_PROP_POS_FRAMES:
		format = GST_FORMAT_BUFFERS;
		flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_ACCURATE);
		if(!gst_element_seek_simple(GST_ELEMENT(cap->pipeline), format,
					    flags, (gint64) value)) {
			CV_WARN("GStreamer: unable to seek");
		}
		break;
	case CV_CAP_PROP_POS_AVI_RATIO:
		format = GST_FORMAT_PERCENT;
		flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_ACCURATE);
		if(!gst_element_seek_simple(GST_ELEMENT(cap->pipeline), format,
					    flags, (gint64) (value * GST_FORMAT_PERCENT_MAX))) {
			CV_WARN("GStreamer: unable to seek");
		}
		break;
	case CV_CAP_PROP_FRAME_WIDTH:
		if(value > 0)
			icvSetFilter(cap, "width", G_TYPE_INT, (int) value, 0);
		else
			icvRemoveFilter(cap, "width");
		break;
	case CV_CAP_PROP_FRAME_HEIGHT:
		if(value > 0)
			icvSetFilter(cap, "height", G_TYPE_INT, (int) value, 0);
		else
			icvRemoveFilter(cap, "height");
		break;
	case CV_CAP_PROP_FPS:
		if(value > 0) {
			int num, denom;
			num = (int) value;
			if(value != num) { // FIXME this supports only fractions x/1 and x/2
				num = (int) (value * 2);
				denom = 2;
			} else
				denom = 1;

			icvSetFilter(cap, "framerate", GST_TYPE_FRACTION, num, denom);
		} else
			icvRemoveFilter(cap, "framerate");
		break;
	case CV_CAP_PROP_FOURCC:
	case CV_CAP_PROP_FRAME_COUNT:
	case CV_CAP_PROP_FORMAT:
	case CV_CAP_PROP_MODE:
	case CV_CAP_PROP_BRIGHTNESS:
	case CV_CAP_PROP_CONTRAST:
	case CV_CAP_PROP_SATURATION:
	case CV_CAP_PROP_HUE:
	case CV_CAP_PROP_GAIN:
	case CV_CAP_PROP_CONVERT_RGB:
		break;
	default:
		CV_WARN("GStreamer: unhandled property");
	}
	return 0;
}

//
// connect decodebin's dynamically created source pads to colourconverter
//
static void icvNewPad(GstElement *decodebin, GstPad *pad, gboolean last, gpointer data)
{
	GstElement *sink = GST_ELEMENT(data);
	GstStructure *str;
	GstPad *sinkpad;
	GstCaps *caps;

	/* link only once */
	sinkpad = gst_element_get_pad(sink, "sink");

	if(GST_PAD_IS_LINKED(sinkpad)) {
		g_print("sink is already linked\n");
		g_object_unref(sinkpad);
		return;
	}

	/* check media type */
	caps = gst_pad_get_caps(pad);
	str = gst_caps_get_structure(caps, 0);
	const char *structname = gst_structure_get_name(str);
//	g_print("new pad %s\n", structname);
	if(!g_strrstr(structname, "video")) {
		gst_caps_unref(caps);
		gst_object_unref(sinkpad);
		return;
	}
	printf("linking pad %s\n", structname);

	/* link'n'play */
	gst_pad_link (pad, sinkpad);

	gst_caps_unref(caps);
	gst_object_unref(sinkpad);
}

static CvCapture_GStreamer * icvCreateCapture_GStreamer(int type, const char *filename)
{
	CvCapture_GStreamer *capture = 0;
	CV_FUNCNAME("cvCaptureFromCAM_GStreamer");

	__BEGIN__;

//	teststreamer(filename);

//	return 0;

	if(!isInited) {
//		printf("gst_init\n");
		gst_init (NULL, NULL);

// according to the documentation this is the way to register a plugin now
// unfortunately, it has not propagated into my distribution yet...
// 		gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR,
// 			"opencv-appsink", "Element application sink",
// 			"0.1", appsink_plugin_init, "LGPL", "highgui", "opencv",
// 			"http://opencvlibrary.sourceforge.net/");

		isInited = true;
	}

	const char *sourcetypes[] = {"dv1394src", "v4lsrc", "v4l2src", "filesrc"};
//	printf("entered capturecreator %s\n", sourcetypes[type]);

	GstElement *source = gst_element_factory_make(sourcetypes[type], NULL);
	if(!source)
		return 0;

	if(type == CV_CAP_GSTREAMER_FILE)
		g_object_set(G_OBJECT(source), "location", filename, NULL);

	GstElement *colour = gst_element_factory_make("ffmpegcolorspace", NULL);

	GstElement *sink = gst_element_factory_make("opencv-appsink", NULL);
	GstCaps *caps = gst_caps_new_simple("video/x-raw-rgb", NULL);
	gst_app_sink_set_caps(GST_APP_SINK(sink), caps);
//	gst_caps_unref(caps);
	gst_base_sink_set_sync(GST_BASE_SINK(sink), false);
//	g_signal_connect(sink, "new-buffer", G_CALLBACK(newbuffer), NULL);

	GstElement *decodebin = gst_element_factory_make("decodebin", NULL);
	g_signal_connect(decodebin, "new-decoded-pad", G_CALLBACK(icvNewPad), colour);

	GstElement *pipeline = gst_pipeline_new (NULL);

	gst_bin_add_many(GST_BIN(pipeline), source, decodebin, colour, sink, NULL);

//	printf("added many\n");

	switch(type) {
	case CV_CAP_GSTREAMER_V4L2: // default to 640x480, 30 fps
		caps = gst_caps_new_simple("video/x-raw-rgb",
					   "width", G_TYPE_INT, 640,
					   "height", G_TYPE_INT, 480,
					   "framerate", GST_TYPE_FRACTION, 30, 1,
					   NULL);
		if(!gst_element_link_filtered(source, decodebin, caps)) {
			CV_ERROR(CV_StsError, "GStreamer: cannot link v4l2src -> decodebin\n");
			gst_object_unref(pipeline);
			return 0;
		}
		gst_caps_unref(caps);
		break;
	case CV_CAP_GSTREAMER_V4L:
	case CV_CAP_GSTREAMER_1394:
	case CV_CAP_GSTREAMER_FILE:
		if(!gst_element_link(source, decodebin)) {
			CV_ERROR(CV_StsError, "GStreamer: cannot link filesrc -> decodebin\n");
			gst_object_unref(pipeline);
			return 0;
		}
		break;
	}

	if(!gst_element_link(colour, sink)) {
		CV_ERROR(CV_StsError, "GStreamer: cannot link colour -> sink\n");
		gst_object_unref(pipeline);
		return 0;
	}

//	printf("linked, pausing\n");

	if(gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PAUSED) ==
	   GST_STATE_CHANGE_FAILURE) {
		CV_WARN("GStreamer: unable to set pipeline to paused\n");
//		icvHandleMessage(capture);
//		cvReleaseCapture((CvCapture **)(void *)&capture);
		gst_object_unref(pipeline);
		return 0;
	}

//	printf("state now paused\n");

	// construct capture struct
	capture = (CvCapture_GStreamer *)cvAlloc(sizeof(CvCapture_GStreamer));
	memset(capture, 0, sizeof(CvCapture_GStreamer));
	capture->type = type;
	capture->pipeline = pipeline;
	capture->source = source;
	capture->decodebin = decodebin;
	capture->colour = colour;
	capture->appsink = sink;

	icvHandleMessage(capture);

	OPENCV_ASSERT(capture,
                      "cvCaptureFromFile_GStreamer( const char * )", "couldn't create capture");

//	GstClock *clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline));
//	printf("clock %s\n", gst_object_get_name(GST_OBJECT(clock)));

	__END__;

	return capture;
}

#if 0
//
//
// image sequence writer
//
//
typedef struct CvVideoWriter_GStreamer {
	char		       *filename;
	unsigned		currentframe;
};

static int icvWriteFrame_GStreamer( CvVideoWriter* writer, const IplImage* image )
{
	CvVideoWriter_GStreamer *wri = (CvVideoWriter_GStreamer *)writer;

	char str[100];
	char *x = str;
	int size = 100;
	while(snprintf(x, size, wri->filename, wri->currentframe) == size - 1) {
		size *= 2;
		if(x == str)
			x = (char *)malloc(size);
		else
			x = (char *)realloc(x, size);
	}

	int ret = cvSaveImage(x, image);

	wri->currentframe++;

	if(x != str)
		free(x);

	return ret;
}

static void icvReleaseVideoWriter_GStreamer( CvVideoWriter** writer )
{
	CvVideoWriter_GStreamer **wri = (CvVideoWriter_GStreamer **)writer;

	free((*wri)->filename);
}

CvVideoWriter* cvCreateVideoWriter_GStreamer( const char* filename )
{
	CvVideoWriter_GStreamer *writer;

	unsigned offset = 0;
	char *name = icvExtractPattern(filename, &offset);
	if(!name)
		return 0;

	char str[100];
	char *x = str;
	int size = 100;
	while(snprintf(x, size, name, 0) == size - 1) {
		size *= 2;
		if(x == str)
			x = (char *)malloc(size);
		else
			x = (char *)realloc(x, size);
	}
	if(!cvHaveImageWriter(x)) {
		if(x != str)
			free(x);
		return 0;
	}
	if(x != str)
		free(x);

	writer = (CvVideoWriter_GStreamer *)cvAlloc(sizeof(CvCapture_GStreamer));
	memset(writer, 0, sizeof(CvVideoWriter_GStreamer));
	writer->filename = strdup(name);
	writer->currentframe = offset;

	return (CvVideoWriter *)writer;
}
#endif


class CvCapture_GStreamer_CPP : public CvCapture
{
public:
    CvCapture_GStreamer_CPP() { captureGS = 0; }
    virtual ~CvCapture_GStreamer_CPP() { close(); }

    virtual bool open( int type, const char* filename );
    virtual void close();

    virtual double getProperty(int);
    virtual bool setProperty(int, double);
    virtual bool grabFrame();
    virtual IplImage* retrieveFrame();
protected:

    CvCapture_GStreamer* captureGS;
};

bool CvCapture_GStreamer_CPP::open( int type, const char* filename )
{
    close();
    captureGS = icvCreateCapture_GStreamer( type, filename );
    return captureGS != 0;
}

void CvCapture_GStreamer_CPP::close()
{
    if( captureGS )
    {
        icvClose_GStreamer( captureGS );
        cvFree( &captureGS );
    }
}

bool CvCapture_GStreamer_CPP::grabFrame()
{
    return captureGS ? icvGrabFrame_GStreamer( captureGS ) != 0 : false;
}

IplImage* CvCapture_GStreamer_CPP::retrieveFrame()
{
    return captureGS ? (IplImage*)icvRetrieveFrame_GStreamer( captureGS ) : 0;
}

double CvCapture_GStreamer_CPP::getProperty( int propId )
{
    return captureGS ? icvGetProperty_GStreamer( captureGS, propId ) : 0;
}

bool CvCapture_GStreamer_CPP::setProperty( int propId, double value )
{
    return captureGS ? icvSetProperty_GStreamer( captureGS, propId, value ) != 0 : false;
}

CvCapture* cvCreateCapture_GStreamer( int type, const char* filename )
{
    CvCapture_GStreamer_CPP* capture = new CvCapture_GStreamer_CPP;

    if( capture->open( type, filename ))
        return capture;

    delete capture;
    return 0;
}
