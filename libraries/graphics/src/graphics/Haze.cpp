//
//  Haze.cpp
//  libraries/graphics/src/graphics
//
//  Created by Nissim Hadar on 9/13/2017.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <memory>
#include "Haze.h"

using namespace graphics;

const float Haze::INITIAL_HAZE_RANGE{ 1000.0f };
const float Haze::INITIAL_HAZE_HEIGHT{ 200.0f };

const float Haze::INITIAL_KEY_LIGHT_RANGE{ 1000.0f };
const float Haze::INITIAL_KEY_LIGHT_ALTITUDE{ 200.0f };

const float Haze::INITIAL_HAZE_BACKGROUND_BLEND{ 0.0f };

const glm::vec3 Haze::INITIAL_HAZE_COLOR{ 0.5f, 0.6f, 0.7f }; // Bluish

const float Haze::INITIAL_HAZE_GLARE_ANGLE{ 20.0f };

const glm::vec3 Haze::INITIAL_HAZE_GLARE_COLOR{ 1.0f, 0.9f, 0.7f };

const float Haze::INITIAL_HAZE_BASE_REFERENCE{ 0.0f };

const float Haze::LOG_P_005{ logf(0.05f)};
const float Haze::LOG_P_05{ logf(0.5f) };

Haze::Haze() {
    Parameters parameters;
    _hazeParametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

enum HazeModes {
    HAZE_MODE_IS_ACTIVE                        = 1 << 0,
    HAZE_MODE_IS_ALTITUDE_BASED                = 1 << 1,
    HAZE_MODE_IS_KEYLIGHT_ATTENUATED           = 1 << 2,
    HAZE_MODE_IS_MODULATE_COLOR                = 1 << 3,
    HAZE_MODE_IS_ENABLE_LIGHT_BLEND            = 1 << 4
};

// For color modulated mode, the colour values are used as range values, which are then converted to range factors
// This is separate for each colour.
// The colour value is converted from [0.0 .. 1.0] to [5.0 .. 3000.0]
const float OFFSET = 5.0f;
const float BIAS = (3000.0f - 5.0f) / (1.0f - 0.0f);
void Haze::setHazeColor(const glm::vec3 hazeColor) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeColor != hazeColor) {
        _hazeParametersBuffer.edit<Parameters>().hazeColor = hazeColor;

        glm::vec3 range = hazeColor * BIAS + OFFSET;
        glm::vec3 factor = convertHazeRangeToHazeRangeFactor(range);
        _hazeParametersBuffer.edit<Parameters>().colorModulationFactor = factor;
    }
 }

void Haze::setHazeEnableGlare(const bool isHazeEnableGlare) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_ENABLE_LIGHT_BLEND) == HAZE_MODE_IS_ENABLE_LIGHT_BLEND) && !isHazeEnableGlare) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_ENABLE_LIGHT_BLEND;
    } else if (((params.hazeMode & HAZE_MODE_IS_ENABLE_LIGHT_BLEND) != HAZE_MODE_IS_ENABLE_LIGHT_BLEND) && isHazeEnableGlare) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_ENABLE_LIGHT_BLEND;
    }
}

void Haze::setHazeGlareBlend(const float hazeGlareBlend) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeGlareBlend != hazeGlareBlend) {
        _hazeParametersBuffer.edit<Parameters>().hazeGlareBlend = hazeGlareBlend;
    }
}

void Haze::setHazeGlareColor(const glm::vec3 hazeGlareColor) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeGlareColor.r != hazeGlareColor.r) {
        _hazeParametersBuffer.edit<Parameters>().hazeGlareColor.r = hazeGlareColor.r;
    }
    if (params.hazeGlareColor.g != hazeGlareColor.g) {
        _hazeParametersBuffer.edit<Parameters>().hazeGlareColor.g = hazeGlareColor.g;
    }
    if (params.hazeGlareColor.b != hazeGlareColor.b) {
        _hazeParametersBuffer.edit<Parameters>().hazeGlareColor.b = hazeGlareColor.b;
    }
}
void Haze::setHazeActive(const bool isHazeActive) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_ACTIVE) == HAZE_MODE_IS_ACTIVE )&& !isHazeActive) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_ACTIVE;
    } else if (((params.hazeMode & HAZE_MODE_IS_ACTIVE) != HAZE_MODE_IS_ACTIVE) && isHazeActive) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_ACTIVE;
    }
}

void Haze::setAltitudeBased(const bool isAltitudeBased) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_ALTITUDE_BASED) == HAZE_MODE_IS_ALTITUDE_BASED )&& !isAltitudeBased) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_ALTITUDE_BASED;
    } else if (((params.hazeMode & HAZE_MODE_IS_ALTITUDE_BASED) != HAZE_MODE_IS_ALTITUDE_BASED) && isAltitudeBased) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_ALTITUDE_BASED;
    }
}

void Haze::setHazeAttenuateKeyLight(const bool isHazeAttenuateKeyLight) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_KEYLIGHT_ATTENUATED) == HAZE_MODE_IS_KEYLIGHT_ATTENUATED) && !isHazeAttenuateKeyLight) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_KEYLIGHT_ATTENUATED;
    } else if (((params.hazeMode & HAZE_MODE_IS_KEYLIGHT_ATTENUATED) != HAZE_MODE_IS_KEYLIGHT_ATTENUATED) && isHazeAttenuateKeyLight) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_KEYLIGHT_ATTENUATED;
    }
}

void Haze::setModulateColorActive(const bool isModulateColorActive) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_MODULATE_COLOR) == HAZE_MODE_IS_MODULATE_COLOR ) && !isModulateColorActive) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_MODULATE_COLOR;
    } else if (((params.hazeMode & HAZE_MODE_IS_MODULATE_COLOR) != HAZE_MODE_IS_MODULATE_COLOR) && isModulateColorActive) {
        _hazeParametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_MODULATE_COLOR;
    }
}

void Haze::setHazeRangeFactor(const float hazeRangeFactor) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeRangeFactor != hazeRangeFactor) {
        _hazeParametersBuffer.edit<Parameters>().hazeRangeFactor = hazeRangeFactor;
    }
}

void Haze::setHazeAltitudeFactor(const float hazeHeightFactor) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeHeightFactor != hazeHeightFactor) {
        _hazeParametersBuffer.edit<Parameters>().hazeHeightFactor = hazeHeightFactor;
    }
}

void Haze::setHazeKeyLightRangeFactor(const float hazeKeyLightRangeFactor) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeKeyLightRangeFactor != hazeKeyLightRangeFactor) {
        _hazeParametersBuffer.edit<Parameters>().hazeKeyLightRangeFactor = hazeKeyLightRangeFactor;
    }
}

void Haze::setHazeKeyLightAltitudeFactor(const float hazeKeyLightAltitudeFactor) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeKeyLightAltitudeFactor != hazeKeyLightAltitudeFactor) {
        _hazeParametersBuffer.edit<Parameters>().hazeKeyLightAltitudeFactor = hazeKeyLightAltitudeFactor;
    }
}

void Haze::setHazeBaseReference(const float hazeBaseReference) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeBaseReference != hazeBaseReference) {
        _hazeParametersBuffer.edit<Parameters>().hazeBaseReference = hazeBaseReference;
    }
}

void Haze::setHazeBackgroundBlend(const float hazeBackgroundBlend) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.hazeBackgroundBlend != hazeBackgroundBlend) {
        _hazeParametersBuffer.edit<Parameters>().hazeBackgroundBlend = hazeBackgroundBlend;
    }
}

void Haze::setTransform(const glm::mat4& transform) {
    auto& params = _hazeParametersBuffer.get<Parameters>();

    if (params.transform != transform) {
        _hazeParametersBuffer.edit<Parameters>().transform = transform;
    }
}

