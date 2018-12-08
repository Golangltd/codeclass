// Copyright 2013 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <highgui.h>
int main(int argc, char* argv[]) {
	CvCapture* cap = cvCreateCameraCapture(0);
	if(cap == NULL) {
		printf("open camera failed!\n");
		return -1;
	}
	IplImage* img = cvQueryFrame(cap);
	cvSaveImage("cam.jpg", img);

	CvVideoWriter* writer = cvCreateVideoWriter(
		"cam.avi", CV_FOURCC('M', 'J', 'P', 'G'), 30,
		cvSize(img->width, img->height), 1
	);
	if(writer == NULL) {
		printf("create writer failed!\n");
		return -1;
	}

	const char* winName = "Go-OpenCV";
	cvNamedWindow(winName, 1);
	for(;;) {
		IplImage* img = cvQueryFrame(cap);
		cvWriteFrame(writer, img);
		cvShowImage(winName, img);
		if(cvWaitKey(50) == 27) {
			break;
		}
	}

	cvDestroyWindow(winName);
	cvReleaseVideoWriter(&writer);
	cvReleaseCapture(&cap);
	return 0;
}
