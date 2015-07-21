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

Light::Light() :
    _flags(0),
    _schemaBuffer(),
    _transform() {
    // only if created from nothing shall we create the Buffer to store the properties
    Schema schema;
    _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));
}

Light::Light(const Light& light) :
    _flags(light._flags),
    _schemaBuffer(light._schemaBuffer),
    _transform(light._transform) {
}

Light& Light::operator= (const Light& light) {
    _flags = (light._flags);
    _schemaBuffer = (light._schemaBuffer);
    _transform = (light._transform);

    return (*this);
}

Light::~Light() {
}

void Light::setPosition(const Vec3& position) {
    _transform.setTranslation(position);
    editSchema()._position = Vec4(position, 1.f);
}

void Light::setOrientation(const glm::quat& orientation) {
    setDirection(orientation * glm::vec3(0.0f, 0.0f, -1.0f));
    _transform.setRotation(orientation);
}

void Light::setDirection(const Vec3& direction) {
    editSchema()._direction = glm::normalize(direction);
}

const Vec3& Light::getDirection() const {
    return getSchema()._direction;
}

void Light::setColor(const Color& color) {
    editSchema()._color = color;
}

void Light::setIntensity(float intensity) {
    editSchema()._intensity = intensity;
}

void Light::setAmbientIntensity(float intensity) {
    editSchema()._ambientIntensity = intensity;
}

void Light::setMaximumRadius(float radius) {
    if (radius <= 0.f) {
        radius = 1.0f;
    }
    float CutOffIntensityRatio = 0.05f;
    float surfaceRadius = radius / (sqrtf(1.0f / CutOffIntensityRatio) - 1.0f);
    editSchema()._attenuation = Vec4(surfaceRadius, 1.0f/surfaceRadius, CutOffIntensityRatio, radius);
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
    auto sinAngle = sin(dangle);
    editSchema()._spot.x = (float) std::abs(cosAngle);
    editSchema()._spot.y = (float) std::abs(sinAngle);
    editSchema()._spot.z = (float) angle;
}

void Light::setSpotExponent(float exponent) {
    if (exponent <= 0.f) {
        exponent = 0.0f;
    }
    editSchema()._spot.w = exponent;
}

void Light::setShowContour(float show) {
    if (show <= 0.f) {
        show = 0.0f;
    }
    editSchema()._control.w = show;
}



