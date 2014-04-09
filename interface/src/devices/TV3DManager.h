//
//  TV3DManager.h
//  interface/src/devices
//
//  Created by Brad Hefta-Gaub on 12/24/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
