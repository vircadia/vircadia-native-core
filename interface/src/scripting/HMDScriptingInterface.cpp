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

#include <avatar/AvatarManager.h>

HMDScriptingInterface& HMDScriptingInterface::getInstance() {
    static HMDScriptingInterface sharedInstance;
    return sharedInstance;
}

bool HMDScriptingInterface::getHUDLookAtPosition3D(glm::vec3& result) const {
    Camera* camera = Application::getInstance()->getCamera();
    glm::vec3 position = camera->getPosition();
    glm::quat orientation = camera->getOrientation();

    glm::vec3 direction = orientation * glm::vec3(0.0f, 0.0f, -1.0f);

    ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();

    return applicationOverlay.calculateRayUICollisionPoint(position, direction, result);
}

QScriptValue HMDScriptingInterface::getHUDLookAtPosition2D(QScriptContext* context, QScriptEngine* engine) {

    glm::vec3 hudIntersection;

    if ((&HMDScriptingInterface::getInstance())->getHUDLookAtPosition3D(hudIntersection)) {
        MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        glm::vec3 sphereCenter = myAvatar->getDefaultEyePosition();
        glm::vec3 direction = glm::inverse(myAvatar->getOrientation()) * (hudIntersection - sphereCenter);
        glm::quat rotation = ::rotationBetween(glm::vec3(0.0f, 0.0f, -1.0f), direction);
        glm::vec3 eulers = ::safeEulerAngles(rotation);
        return qScriptValueFromValue<glm::vec2>(engine, Application::getInstance()->getApplicationOverlay()
                                                .sphericalToOverlay(glm::vec2(eulers.y, -eulers.x)));
    }
    return QScriptValue::NullValue;
}

QScriptValue HMDScriptingInterface::getHUDLookAtPosition3D(QScriptContext* context, QScriptEngine* engine) {
    glm::vec3 result;
    HMDScriptingInterface* hmdInterface = &HMDScriptingInterface::getInstance();
    if ((&HMDScriptingInterface::getInstance())->getHUDLookAtPosition3D(result)) {
        return qScriptValueFromValue<glm::vec3>(engine, result);
    }
    return QScriptValue::NullValue;
}
