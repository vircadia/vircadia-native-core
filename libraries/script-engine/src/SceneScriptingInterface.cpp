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

#include <AddressManager.h>

#include "SceneScriptingInterface.h"

 Q_INVOKABLE void setOriginGeoLocation(float longitude, float latitude, float altitude);
void SceneScriptingInterface::setOriginLocation(float longitude, float latitude, float altitude) {
    _skyStage->setOriginLocation(longitude, latitude, altitude);
}
void SceneScriptingInterface::setSunColor(const glm::vec3& color) {
    _skyStage->setSunColor(color);
}
void SceneScriptingInterface::setSunIntensity(float intensity) {
    _skyStage->setSunIntensity(intensity);
}

void SceneScriptingInterface::setDayTime(float hour) {
    _skyStage->setDayTime(hour);
}
void SceneScriptingInterface::setYearTime(int day) {
    _skyStage->setYearTime(day);
}

model::SunSkyStagePointer SceneScriptingInterface::getSkyStage() const {
    return _skyStage;
}