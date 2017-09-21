//
//  Haze.cpp
//  libraries/model/src/model
//
//  Created by Nissim Hadar on 9/13/2017.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <memory>
#include <gpu/Resource.h>

#include "Haze.h"

using namespace model;

Haze::Haze() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

enum HazeModes {
    HAZE_MODE_IS_ACTIVE                        = 1 << 0,
    HAZE_MODE_IS_ALTITUDE_BASED                = 1 << 1,
    HAZE_MODE_IS_DIRECTIONAL_LIGHT_ATTENUATED  = 1 << 2,
    HAZE_MODE_IS_MODULATE_COLOR                = 1 << 3
};


// For color modulated mode, the colour values are used as range values, which are then converted to range factors
// This is separate for each colour.
// The colour value is converted from [0.0 .. 1.0] to [5.0 .. 3000.0]
void Haze::setHazeColor(const glm::vec3 hazeColor) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.hazeColor.r != hazeColor.r) {
        _parametersBuffer.edit<Parameters>().hazeColor.r = hazeColor.r;

        float range = hazeColor.r * 2995.0f + 5.0f;
        float factor = convertHazeRangeToHazeRangeFactor(range);
        _parametersBuffer.edit<Parameters>().colorModulationFactor.r = factor;
    }
    if (params.hazeColor.g != hazeColor.g) {
        _parametersBuffer.edit<Parameters>().hazeColor.g = hazeColor.g;

        float range = hazeColor.g * 2995.0f + 5.0f;
        float factor = convertHazeRangeToHazeRangeFactor(range);
        _parametersBuffer.edit<Parameters>().colorModulationFactor.g = factor;
    }

    if (params.hazeColor.b != hazeColor.b) {
        _parametersBuffer.edit<Parameters>().hazeColor.b = hazeColor.b;

        float range = hazeColor.b * 2995.0f + 5.0f;
        float factor = convertHazeRangeToHazeRangeFactor(range);
        _parametersBuffer.edit<Parameters>().colorModulationFactor.b = factor;
    }
}

void Haze::setDirectionalLightBlend(const float directionalLightBlend) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.directionalLightBlend != directionalLightBlend) {
        _parametersBuffer.edit<Parameters>().directionalLightBlend = directionalLightBlend;
    }
}

void Haze::setDirectionalLightColor(const glm::vec3 directionalLightColor) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.directionalLightColor.r != directionalLightColor.r) {
        _parametersBuffer.edit<Parameters>().directionalLightColor.r = directionalLightColor.r;
    }
    if (params.directionalLightColor.g != directionalLightColor.g) {
        _parametersBuffer.edit<Parameters>().directionalLightColor.g = directionalLightColor.g;
    }
    if (params.directionalLightColor.b != directionalLightColor.b) {
        _parametersBuffer.edit<Parameters>().directionalLightColor.b = directionalLightColor.b;
    }
}
void Haze::setIsHazeActive(const bool isHazeActive) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_ACTIVE) == HAZE_MODE_IS_ACTIVE )&& !isHazeActive) {
        _parametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_ACTIVE;
    }
    else if (((params.hazeMode & HAZE_MODE_IS_ACTIVE) != HAZE_MODE_IS_ACTIVE) && isHazeActive) {
        _parametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_ACTIVE;
    }
}

void Haze::setIsAltitudeBased(const bool isAltitudeBased) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_ALTITUDE_BASED) == HAZE_MODE_IS_ALTITUDE_BASED )&& !isAltitudeBased) {
        _parametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_ALTITUDE_BASED;
    }
    else if (((params.hazeMode & HAZE_MODE_IS_ALTITUDE_BASED) != HAZE_MODE_IS_ALTITUDE_BASED) && isAltitudeBased) {
        _parametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_ALTITUDE_BASED;
    }
}

void Haze::setIsDirectionaLightAttenuationActive(const bool isDirectionaLightAttenuationActive) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_DIRECTIONAL_LIGHT_ATTENUATED) == HAZE_MODE_IS_DIRECTIONAL_LIGHT_ATTENUATED ) && !isDirectionaLightAttenuationActive) {
        _parametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_DIRECTIONAL_LIGHT_ATTENUATED;
    }
    else if (((params.hazeMode & HAZE_MODE_IS_DIRECTIONAL_LIGHT_ATTENUATED) != HAZE_MODE_IS_DIRECTIONAL_LIGHT_ATTENUATED) && isDirectionaLightAttenuationActive) {
        _parametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_DIRECTIONAL_LIGHT_ATTENUATED;
    }
}

void Haze::setIsModulateColorActive(const bool isModulateColorActive) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (((params.hazeMode & HAZE_MODE_IS_MODULATE_COLOR) == HAZE_MODE_IS_MODULATE_COLOR ) && !isModulateColorActive) {
        _parametersBuffer.edit<Parameters>().hazeMode &= ~HAZE_MODE_IS_MODULATE_COLOR;
    }
    else if (((params.hazeMode & HAZE_MODE_IS_MODULATE_COLOR) != HAZE_MODE_IS_MODULATE_COLOR) && isModulateColorActive) {
        _parametersBuffer.edit<Parameters>().hazeMode |= HAZE_MODE_IS_MODULATE_COLOR;
    }
}

void Haze::setHazeRangeFactor(const float hazeRangeFactor) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.hazeRangeFactor != hazeRangeFactor) {
        _parametersBuffer.edit<Parameters>().hazeRangeFactor = hazeRangeFactor;
    }
}

void Haze::setHazeAltitudeFactor(const float hazeAltitudeFactor) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.hazeAltitudeFactor != hazeAltitudeFactor) {
        _parametersBuffer.edit<Parameters>().hazeAltitudeFactor = hazeAltitudeFactor;
    }
}

void Haze::setHazeRangeFactorKeyLight(const float hazeRangeFactorKeyLight) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.hazeRangeFactorKeyLight != hazeRangeFactorKeyLight) {
        _parametersBuffer.edit<Parameters>().hazeRangeFactorKeyLight = hazeRangeFactorKeyLight;
    }
}

void Haze::setHazeAltitudeFactorKeyLight(const float hazeAltitudeFactorKeyLight) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.hazeAltitudeFactorKeyLight != hazeAltitudeFactorKeyLight) {
        _parametersBuffer.edit<Parameters>().hazeAltitudeFactorKeyLight = hazeAltitudeFactorKeyLight;
    }
}
void Haze::setHazeBaseReference(const float hazeBaseReference) {
    auto& params = _parametersBuffer.get<Parameters>();

    if (params.hazeBaseReference != hazeBaseReference) {
        _parametersBuffer.edit<Parameters>().hazeBaseReference = hazeBaseReference;
    }
}
