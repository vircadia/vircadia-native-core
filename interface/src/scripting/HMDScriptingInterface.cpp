//
//  HMDScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Thijs Wenker on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HMDScriptingInterface.h"

HMDScriptingInterface& HMDScriptingInterface::getInstance() {
    static HMDScriptingInterface sharedInstance;
    return sharedInstance;
}

glm::vec3 HMDScriptingInterface::getHUDLookAtPosition3D() const {
    Camera* camera = Application::getInstance()->getCamera();
    glm::vec3 position = camera->getPosition();
    glm::quat orientation = camera->getOrientation();

    glm::vec3 direction = orientation * glm::vec3(0.0f, 0.0f, -1.0f);

    ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();

    glm::vec3 result;
    applicationOverlay.calculateRayUICollisionPoint(position, direction, result);
    return result;
}

glm::vec2 HMDScriptingInterface::getHUDLookAtPosition2D() const {
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    glm::vec3 sphereCenter = myAvatar->getDefaultEyePosition();

    glm::vec3 hudIntersection = getHUDLookAtPosition3D();

    glm::vec3 direction = glm::inverse(myAvatar->getOrientation()) * (hudIntersection - sphereCenter);
    glm::quat rotation = ::rotationBetween(glm::vec3(0.0f, 0.0f, -1.0f), direction);
    glm::vec3 eulers = ::safeEulerAngles(rotation);

    return Application::getInstance()->getApplicationOverlay().sphericalToOverlay(glm::vec2(eulers.y, -eulers.x));
}
