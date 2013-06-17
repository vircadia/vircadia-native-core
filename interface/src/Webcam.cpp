//
//  Webcam.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <opencv2/opencv.hpp>

#include <Log.h>

#include "Webcam.h"

Webcam::Webcam() {
    if ((_capture = cvCaptureFromCAM(-1)) == 0) {
        printLog("Failed to open webcam.\n");
    }
}

Webcam::~Webcam() {
    if (_capture != 0) {
        cvReleaseCapture(&_capture);
    }
}
