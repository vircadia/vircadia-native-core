//
//  TV3DManager.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/24/2013
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__TV3DManager__
#define __hifi__TV3DManager__

#include <iostream>

class Camera;

struct eyeFrustum {
    double left;
    double right;
    double bottom;
    double top;
    float modelTranslation;
};


/// Handles interaction with 3D TVs
class TV3DManager {
public:
    static void connect();
    static bool isConnected();
    static void configureCamera(Camera& camera, int screenWidth, int screenHeight);
    static void display(Camera& whichCamera);
private:    
    static void setFrustum(Camera& whichCamera);
    static int _screenWidth;
    static int _screenHeight;
    static double _aspect;
    static eyeFrustum _leftEye;
    static eyeFrustum _rightEye;
};

#endif /* defined(__hifi__TV3DManager__) */
