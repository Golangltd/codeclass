// Copyright 2011 <chaishushan@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "opencv.h"
#include "_cgo_export.h"

#include <stdlib.h>
#include <string.h>

//-----------------------------------------------------------------------------
// trackbar
//-----------------------------------------------------------------------------

// trackbar data
struct TrackbarUserdata {
	schar* win_name;
	schar* bar_name;
	int value_old;
	int value_now;
	int value_max;
};
static struct TrackbarUserdata *trackbar_list[1000];
static int trackbar_list_len = 0;

static void trackbarCallback(int pos) {
	int i;
	for(i = 0; i < trackbar_list_len; ++i) {
		struct TrackbarUserdata *arg = trackbar_list[i];
		if(arg->value_old != arg->value_now) {
			arg->value_old = arg->value_now;
			goTrackbarCallback(arg->bar_name, arg->win_name, pos);
			return;
		}
	}
}
int GoOpenCV_CreateTrackbar(
	char* trackbar_name, char* window_name,
	int value, int count
) {
	struct TrackbarUserdata *userdata = malloc(sizeof(*userdata));
	trackbar_list[trackbar_list_len++] = userdata;

	userdata->win_name = (schar*)window_name;
	userdata->bar_name = (schar*)trackbar_name;
	userdata->value_old = value;
	userdata->value_now = value;
	userdata->value_max = count;

	return cvCreateTrackbar(trackbar_name, window_name,
		&(userdata->value_now), count,
		trackbarCallback
	);
}
void GoOpenCV_DestroyTrackbar(char* trackbar_name, char* window_name) {
	int i;
	for(i = 0; i < trackbar_list_len; ++i) {
		if(strcmp((char*)trackbar_list[i]->win_name, window_name)) continue;
		if(strcmp((char*)trackbar_list[i]->bar_name, trackbar_name)) continue;

		free(trackbar_list[i]);
		trackbar_list[i] = trackbar_list[--trackbar_list_len];
		break;
	}
}

//-----------------------------------------------------------------------------
// mouse callback
//-----------------------------------------------------------------------------

static void mouseCallback(int event, int x, int y, int flags, void* param) {
	schar* name = (schar*)param;
	goMouseCallback(name, event, x, y, flags);
}
void GoOpenCV_SetMouseCallback(const char* window_name) {
	cvSetMouseCallback(window_name, mouseCallback, (void*)window_name);
}

//-----------------------------------------------------------------------------

// video writer args
unsigned GoOpenCV_FOURCC_(int c1, int c2, int c3, int c4) {
	return (unsigned)CV_FOURCC(c1,c2,c3,c4);
}

//-----------------------------------------------------------------------------

