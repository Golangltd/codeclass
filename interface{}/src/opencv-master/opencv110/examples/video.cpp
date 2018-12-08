// Copyright 2013 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <highgui.h>

int main(int argc, char* argv[]) {
	const char* filename = "./test.avi";
	if(argc > 1) {
		filename = argv[1];
	}

	CvCapture* cap = cvCreateFileCapture(filename);
	if(cap == NULL) {
		printf("open camera failed!\n");
		return -1;
	}
	cvSaveImage("video.jpg", cvQueryFrame(cap));

	const char* winName = "Go-OpenCV";
	cvNamedWindow(winName, 1);
	for(;;) {
		IplImage* img = cvQueryFrame(cap);
		cvShowImage(winName, img);
		if(cvWaitKey(50) == 27) {
			break;
		}
	}

	cvDestroyWindow(winName);
	cvReleaseCapture(&cap);
	return 0;
}
