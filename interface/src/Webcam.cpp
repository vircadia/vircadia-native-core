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

Webcam::Webcam() : _frameTextureID(0) {
    // the grabber simply runs as fast as possible
    _grabber = new FrameGrabber();
    _grabber->moveToThread(&_grabberThread);
}

void Webcam::init() {
    // start the grabber thread
    _grabberThread.start();
    
    // let the grabber know we're ready for the first frame
    QMetaObject::invokeMethod(_grabber, "grabFrame");
}

void Webcam::renderPreview(int screenWidth, int screenHeight) {
    if (_frameTextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            const int PREVIEW_HEIGHT = 200;
            int previewWidth = _frameWidth * PREVIEW_HEIGHT / _frameHeight;
            int bottom = screenHeight - 400;
            int left = screenWidth - previewWidth - 400;
            
            glTexCoord2f(0, 0);
            glVertex2f(left, bottom);
            glTexCoord2f(1, 0);
            glVertex2f(left + previewWidth, bottom);
            glTexCoord2f(1, 1);
            glVertex2f(left + previewWidth, bottom + PREVIEW_HEIGHT);
            glTexCoord2f(0, 1);
            glVertex2f(left, bottom + PREVIEW_HEIGHT);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
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
    GLint internalFormat;
    GLenum format;
    if (img->nChannels == 3) {
        internalFormat = GL_RGB;
        format = GL_BGR;
        
    } else {
        internalFormat = GL_RGBA;
        format = GL_BGRA;
    }
    if (_frameTextureID == 0) {
        glGenTextures(1, &_frameTextureID);
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _frameWidth = img->width, _frameHeight = img->height, 0, format,
            GL_UNSIGNED_BYTE, img->imageData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    } else {
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _frameWidth, _frameHeight, format, GL_UNSIGNED_BYTE, img->imageData);    
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // let the grabber know we're ready for the next frame
    QMetaObject::invokeMethod(_grabber, "grabFrame");
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
    }
    IplImage* image = cvQueryFrame(_capture);
    if (image == 0) {
        // try again later
        QMetaObject::invokeMethod(this, "grabFrame", Qt::QueuedConnection);
        return;
    }
    // make sure it's in the format we expect
    if ((image->nChannels != 3 && image->nChannels != 4) || image->depth != IPL_DEPTH_8U ||
            image->dataOrder != IPL_DATA_ORDER_PIXEL || image->origin != 0) {
        printLog("Invalid webcam image format.\n");
        return;
    }
    QMetaObject::invokeMethod(Application::getInstance()->getWebcam(), "setFrame", Q_ARG(void*, image));
}
