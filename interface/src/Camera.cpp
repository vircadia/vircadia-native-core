//
//  Camera.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <SharedUtil.h>
#include <VoxelConstants.h>
#include <EventTypes.h>

#include "Application.h"
#include "Camera.h"
#include "Menu.h"
#include "Util.h"
#include "devices/OculusManager.h"


CameraMode stringToMode(const QString& mode) {
    if (mode == "third person") {
        return CAMERA_MODE_THIRD_PERSON;
    } else if (mode == "first person") {
        return CAMERA_MODE_FIRST_PERSON;
    } else if (mode == "mirror") {
        return CAMERA_MODE_MIRROR;
    } else if (mode == "independent") {
        return CAMERA_MODE_INDEPENDENT;
    }
    return CAMERA_MODE_NULL;
}

QString modeToString(CameraMode mode) {
    if (mode == CAMERA_MODE_THIRD_PERSON) {
        return "third person";
    } else if (mode == CAMERA_MODE_FIRST_PERSON) {
        return "first person";
    } else if (mode == CAMERA_MODE_MIRROR) {
        return "mirror";
    } else if (mode == CAMERA_MODE_INDEPENDENT) {
        return "independent";
    }
    return "unknown";
}

Camera::Camera() : 
    _mode(CAMERA_MODE_THIRD_PERSON),
    _position(0.0f, 0.0f, 0.0f),
    _fieldOfView(DEFAULT_FIELD_OF_VIEW_DEGREES),
    _aspectRatio(16.0f/9.0f),
    _nearClip(DEFAULT_NEAR_CLIP), // default
    _farClip(DEFAULT_FAR_CLIP), // default
    _hmdPosition(),
    _hmdRotation(),
    _targetPosition(),
    _targetRotation(),
    _scale(1.0f)
{
}

void Camera::update(float deltaTime) {
    return;
}

float Camera::getFarClip() const {
    return (_scale * _farClip < std::numeric_limits<int16_t>::max())
            ? _scale * _farClip
            : std::numeric_limits<int16_t>::max() - 1;
}

void Camera::setMode(CameraMode mode) {
    _mode = mode;
    emit modeUpdated(modeToString(mode));
}


void Camera::setFieldOfView(float f) { 
    _fieldOfView = f; 
}

void Camera::setAspectRatio(float a) { 
    _aspectRatio = a;
}

void Camera::setNearClip(float n) { 
    _nearClip = n;
}

void Camera::setFarClip(float f) { 
    _farClip = f;
}

PickRay Camera::computePickRay(float x, float y) {
    float screenWidth = Application::getInstance()->getGLWidget()->width();
    float screenHeight = Application::getInstance()->getGLWidget()->height();
    PickRay result;
    if (OculusManager::isConnected()) {
        result.origin = getPosition();
        Application::getInstance()->getApplicationOverlay().computeOculusPickRay(x / screenWidth, y / screenHeight, result.direction);
    } else {
        Application::getInstance()->getViewFrustum()->computePickRay(x / screenWidth, y / screenHeight,
                                                                     result.origin, result.direction);
    }
    return result;
}

PickRay Camera::computeViewPickRay(float xRatio, float yRatio) {
    PickRay result;
    Application::getInstance()->getViewFrustum()->computePickRay(xRatio, yRatio, result.origin, result.direction);
    return result;
}

void Camera::setModeString(const QString& mode) {
    CameraMode targetMode = stringToMode(mode);
    
    switch (targetMode) {
        case CAMERA_MODE_THIRD_PERSON:
            Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, false);
            Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, false);
            break;
        case CAMERA_MODE_MIRROR:
            Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, true);
            Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, false);
            break;
        case CAMERA_MODE_INDEPENDENT:
            Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, false);
            Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, false);
            break;
        default:
            break;
    }
    
    if (_mode != targetMode) {
        setMode(targetMode);
    }
}

QString Camera::getModeString() const {
    return modeToString(_mode);
}
