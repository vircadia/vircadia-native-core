//
//  Webcam.h
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Webcam__
#define __interface__Webcam__

#include <QObject>
#include <QThread>

#include "InterfaceConfig.h"

class QImage;

struct CvCapture;

class FrameGrabber;

class Webcam : public QObject {
    Q_OBJECT
    
public:
    
    Webcam();
    ~Webcam();

    void init();
    void renderPreview(int screenWidth, int screenHeight);

public slots:
    
    void setFrame(void* image);
    
private:
    
    QThread _grabberThread;
    FrameGrabber* _grabber;
    
    int _frameWidth;
    int _frameHeight;
    GLuint _frameTextureID;
};

class FrameGrabber : public QObject {
    Q_OBJECT
    
public:
    
    FrameGrabber() : _capture(0) { }
    virtual ~FrameGrabber();

public slots:
    
    void grabFrame();
    
private:
    
    CvCapture* _capture;
};

#endif /* defined(__interface__Webcam__) */
