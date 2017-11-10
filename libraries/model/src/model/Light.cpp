//
//  Light.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 1/26/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Light.h"

using namespace model;

Light::Light() {
    updateLightRadius();
}

Light::Light(const Light& light) :
    _flags(light._flags),
    _transform(light._transform)
{
}

Light& Light::operator= (const Light& light) {
    _flags = (light._flags);
    _lightSchemaBuffer = (light._lightSchemaBuffer);
    _ambientSchemaBuffer = (light._ambientSchemaBuffer);
    _transform = (light._transform);

    return (*this);
}

Light::~Light() {
}

void Light::setType(Type type) {
    if (_type != type) {
        _type = type;
        if (type != SPOT) {
            _lightSchemaBuffer.edit().volume.spotCos = -1.f;
        } else {
            _lightSchemaBuffer.edit().volume.spotCos = _spotCos;
        }
        updateLightRadius();
    }
}


void Light::setPosition(const Vec3& position) {
    _transform.setTranslation(position);
    _lightSchemaBuffer.edit().volume.position = position;
}

void Light::setOrientation(const glm::quat& orientation) {
    setDirection(orientation * glm::vec3(0.0f, 0.0f, -1.0f));
    _transform.setRotation(orientation);
}

void Light::setDirection(const Vec3& direction) {
    _lightSchemaBuffer.edit().volume.direction = (direction);
}

const Vec3& Light::getDirection() const {
    return _lightSchemaBuffer->volume.direction;
}

void Light::setColor(const Color& color) {
    _lightSchemaBuffer.edit().irradiance.color = color;
    updateLightRadius();
}

void Light::setIntensity(float intensity) {
    _lightSchemaBuffer.edit().irradiance.intensity = intensity;
    updateLightRadius();
}

void Light::setFalloffRadius(float radius) {
    if (radius <= 0.0f) {
        radius = 0.1f;
    }
    _lightSchemaBuffer.edit().irradiance.falloffRadius = radius;
    updateLightRadius();
}
void Light::setMaximumRadius(float radius) {
    if (radius <= 0.f) {
        radius = 1.0f;
    }
    _lightSchemaBuffer.edit().volume.radius = radius;
    updateLightRadius();
}

void Light::updateLightRadius() {
    // This function relies on the attenuation equation:
    // I = Li / (1 + (d + Lr)/Lr)^2
    // where I = calculated intensity, Li = light intensity, Lr = light falloff radius, d = distance from surface 
    // see: https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
    // note that falloff radius replaces surface radius in linked example
    // This equation is biased back by Lr so that all lights act as true points, regardless of surface radii

    const float MIN_CUTOFF_INTENSITY = 0.001f;
    // Get cutoff radius at minimum intensity
    float intensity = getIntensity() * std::max(std::max(getColor().x, getColor().y), getColor().z);
    float cutoffRadius = getFalloffRadius() * ((glm::sqrt(intensity / MIN_CUTOFF_INTENSITY) - 1) - 1);

    // If it is less than max radius, store it to buffer to avoid extra shading
    _lightSchemaBuffer.edit().irradiance.cutoffRadius = std::min(getMaximumRadius(), cutoffRadius);
}

#include <math.h>

void Light::setSpotAngle(float angle) {
    double dangle = angle;
    if (dangle <= 0.0) {
        dangle = 0.0;
    }
    if (dangle > glm::half_pi<double>()) {
        dangle = glm::half_pi<double>();
    }
    auto cosAngle = cos(dangle);
    _spotCos = (float)std::abs(cosAngle);

    if (isSpot()) {
        _lightSchemaBuffer.edit().volume.spotCos = _spotCos;
    }
}

void Light::setSpotExponent(float exponent) {
    if (exponent <= 0.f) {
        exponent = 0.0f;
    }
    _lightSchemaBuffer.edit().irradiance.falloffSpot = exponent;
}


void Light::setAmbientIntensity(float intensity) {
    _ambientSchemaBuffer.edit().intensity = intensity;
}

void Light::setAmbientSphere(const gpu::SphericalHarmonics& sphere) {
    _ambientSchemaBuffer.edit().ambientSphere = sphere;
}

void Light::setAmbientSpherePreset(gpu::SphericalHarmonics::Preset preset) {
    _ambientSchemaBuffer.edit().ambientSphere.assignPreset(preset);
}

void Light::setAmbientMap(const gpu::TexturePointer& ambientMap) {
    _ambientMap = ambientMap;
    if (ambientMap) {
        setAmbientMapNumMips(_ambientMap->getNumMips());
    } else {
        setAmbientMapNumMips(0);
    }
}

void Light::setAmbientMapNumMips(uint16_t numMips) {
    _ambientSchemaBuffer.edit().mapNumMips = (float)numMips;
}

