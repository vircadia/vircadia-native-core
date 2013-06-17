//
//  Webcam.h
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Webcam__
#define __interface__Webcam__

class CvCapture;

class Webcam {
public:
    
    Webcam();
    ~Webcam();

private:
    
    CvCapture* _capture;
};

#endif /* defined(__interface__Webcam__) */
