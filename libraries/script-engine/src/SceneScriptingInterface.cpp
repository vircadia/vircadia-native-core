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

void SceneScripting::Stage::setHazeMode(const QString& mode) {
    if (mode == QString("haze off")) {
        _skyStage->setHazeMode(model::SunSkyStage::HAZE_OFF);
    } else if (mode == QString("haze on")) {
        _skyStage->setHazeMode(model::SunSkyStage::HAZE_ON);
    }
}

QString SceneScripting::Stage::getHazeMode() const {
    switch (_skyStage->getHazeMode()) {
    case model::SunSkyStage::HAZE_OFF:
        return QString("haze off");
    case model::SunSkyStage::HAZE_ON:
        return QString("haze on");
    default:
        return QString("inherit");
    };
}

void SceneScripting::Stage::setHazeRange(const float hazeRange) {
     _skyStage->setHazeRange(hazeRange);
}
float SceneScripting::Stage::getHazeRange() const {
    return _skyStage->getHazeRange();
}
void SceneScripting::Stage::setHazeColor(const xColor hazeColor) {
    _skyStage->setHazeColor(hazeColor);
}
xColor SceneScripting::Stage::getHazeColor() const {
    return _skyStage->getHazeColor();
}
void SceneScripting::Stage::setHazeGlareColor(const xColor hazeGlareColor) {
    _skyStage->setHazeGlareColor(hazeGlareColor);
}
xColor SceneScripting::Stage::getHazeGlareColor() const {
    return _skyStage->getHazeGlareColor();
}
void SceneScripting::Stage::setHazeEnableGlare(const bool hazeEnableGlare) {
    _skyStage->setHazeEnableGlare(hazeEnableGlare);
}
bool SceneScripting::Stage::getHazeEnableGlare() const {
    return _skyStage->getHazeEnableGlare();
}
void SceneScripting::Stage::setHazeGlareAngle(const float hazeGlareAngle) {
    _skyStage->setHazeGlareAngle(hazeGlareAngle);
}
float SceneScripting::Stage::getHazeGlareAngle() const {
    return _skyStage->getHazeGlareAngle();
}

void SceneScripting::Stage::setHazeAltitudeEffect(const bool hazeAltitudeEffect) {
    _skyStage->setHazeAltitudeEffect(hazeAltitudeEffect);
}
bool SceneScripting::Stage::getHazeAltitudeEffect() const {
    return _skyStage->getHazeAltitudeEffect();
}
void SceneScripting::Stage::setHazeCeiling(const float hazeCeiling) {
    _skyStage->setHazeCeiling(hazeCeiling);
}
float SceneScripting::Stage::getHazeCeiling() const {
    return _skyStage->getHazeCeiling();
}
void SceneScripting::Stage::setHazeBaseRef(const float hazeBaseRef) {
    _skyStage->setHazeBaseRef(hazeBaseRef);
}
float SceneScripting::Stage::getHazeBaseRef() const {
    return _skyStage->getHazeBaseRef();
}

void SceneScripting::Stage::setHazeBackgroundBlend(const float hazeBackgroundBlend) {
    _skyStage->setHazeBackgroundBlend(hazeBackgroundBlend);
}
float SceneScripting::Stage::getHazeBackgroundBlend() const {
    return _skyStage->getHazeBackgroundBlend();
}

void SceneScripting::Stage::setHazeAttenuateKeyLight(const bool hazeAttenuateKeyLight) {
    _skyStage->setHazeAttenuateKeyLight(hazeAttenuateKeyLight);
}
bool SceneScripting::Stage::getHazeAttenuateKeyLight() const {
    return _skyStage->getHazeAttenuateKeyLight();
}
void SceneScripting::Stage::setHazeKeyLightRange(const float hazeKeyLightRange) {
    _skyStage->setHazeKeyLightRange(hazeKeyLightRange);
}
float SceneScripting::Stage::getHazeKeyLightRange() const {
    return _skyStage->getHazeKeyLightRange();
}
void SceneScripting::Stage::setHazeKeyLightAltitude(const float hazeKeyLightAltitude) {
    _skyStage->setHazeKeyLightAltitude(hazeKeyLightAltitude);
}
float SceneScripting::Stage::getHazeKeyLightAltitude() const {
    return _skyStage->getHazeKeyLightAltitude();
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
