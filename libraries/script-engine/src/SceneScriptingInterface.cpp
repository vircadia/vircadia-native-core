//
//  SceneScriptingInterface.cpp
//  libraries/script-engine
//
//  Created by Sam Gateau on 2/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SceneScriptingInterface.h"

#include <procedural/ProceduralSkybox.h>

float SceneScripting::Location::getLongitude() const {
    return _skyStage->getOriginLongitude();
}

float SceneScripting::Location::getLatitude() const {
    return _skyStage->getOriginLatitude();
}

float SceneScripting::Location::getAltitude() const {
    return _skyStage->getOriginSurfaceAltitude();
}

void SceneScripting::Location::setLongitude(float longitude) {
    _skyStage->setOriginLongitude(longitude);
}

void SceneScripting::Location::setLatitude(float latitude) {
    _skyStage->setOriginLatitude(latitude);
}

void SceneScripting::Location::setAltitude(float altitude) {
    _skyStage->setOriginSurfaceAltitude(altitude);
}

void SceneScripting::Time::setHour(float hour) {
    _skyStage->setDayTime(hour);
}

float SceneScripting::Time::getHour() const {
    return _skyStage->getDayTime();
}

void SceneScripting::Time::setDay(int day) {
    _skyStage->setYearTime(day);
}

int SceneScripting::Time::getDay() const {
    return _skyStage->getYearTime();
}

glm::vec3 SceneScripting::KeyLight::getColor() const {
    return _skyStage->getSunColor();
}

void SceneScripting::KeyLight::setColor(const glm::vec3& color) {
    _skyStage->setSunColor(color);
}

float SceneScripting::KeyLight::getIntensity() const {
    return _skyStage->getSunIntensity();
}

void SceneScripting::KeyLight::setIntensity(float intensity) {
    _skyStage->setSunIntensity(intensity);
}

float SceneScripting::KeyLight::getAmbientIntensity() const {
    return _skyStage->getSunAmbientIntensity();
}

void SceneScripting::KeyLight::setAmbientIntensity(float intensity) {
    _skyStage->setSunAmbientIntensity(intensity);
}

void SceneScripting::KeyLight::setAmbientSphere(const gpu::SHPointer& sphere) {
    _skyStage->setSunAmbientSphere(sphere);
}

void SceneScripting::KeyLight::setAmbientMap(const gpu::TexturePointer& map) {
    _skyStage->setSunAmbientMap(map);
}


glm::vec3 SceneScripting::KeyLight::getDirection() const {
    return _skyStage->getSunDirection();
}

void SceneScripting::KeyLight::setDirection(const glm::vec3& direction) {
    _skyStage->setSunDirection(direction);
}

void SceneScripting::Stage::setOrientation(const glm::quat& orientation) const {
    _skyStage->setOriginOrientation(orientation);
}

void SceneScripting::Stage::setLocation(float longitude, float latitude, float altitude) {
    _skyStage->setOriginLocation(longitude, latitude, altitude);
}

void SceneScripting::Stage::setSunModelEnable(bool isEnabled) {
    _skyStage->setSunModelEnable(isEnabled);
}

bool SceneScripting::Stage::isSunModelEnabled() const {
    return _skyStage->isSunModelEnabled();
}

void SceneScripting::Stage::setBackgroundMode(const QString& mode) {
    if (mode == QString("inherit")) {
        _skyStage->setBackgroundMode(model::SunSkyStage::NO_BACKGROUND);
    } else if (mode == QString("skybox")) {
        _skyStage->setBackgroundMode(model::SunSkyStage::SKY_BOX);
    }
}

QString SceneScripting::Stage::getBackgroundMode() const {
    switch (_skyStage->getBackgroundMode()) {
    case model::SunSkyStage::NO_BACKGROUND:
        return QString("inherit");
    case model::SunSkyStage::SKY_BOX:
        return QString("skybox");
    default:
        return QString("inherit");
    };
}

SceneScriptingInterface::SceneScriptingInterface() : _stage{ new SceneScripting::Stage{ _skyStage } } {
    // Let's make sure the sunSkyStage is using a proceduralSkybox
    _skyStage->setSkybox(model::SkyboxPointer(new ProceduralSkybox()));
}

void SceneScriptingInterface::setShouldRenderAvatars(bool shouldRenderAvatars) {
    if (shouldRenderAvatars != _shouldRenderAvatars) {
        _shouldRenderAvatars = shouldRenderAvatars;
        emit shouldRenderAvatarsChanged(_shouldRenderAvatars);
    }
}

void SceneScriptingInterface::setShouldRenderEntities(bool shouldRenderEntities) {
    if (shouldRenderEntities != _shouldRenderEntities) {
        _shouldRenderEntities = shouldRenderEntities;
        emit shouldRenderEntitiesChanged(_shouldRenderEntities);
    }
}

model::SunSkyStagePointer SceneScriptingInterface::getSkyStage() const {
    return _skyStage;
}
