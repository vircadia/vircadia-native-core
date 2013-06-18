//
//  Webcam.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QtDebug>

#include <opencv2/opencv.hpp>

#include <Log.h>

#include "Application.h"
#include "Webcam.h"

class FrameGrabber : public QObject {
public:
    
    FrameGrabber() : _capture(0) { }
    virtual ~FrameGrabber();

protected:
    
    virtual void timerEvent(QTimerEvent* event);
    
private:
    
    CvCapture* _capture;
};

FrameGrabber::~FrameGrabber() {
    if (_capture != 0) {
        cvReleaseCapture(&_capture);
    }
}

void FrameGrabber::timerEvent(QTimerEvent* event) {
    if (_capture == 0) {
        if ((_capture = cvCaptureFromCAM(-1)) == 0) {
            printLog("Failed to open webcam.\n");
            return;
        }
    }
    IplImage* image = cvQueryFrame(_capture);
    if (image == 0) {
        return;
    }
    // make sure it's in the format we expect
    if ((image->nChannels != 3 && image->nChannels != 4) || image->depth != IPL_DEPTH_8U ||
            image->dataOrder != IPL_DATA_ORDER_PIXEL || image->origin != 0) {
        printLog("Invalid webcam image format.\n");
        return;
    }
    QMetaObject::invokeMethod(Application::getInstance()->getWebcam(), "setFrame",
        Q_ARG(QImage, QImage((uchar*)image->imageData, image->width, image->height, image->widthStep,
            image->nChannels == 3 ? QImage::Format_RGB888 : QImage::Format_ARGB32).copy(0, 0, image->width, image->height)));
}

Webcam::Webcam() : _frameTextureID(0) {
    // the grabber simply runs as fast as possible
    _grabber = new FrameGrabber();
    _grabber->startTimer(0);
    _grabber->moveToThread(&_grabberThread);
}

void Webcam::init() {
    // start the grabber thread
    _grabberThread.start();
}

Webcam::~Webcam() {
    // stop the grabber thread
    _grabberThread.quit();
    _grabberThread.wait();
    
    delete _grabber;
}

void Webcam::setFrame(const QImage& image) {
    GLenum format = (image.format() == QImage::Format_RGB888) ? GL_BGR : GL_BGRA;
    if (_frameTextureID == 0) {
        glGenTextures(1, &_frameTextureID);
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _frameWidth = image.width(), _frameHeight = image.height(), 0, format,
            GL_UNSIGNED_BYTE, image.constBits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    } else {
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _frameWidth, _frameHeight, format, GL_UNSIGNED_BYTE, image.constBits());    
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

