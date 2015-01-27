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
    _schemaBuffer = gpu::BufferView(new gpu::Buffer(sizeof(Schema), (const gpu::Buffer::Byte*) &schema));
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
    _transform.getMatrix(editSchema()._transform);
}

void Light::setOrientation(const glm::quat& orientation) {
    _transform.setRotation(orientation);
    _transform.getMatrix(editSchema()._transform);
}

void Light::setDirection(const Vec3& direction) {
    glm::mat3 rotation(
        Vec3(1.0f, 0.0f, 0.0f),
        Vec3(0.0f, 1.0f, 0.0f),
        -direction);

    setOrientation(glm::quat(rotation));
}

const Vec3& Light::getDirection() const {
    return Vec3(_transform.transform(Vec4(0.0f, 0.0f, 1.0f, 0.0f)));
}

void Light::setColor(const Color& color) {
    editSchema()._color = color;
}

void Light::setIntensity(float intensity) {
    editSchema()._intensity = intensity;
}

void Light::setAttenuationRadius(float radius) {
    if (radius <= 0.f) {
        radius = 1.0f;
    }
    editSchema()._attenuation = Vec4(1.0f, 2.0f/radius, 1.0f/(radius*radius), radius);
}

void Light::setSpotCone(float angle) {
    if (angle <= 0.f) {
        angle = 0.0f;
    }
    editSchema()._spot.x = cos(angle);
    editSchema()._spot.z = angle;
}

void Light::setSpotConeExponent(float exponent) {
    if (exponent <= 0.f) {
        exponent = 1.0f;
    }
    editSchema()._spot.y = exponent;
    editSchema()._spot.w = exponent;
}

