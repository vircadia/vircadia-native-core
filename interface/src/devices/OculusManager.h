//
//  OculusManager.h
//  interface/src/devices
//
//  Created by Stephen Birarda on 5/9/13.
//  Refactored by Ben Arnold on 6/30/2014
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OculusManager_h
#define hifi_OculusManager_h

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QSize>

#include "RenderArgs.h"

class QOpenGLContext;
class QGLWidget;
class Camera;

/// Handles interaction with the Oculus Rift.
class OculusManager {
public:
    static void connect(QOpenGLContext* shareContext);
    static void disconnect();
    static bool isConnected();
    static void recalibrate();
    static void abandonCalibration();
    static void beginFrameTiming();
    static void endFrameTiming();
    static bool allowSwap();
    static void configureCamera(Camera& camera);
    static void display(QGLWidget * glCanvas, RenderArgs* renderArgs, const glm::quat &bodyOrientation, const glm::vec3 &position, Camera& whichCamera);
    static void reset();
    
    static glm::vec3 getRelativePosition();
    static glm::quat getOrientation();
    static QSize getRenderTargetSize();
    
    static void overrideOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane);
    
    static glm::vec3 getLeftEyePosition();
    static glm::vec3 getRightEyePosition();
    static glm::vec3 getMidEyePosition();
    
    static int getHMDScreen();

    static glm::mat4 getEyeProjection(int eye);
    static glm::mat4 getEyePose(int eye);
    static glm::mat4 getHeadPose();
};

#endif // hifi_OculusManager_h
