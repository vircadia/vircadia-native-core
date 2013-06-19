//
//  Webcam.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QTimer>
#include <QtDebug>

#include <opencv2/opencv.hpp>

#include <Log.h>
#include <SharedUtil.h>

#ifdef __APPLE__
#include <UVCCameraControl.hpp>
#endif

#include "Application.h"
#include "Webcam.h"

Webcam::Webcam() : _enabled(false), _frameTextureID(0)  {
    // the grabber simply runs as fast as possible
    _grabber = new FrameGrabber();
    _grabber->moveToThread(&_grabberThread);
}

void Webcam::setEnabled(bool enabled) {
    if (_enabled == enabled) {
        return;
    }
    if ((_enabled = enabled)) {
        _grabberThread.start();
        _startTimestamp = 0;
        _frameCount = 0;
        
        // let the grabber know we're ready for the first frame
        QMetaObject::invokeMethod(_grabber, "grabFrame");
    
    } else {
        _grabberThread.quit();
    }
}

void Webcam::renderPreview(int screenWidth, int screenHeight) {
    if (_enabled && _frameTextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            const int PREVIEW_HEIGHT = 200;
            int previewWidth = _frameWidth * PREVIEW_HEIGHT / _frameHeight;
            int top = screenHeight - 600;
            int left = screenWidth - previewWidth - 10;
            
            glTexCoord2f(0, 0);
            glVertex2f(left, top);
            glTexCoord2f(1, 0);
            glVertex2f(left + previewWidth, top);
            glTexCoord2f(1, 1);
            glVertex2f(left + previewWidth, top + PREVIEW_HEIGHT);
            glTexCoord2f(0, 1);
            glVertex2f(left, top + PREVIEW_HEIGHT);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        
        char fps[20];
        sprintf(fps, "FPS: %.4g", _frameCount * 1000000.0f / (usecTimestampNow() - _startTimestamp));
        drawtext(left, top + PREVIEW_HEIGHT + 20, 0.10, 0, 1, 0, fps);
    }
}

Webcam::~Webcam() {
    // stop the grabber thread
    _grabberThread.quit();
    _grabberThread.wait();
    
    delete _grabber;
}

void Webcam::setFrame(void* image) {
    IplImage* img = static_cast<IplImage*>(image);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, img->widthStep / 3);
    if (_frameTextureID == 0) {
        glGenTextures(1, &_frameTextureID);
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _frameWidth = img->width, _frameHeight = img->height, 0, GL_BGR,
            GL_UNSIGNED_BYTE, img->imageData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        printLog("Capturing webcam at %dx%d.\n", _frameWidth, _frameHeight);
    
    } else {
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _frameWidth, _frameHeight, GL_BGR, GL_UNSIGNED_BYTE, img->imageData);    
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // update our frame count for fps computation
    _frameCount++;
    
    const int MAX_FPS = 30;
    const int MIN_FRAME_DELAY = 1000000 / MAX_FPS;
    long long now = usecTimestampNow();
    long long remaining = MIN_FRAME_DELAY;
    if (_startTimestamp == 0) {
        _startTimestamp = now;
    } else {
        remaining -= (now - _lastFrameTimestamp);
    }
    _lastFrameTimestamp = now;
    
    // let the grabber know we're ready for the next frame
    QTimer::singleShot(qMax((int)remaining / 1000, 0), _grabber, SLOT(grabFrame()));
}

FrameGrabber::~FrameGrabber() {
    if (_capture != 0) {
        cvReleaseCapture(&_capture);
    }
}

void FrameGrabber::grabFrame() {
    if (_capture == 0) {
        if ((_capture = cvCaptureFromCAM(-1)) == 0) {
            printLog("Failed to open webcam.\n");
            return;
        }
        const int IDEAL_FRAME_WIDTH = 320;
        const int IDEAL_FRAME_HEIGHT = 240;
        cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH, IDEAL_FRAME_WIDTH);
        cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT, IDEAL_FRAME_HEIGHT);
        
#ifdef __APPLE__
        configureCamera(0x5ac, 0x8510, false, 0.99, 0.5, 0.5, 0.5, true, 0.5);
#endif
    }
    IplImage* image = cvQueryFrame(_capture);
    if (image == 0) {
        // try again later
        QMetaObject::invokeMethod(this, "grabFrame", Qt::QueuedConnection);
        return;
    }
    // make sure it's in the format we expect
    if (image->nChannels != 3 || image->depth != IPL_DEPTH_8U || image->dataOrder != IPL_DATA_ORDER_PIXEL ||
            image->origin != 0) {
        printLog("Invalid webcam image format.\n");
        return;
    }
    QMetaObject::invokeMethod(Application::getInstance()->getWebcam(), "setFrame", Q_ARG(void*, image));
}
