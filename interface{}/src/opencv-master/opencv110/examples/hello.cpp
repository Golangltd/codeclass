// Copyright 2013 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <highgui.h>

void onMouseEvent(int event, int x, int y, int flags, void* param) {
	printf("onMouseEvent: event = %d, x = %d, y = %d, flags = %d\n", event, x, y, flags);	
}

int track_val = 0;
void onTrackbar(int pos) {
	printf("onTrackbar: pos = %d\n", pos);
}

int main(int argc, char* argv[]) {
	const char* filename = "../testdata/lena.jpg";
	if(argc >= 2) {
		filename = argv[1];
	}

	IplImage* img = cvLoadImage(filename, 1);
	if(img == NULL) {
		printf("Load %s failed!\n", filename);
		return -1;
	}
	cvSaveImage("out.jpg", img);

	const char* winName = "Go-OpenCV";
	cvNamedWindow(winName, 1);
	cvSetMouseCallback(winName, onMouseEvent);
	cvCreateTrackbar("TrackBar", winName, &track_val, 100, onTrackbar);

	cvShowImage(winName, img);
	cvWaitKey(0);

	return 0;
}
