//
//  VisionSqueeze.cpp
//  interface/src
//
//  Created by Seth Alves on 2019-3-13.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VisionSqueeze.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/epsilon.hpp>

#include <display-plugins/hmd/HmdDisplayPlugin.h>

#include "Application.h"

VisionSqueeze::VisionSqueeze() :
    _visionSqueezeEnabled(_visionSqueezeEnabledSetting.get()),
    _visionSqueezeRatioX(_visionSqueezeRatioXSetting.get()),
    _visionSqueezeRatioY(_visionSqueezeRatioYSetting.get()),
    _visionSqueezeUnSqueezeDelay(_visionSqueezeUnSqueezeDelaySetting.get()),
    _visionSqueezeUnSqueezeSpeed(_visionSqueezeUnSqueezeSpeedSetting.get()),
    _visionSqueezeTransition(_visionSqueezeTransitionSetting.get()),
    _visionSqueezePerEye(_visionSqueezePerEyeSetting.get()),
    _visionSqueezeGroundPlaneY(_visionSqueezeGroundPlaneYSetting.get()),
    _visionSqueezeSpotlightSize(_visionSqueezeSpotlightSizeSetting.get()),
    _visionSqueezeTurningXFactor(_visionSqueezeTurningXFactorSetting.get()),
    _visionSqueezeTurningYFactor(_visionSqueezeTurningYFactorSetting.get()) {
}

void VisionSqueeze::setVisionSqueezeEnabled(bool value) {
    if (value != _visionSqueezeEnabled) {
        _visionSqueezeEnabled = value;
        _visionSqueezeEnabledSetting.set(_visionSqueezeEnabled);
    }
}

void VisionSqueeze::setVisionSqueezeRatioX(float value) {
    if (value != _visionSqueezeRatioX) {
        _visionSqueezeRatioX = value;
        _visionSqueezeRatioXSetting.set(_visionSqueezeRatioX);
    }
}

void VisionSqueeze::setVisionSqueezeRatioY(float value) {
    if (value != _visionSqueezeRatioY) {
        _visionSqueezeRatioY = value;
        _visionSqueezeRatioYSetting.set(_visionSqueezeRatioY);
    }
}

void VisionSqueeze::setVisionSqueezeUnSqueezeDelay(float value) {
    if (value != _visionSqueezeUnSqueezeDelay) {
        _visionSqueezeUnSqueezeDelay = value;
        _visionSqueezeUnSqueezeDelaySetting.set(_visionSqueezeUnSqueezeDelay);
    }
}

void VisionSqueeze::setVisionSqueezeUnSqueezeSpeed(float value) {
    if (value != _visionSqueezeUnSqueezeSpeed) {
        _visionSqueezeUnSqueezeSpeed = value;
        _visionSqueezeUnSqueezeSpeedSetting.set(_visionSqueezeUnSqueezeSpeed);
    }
}

void VisionSqueeze::setVisionSqueezeTransition(float value) {
    if (value != _visionSqueezeTransition) {
        _visionSqueezeTransition = value;
        _visionSqueezeTransitionSetting.set(_visionSqueezeTransition);
    }
}

void VisionSqueeze::setVisionSqueezePerEye(int value) {
    if (value != _visionSqueezePerEye) {
        _visionSqueezePerEye = value;
        _visionSqueezePerEyeSetting.set(_visionSqueezePerEye);
    }
}

void VisionSqueeze::setVisionSqueezeGroundPlaneY(float value) {
    if (value != _visionSqueezeGroundPlaneY) {
        _visionSqueezeGroundPlaneY = value;
        _visionSqueezeGroundPlaneYSetting.set(_visionSqueezeGroundPlaneY);
    }
}

void VisionSqueeze::setVisionSqueezeSpotlightSize(float value) {
    if (value != _visionSqueezeSpotlightSize) {
        _visionSqueezeSpotlightSize = value;
        _visionSqueezeSpotlightSizeSetting.set(_visionSqueezeSpotlightSize);
    }
}

void VisionSqueeze::setVisionSqueezeTurningXFactor(float value) {
    if (value != _visionSqueezeTurningXFactor) {
        _visionSqueezeTurningXFactor = value;
        _visionSqueezeTurningXFactorSetting.set(_visionSqueezeTurningXFactor);
    }
}

void VisionSqueeze::setVisionSqueezeTurningYFactor(float value) {
    if (value != _visionSqueezeTurningYFactor) {
        _visionSqueezeTurningYFactor = value;
        _visionSqueezeTurningYFactorSetting.set(_visionSqueezeTurningYFactor);
    }
}

void VisionSqueeze::updateVisionSqueeze(const glm::mat4& sensorToWorldMatrix, float deltaTime) {

    const float SENSOR_TO_WORLD_TRANS_EPSILON = 0.0001f;
    const float SENSOR_TO_WORLD_TRANS_Y_EPSILON = 0.01f;
    const float SENSOR_TO_WORLD_TRANS_ITS_A_TELEPORT_SQUARED = 2.0f;
    const float SENSOR_TO_WORLD_ROT_EPSILON = 0.000005f;
    const float SENSOR_TO_WORLD_ROT_ITS_A_SNAP_TURN = 0.99f;
    const float VISION_SQUEEZE_TP_LOCKOUT = 0.1f; // seconds

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(sensorToWorldMatrix, scale, rotation, translation, skew, perspective);

    if (!_visionSqueezeEnabled) {
        _squeezeVision = false;
        _squeezeVisionTurning = false;
    } else if (_visionSqueezeLockout > 0.0f) {
        _visionSqueezeLockout -= deltaTime;
    } else {
        _squeezeVision = false;
        _squeezeVisionTurning = false;
        glm::vec3 absTransDelta = glm::abs(translation - _prevTranslation);
        float rotDot = fabsf(glm::dot(rotation, _prevRotation));

        // if the avatar has just teleported or snap-turned, briefly disable triggering of vision-squeeze
        if (glm::length2(translation - _prevTranslation) > SENSOR_TO_WORLD_TRANS_ITS_A_TELEPORT_SQUARED ||
            rotDot < SENSOR_TO_WORLD_ROT_ITS_A_SNAP_TURN) {
            _visionSqueezeLockout = VISION_SQUEEZE_TP_LOCKOUT;
            _squeezeVision = true;
            _squeezeVisionTurning = true;
        } else if (rotDot < 1.0f - SENSOR_TO_WORLD_ROT_EPSILON) {
            _squeezeVision = true;
            _squeezeVisionTurning = true;
        } else if (absTransDelta.x > SENSOR_TO_WORLD_TRANS_EPSILON ||
                   absTransDelta.y > SENSOR_TO_WORLD_TRANS_Y_EPSILON ||
                   absTransDelta.z > SENSOR_TO_WORLD_TRANS_EPSILON) {
            _squeezeVision = true;
            _squeezeVisionTurning = false;
        }
    }

    _prevTranslation = translation;
    _prevRotation = rotation;

    static quint64 lastSqueezeTime = 0;
    quint64 now = usecTimestampNow();
    static float visionSqueezeX = 0.0f; // 0.0 -- unobstructed, 1.0 -- fully blocked
    static float visionSqueezeY = 0.0f; // 0.0 -- unobstructed, 1.0 -- fully blocked

    if (_squeezeVision) {
        float ratioX = getVisionSqueezeRatioX();
        float ratioY = getVisionSqueezeRatioY();

        if (ratioX >= 0.0f) {
            if (_squeezeVisionTurning) {
                ratioX += (1.0f - ratioX) * getVisionSqueezeTurningXFactor();
            }
            float newVisionSqueezeX = ratioX;
            if (newVisionSqueezeX >= visionSqueezeX) {
                lastSqueezeTime = now;
                visionSqueezeX = newVisionSqueezeX;
            }
        } else {
            visionSqueezeX = -1.0f;
        }

        if (ratioY >= 0.0f) {
            float newVisionSqueezeY = ratioY;
            if (newVisionSqueezeY >= visionSqueezeY) {
                lastSqueezeTime = now;
                visionSqueezeY = newVisionSqueezeY;
            }
        } else {
            visionSqueezeY = -1.0f;
        }
    }

    float unsqueezeAmount = deltaTime * getVisionSqueezeUnSqueezeSpeed();
    if (now - lastSqueezeTime > getVisionSqueezeUnSqueezeDelay() * USECS_PER_SECOND) {
        visionSqueezeX -= unsqueezeAmount;
        if (visionSqueezeX < 0.0f) {
            visionSqueezeX = -1.0f;
        }
        visionSqueezeY -= unsqueezeAmount;
        if (visionSqueezeY < 0.0f) {
            visionSqueezeY = -1.0f;
        }
    }

    std::shared_ptr<HmdDisplayPlugin> hmdDisplayPlugin =
        std::dynamic_pointer_cast<HmdDisplayPlugin>(qApp->getActiveDisplayPlugin());
    if (hmdDisplayPlugin) {
        hmdDisplayPlugin->updateVisionSqueezeParameters(visionSqueezeX, visionSqueezeY,
                                                        getVisionSqueezeTransition(),
                                                        getVisionSqueezePerEye(),
                                                        getVisionSqueezeGroundPlaneY(),
                                                        getVisionSqueezeSpotlightSize());
    }
}
