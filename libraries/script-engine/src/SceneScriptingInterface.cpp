//
//  SceneScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Sam Gateau on 2/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SceneScriptingInterface.h"

#include <AddressManager.h>


#include <procedural/ProceduralSkybox.h>

SceneScriptingInterface::SceneScriptingInterface() {
    // Let's make sure the sunSkyStage is using a proceduralSKybox
    _skyStage->setSkybox(model::SkyboxPointer(new ProceduralSkybox()));
}

void SceneScriptingInterface::setStageOrientation(const glm::quat& orientation) {
    _skyStage->setOriginOrientation(orientation);
}
void SceneScriptingInterface::setStageLocation(float longitude, float latitude, float altitude) {
    _skyStage->setOriginLocation(longitude, latitude, altitude);
}

float SceneScriptingInterface::getStageLocationLongitude() const {
    return _skyStage->getOriginLongitude();
}
float SceneScriptingInterface::getStageLocationLatitude() const {
    return _skyStage->getOriginLatitude();
}
float SceneScriptingInterface::getStageLocationAltitude() const {
    return _skyStage->getOriginSurfaceAltitude();
}

void SceneScriptingInterface::setStageDayTime(float hour) {
    _skyStage->setDayTime(hour);
}

float SceneScriptingInterface::getStageDayTime() const {
    return _skyStage->getDayTime();
}

void SceneScriptingInterface::setStageYearTime(int day) {
    _skyStage->setYearTime(day);
}

int SceneScriptingInterface::getStageYearTime() const {
    return _skyStage->getYearTime();
}

void SceneScriptingInterface::setKeyLightColor(const glm::vec3& color) {
    _skyStage->setSunColor(color);
}

glm::vec3 SceneScriptingInterface::getKeyLightColor() const {
    return _skyStage->getSunColor();
}

void SceneScriptingInterface::setKeyLightIntensity(float intensity) {
    _skyStage->setSunIntensity(intensity);
}

float SceneScriptingInterface::getKeyLightIntensity() const {
    return _skyStage->getSunIntensity();
}

void SceneScriptingInterface::setKeyLightAmbientIntensity(float intensity) {
    _skyStage->setSunAmbientIntensity(intensity);
}

float SceneScriptingInterface::getKeyLightAmbientIntensity() const {
    return _skyStage->getSunAmbientIntensity();
}

void SceneScriptingInterface::setKeyLightDirection(const glm::vec3& direction) {
    _skyStage->setSunDirection(direction);
}

glm::vec3 SceneScriptingInterface::getKeyLightDirection() const {
    return _skyStage->getSunDirection();
}

void SceneScriptingInterface::setStageSunModelEnable(bool isEnabled) {
    _skyStage->setSunModelEnable(isEnabled);
}

bool SceneScriptingInterface::isStageSunModelEnabled() const {
    return _skyStage->isSunModelEnabled();
}

void SceneScriptingInterface::setBackgroundMode(const QString& mode) {
    if (mode == QString("inherit")) {
        _skyStage->setBackgroundMode(model::SunSkyStage::NO_BACKGROUND);
    } else if (mode == QString("atmosphere")) {
        _skyStage->setBackgroundMode(model::SunSkyStage::SKY_DOME);
    } else if (mode == QString("skybox")) {
        _skyStage->setBackgroundMode(model::SunSkyStage::SKY_BOX);
    }
}

QString SceneScriptingInterface::getBackgroundMode() const {
    switch (_skyStage->getBackgroundMode()) {
    case model::SunSkyStage::NO_BACKGROUND:
        return QString("inherit");
    case model::SunSkyStage::SKY_DOME:
        return QString("atmosphere");
    case model::SunSkyStage::SKY_BOX:
        return QString("skybox");
    default:
        return QString("inherit");
    };
}

model::SunSkyStagePointer SceneScriptingInterface::getSkyStage() const {
    return _skyStage;
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

void SceneScriptingInterface::setEngineRenderOpaque(bool renderOpaque) {
    _engineRenderOpaque = renderOpaque;
}

void SceneScriptingInterface::setEngineRenderTransparent(bool renderTransparent) {
    _engineRenderTransparent = renderTransparent;
}

void SceneScriptingInterface::setEngineCullOpaque(bool cullOpaque) {
    _engineCullOpaque = cullOpaque;
}

void SceneScriptingInterface::setEngineCullTransparent(bool cullTransparent) {
    _engineCullTransparent = cullTransparent;
}

void SceneScriptingInterface::setEngineSortOpaque(bool sortOpaque) {
    _engineSortOpaque = sortOpaque;
}

void SceneScriptingInterface::setEngineSortTransparent(bool sortTransparent) {
    _engineSortOpaque = sortTransparent;
}

void SceneScriptingInterface::clearEngineCounters() {
    _numFeedOpaqueItems = 0;
    _numDrawnOpaqueItems = 0;
    _numFeedTransparentItems = 0;
    _numDrawnTransparentItems = 0;
    _numFeedOverlay3DItems = 0;
    _numDrawnOverlay3DItems = 0;
}
