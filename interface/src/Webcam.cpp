//
//  Webcam.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QtDebug>

#include <opencv2/opencv.hpp>

#include <Log.h>

#include "Webcam.h"

Webcam::Webcam() {
    if ((_capture = cvCaptureFromCAM(-1)) == 0) {
        printLog("Failed to open webcam.\n");
        return;
    }
    
    // get the dimensions, fps of the frames
    _frameWidth = cvGetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH);
    _frameHeight = cvGetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT);
    int fps = cvGetCaptureProperty(_capture, CV_CAP_PROP_FPS);
    printLog("Opened camera [width=%d, height=%d, fps=%d].", _frameWidth, _frameHeight, fps);    
}

void Webcam::init() {
    // initialize the texture that will contain the grabbed frames
    glGenTextures(1, &_frameTextureID);
    glBindTexture(GL_TEXTURE_2D, _frameTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _frameWidth, _frameHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);   
}

Webcam::~Webcam() {
    if (_capture != 0) {
        cvReleaseCapture(&_capture);
    }
}

void Webcam::grabFrame() {
    IplImage* image = cvQueryFrame(_capture);
    if (image == 0) {
        return;
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _frameWidth, _frameHeight, GL_RGB, GL_UNSIGNED_BYTE, image->imageData);
    glBindTexture(GL_TEXTURE_2D, 0);
}
