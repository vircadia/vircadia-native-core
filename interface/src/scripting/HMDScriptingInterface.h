//
//  HMDScriptingInterface.h
//  interface/src/scripting
//
//  Created by Thijs Wenker on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HMDScriptingInterface_h
#define hifi_HMDScriptingInterface_h

#include <GLMHelpers.h>

#include "Application.h"
#include "devices/OculusManager.h"

class HMDScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool magnifier READ getMagnifier)
    Q_PROPERTY(bool active READ isHMDMode)
    Q_PROPERTY(float getHUDLookAtPositionX READ getHUDLookAtPositionX)
    Q_PROPERTY(float getHUDLookAtPositionY READ getHUDLookAtPositionY)
public:
    static HMDScriptingInterface& getInstance();

public slots:
    void toggleMagnifier() { Application::getInstance()->getApplicationOverlay().toggleMagnifier(); };

private:
    HMDScriptingInterface() {};
    bool getMagnifier() const { return Application::getInstance()->getApplicationOverlay().hasMagnifier(); };
    bool isHMDMode() const { return Application::getInstance()->isHMDMode(); }
    float getHUDLookAtPositionX() const {
        float pitch, yaw, roll;
        OculusManager::getEulerAngles(yaw, pitch, roll);
        glm::vec3 relativePos = OculusManager::getRelativePosition();
        glm::quat rotation = ::rotationBetween(glm::vec3(), relativePos);
        glm::vec2 screenPos = Application::getInstance()->getApplicationOverlay().sphericalToOverlay(glm::vec2(yaw, -pitch));
        
        return screenPos.x;
    }
    float getHUDLookAtPositionY() const {
        float pitch, yaw, roll;
        OculusManager::getEulerAngles(yaw, pitch, roll);
        glm::vec2 screenPos = Application::getInstance()->getApplicationOverlay().sphericalToOverlay(glm::vec2(yaw, -pitch));
        return screenPos.y;
    }

};

#endif // hifi_HMDScriptingInterface_h
