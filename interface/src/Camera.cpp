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
    _hmdRotation()
{
}

void Camera::setPosition(const glm::vec3& position) {
    _position = position; 
}

void Camera::setRotation(const glm::quat& rotation) {
    _rotation = rotation; 
}

void Camera::setHmdPosition(const glm::vec3& hmdPosition) {
    _hmdPosition = hmdPosition; 
}

void Camera::setHmdRotation(const glm::quat& hmdRotation) {
    _hmdRotation = hmdRotation; 
}

float Camera::getFarClip() const {
    return (_farClip < std::numeric_limits<int16_t>::max())
            ? _farClip
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
