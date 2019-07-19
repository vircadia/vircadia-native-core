//
//  VisionSqueeze.h
//  interface/src
//
//  Created by Seth Alves on 2019-3-13.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VisionSqueeze_h
#define hifi_VisionSqueeze_h

#include <memory>
#include <glm/glm.hpp>

#include <SettingHandle.h>

static const float DEFAULT_VISION_SQUEEZE_TURNING_X_FACTOR = 0.51f;
static const float DEFAULT_VISION_SQUEEZE_TURNING_Y_FACTOR = 0.36f;
static const float DEFAULT_VISION_SQUEEZE_UNSQUEEZE_DELAY = 0.2f; // seconds
static const float DEFAULT_VISION_SQUEEZE_UNSQUEEZE_SPEED = 3.0f;
static const float DEFAULT_VISION_SQUEEZE_TRANSITION = 0.25f;
static const int DEFAULT_VISION_SQUEEZE_PER_EYE = 1;
static const float DEFAULT_VISION_SQUEEZE_GROUND_PLANE_Y = 0.0f;
static const float DEFAULT_VISION_SQUEEZE_SPOTLIGHT_SIZE = 6.0f;


class VisionSqueeze {

public:

    VisionSqueeze();

    bool getVisionSqueezeEnabled() const { return _visionSqueezeEnabled; }
    void setVisionSqueezeEnabled(bool value);
    float getVisionSqueezeRatioX() const { return _visionSqueezeRatioX; }
    float getVisionSqueezeRatioY() const { return _visionSqueezeRatioY; }
    void setVisionSqueezeRatioX(float value);
    void setVisionSqueezeRatioY(float value);
    float getVisionSqueezeUnSqueezeDelay() const { return _visionSqueezeUnSqueezeDelay; }
    void setVisionSqueezeUnSqueezeDelay(float value);
    float getVisionSqueezeUnSqueezeSpeed() const { return _visionSqueezeUnSqueezeSpeed; }
    void setVisionSqueezeUnSqueezeSpeed(float value);
    float getVisionSqueezeTransition() const { return _visionSqueezeTransition; }
    void setVisionSqueezeTransition(float value);
    int getVisionSqueezePerEye() const { return _visionSqueezePerEye; }
    void setVisionSqueezePerEye(int value);
    float getVisionSqueezeGroundPlaneY() const { return _visionSqueezeGroundPlaneY; }
    void setVisionSqueezeGroundPlaneY(float value);
    float getVisionSqueezeSpotlightSize() const { return _visionSqueezeSpotlightSize; }
    void setVisionSqueezeSpotlightSize(float value);
    float getVisionSqueezeTurningXFactor() const { return _visionSqueezeTurningXFactor; }
    void setVisionSqueezeTurningXFactor(float value);
    float getVisionSqueezeTurningYFactor() const { return _visionSqueezeTurningYFactor; }
    void setVisionSqueezeTurningYFactor(float value);

    void updateVisionSqueeze(const glm::mat4& sensorToWorldMatrix, float deltaTime);

    // state variable accessors used by Application.cpp...
    bool getSqueezeVision() const { return _squeezeVision; }
    void setSqueezeVision(bool value) { _squeezeVision = value; }
    bool getSqueezeVisionTurning() const { return _squeezeVisionTurning; }
    void setSqueezeVisionTurning(bool value) { _squeezeVisionTurning = value; }

private:
    Setting::Handle<bool> _visionSqueezeEnabledSetting {"visionSqueezeEnabled", false};
    Setting::Handle<float> _visionSqueezeRatioXSetting {"visionSqueezeRatioX", 0.0f};
    Setting::Handle<float> _visionSqueezeRatioYSetting {"visionSqueezeRatioY", 0.0f};
    Setting::Handle<float> _visionSqueezeUnSqueezeDelaySetting {"visionSqueezeUnSqueezeDelay",
            DEFAULT_VISION_SQUEEZE_UNSQUEEZE_DELAY};
    Setting::Handle<float> _visionSqueezeUnSqueezeSpeedSetting {"visionSqueezeUnSqueezeSpeed",
            DEFAULT_VISION_SQUEEZE_UNSQUEEZE_SPEED};
    Setting::Handle<float> _visionSqueezeTransitionSetting {"visionSqueezeTransition", DEFAULT_VISION_SQUEEZE_TRANSITION};
    Setting::Handle<float> _visionSqueezePerEyeSetting {"visionSqueezePerEye", DEFAULT_VISION_SQUEEZE_PER_EYE};
    Setting::Handle<float> _visionSqueezeGroundPlaneYSetting {"visionSqueezeGroundPlaneY",
            DEFAULT_VISION_SQUEEZE_GROUND_PLANE_Y};
    Setting::Handle<float> _visionSqueezeSpotlightSizeSetting {"visionSqueezeSpotlightSize",
            DEFAULT_VISION_SQUEEZE_SPOTLIGHT_SIZE};
    Setting::Handle<float> _visionSqueezeTurningXFactorSetting {"visionSqueezeTurningXFactor",
            DEFAULT_VISION_SQUEEZE_TURNING_X_FACTOR};
    Setting::Handle<float> _visionSqueezeTurningYFactorSetting {"visionSqueezeTurningYFactor",
            DEFAULT_VISION_SQUEEZE_TURNING_Y_FACTOR};


    // these are readable and writable from the scripting interface (on a different thread), so make them atomic
    std::atomic<bool> _visionSqueezeEnabled { false };
    std::atomic<float> _visionSqueezeRatioX { 0.0f };
    std::atomic<float> _visionSqueezeRatioY { 0.0f };
    std::atomic<float> _visionSqueezeUnSqueezeDelay { DEFAULT_VISION_SQUEEZE_UNSQUEEZE_DELAY }; // seconds
    std::atomic<float> _visionSqueezeUnSqueezeSpeed { DEFAULT_VISION_SQUEEZE_UNSQUEEZE_SPEED };
    std::atomic<float> _visionSqueezeTransition { DEFAULT_VISION_SQUEEZE_TRANSITION };
    std::atomic<int> _visionSqueezePerEye { DEFAULT_VISION_SQUEEZE_PER_EYE };
    std::atomic<float> _visionSqueezeGroundPlaneY { DEFAULT_VISION_SQUEEZE_GROUND_PLANE_Y };
    std::atomic<float> _visionSqueezeSpotlightSize { DEFAULT_VISION_SQUEEZE_SPOTLIGHT_SIZE };
    std::atomic<float> _visionSqueezeTurningXFactor { DEFAULT_VISION_SQUEEZE_TURNING_X_FACTOR };
    std::atomic<float> _visionSqueezeTurningYFactor { DEFAULT_VISION_SQUEEZE_TURNING_Y_FACTOR };

    bool _squeezeVision { false };
    bool _squeezeVisionTurning { false };

    float _visionSqueezeLockout { 0.0 };
    glm::vec3 _prevTranslation;
    glm::quat _prevRotation;
};

#endif // hifi_VisionSqueeze_h
