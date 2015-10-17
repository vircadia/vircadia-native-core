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

#include <QtScript/QScriptContext>

#include "display-plugins/DisplayPlugin.h"
#include <avatar/AvatarManager.h>
#include "Application.h"

HMDScriptingInterface::HMDScriptingInterface() {
}

QScriptValue HMDScriptingInterface::getHUDLookAtPosition2D(QScriptContext* context, QScriptEngine* engine) {
    glm::vec3 hudIntersection;
    auto instance = DependencyManager::get<HMDScriptingInterface>();
    if (instance->getHUDLookAtPosition3D(hudIntersection)) {
        MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        glm::vec3 sphereCenter = myAvatar->getDefaultEyePosition();
        glm::vec3 direction = glm::inverse(myAvatar->getOrientation()) * (hudIntersection - sphereCenter);
        glm::quat rotation = ::rotationBetween(glm::vec3(0.0f, 0.0f, -1.0f), direction);
        glm::vec3 eulers = ::safeEulerAngles(rotation);
        return qScriptValueFromValue<glm::vec2>(engine, qApp->getApplicationCompositor()
            .sphericalToOverlay(glm::vec2(eulers.y, -eulers.x)));
    }
    return QScriptValue::NullValue;
}

QScriptValue HMDScriptingInterface::getHUDLookAtPosition3D(QScriptContext* context, QScriptEngine* engine) {
    glm::vec3 result;
    auto instance = DependencyManager::get<HMDScriptingInterface>();
    if (instance->getHUDLookAtPosition3D(result)) {
        return qScriptValueFromValue<glm::vec3>(engine, result);
    }
    return QScriptValue::NullValue;
}

void HMDScriptingInterface::toggleMagnifier() {
    qApp->getApplicationCompositor().toggleMagnifier();
}

bool HMDScriptingInterface::getMagnifier() const {
    return qApp->getApplicationCompositor().hasMagnifier();
}

bool HMDScriptingInterface::getHUDLookAtPosition3D(glm::vec3& result) const {
    Camera* camera = qApp->getCamera();
    glm::vec3 position = camera->getPosition();
    glm::quat orientation = camera->getOrientation();

    glm::vec3 direction = orientation * glm::vec3(0.0f, 0.0f, -1.0f);

    const auto& compositor = qApp->getApplicationCompositor();

    return compositor.calculateRayUICollisionPoint(position, direction, result);
}

glm::mat4 HMDScriptingInterface::getWorldHMDMatrix() const {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    return myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
}

glm::vec3 HMDScriptingInterface::getPosition() const {
    if (qApp->getActiveDisplayPlugin()->isHmd()) {
        return extractTranslation(getWorldHMDMatrix());
    }
    return glm::vec3();
}

glm::quat HMDScriptingInterface::getOrientation() const {
    if (qApp->getActiveDisplayPlugin()->isHmd()) {
        return glm::normalize(glm::quat_cast(getWorldHMDMatrix()));
    }
    return glm::quat();
}
