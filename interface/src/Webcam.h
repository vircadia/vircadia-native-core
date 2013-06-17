//
//  Webcam.h
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Webcam__
#define __interface__Webcam__

#include "InterfaceConfig.h"

struct CvCapture;

class Webcam {
public:
    
    Webcam();
    ~Webcam();

    void init();

    int getFrameWidth() const { return _frameWidth; }
    int getFrameHeight() const { return _frameHeight; }
    float getFrameAspectRatio() const { return _frameWidth / (float)_frameHeight; }
    GLuint getFrameTextureID() const { return _frameTextureID; }

    void grabFrame();

private:
    
    CvCapture* _capture;
    
    int _frameWidth;
    int _frameHeight;
    GLuint _frameTextureID;
};

#endif /* defined(__interface__Webcam__) */
