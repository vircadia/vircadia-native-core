//
//  DebugDraw.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DebugDraw.h"
#include "SharedUtil.h"

DebugDraw& DebugDraw::getInstance() {
    static DebugDraw* instance = globalInstance<DebugDraw>("com.highfidelity.DebugDraw");
    return *instance;
}

DebugDraw::DebugDraw() {

}

DebugDraw::~DebugDraw() {

}

// world space line, drawn only once
void DebugDraw::drawRay(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color) {
    _rays.push_back(Ray(start, end, color));
}

void DebugDraw::addMarker(const std::string& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color) {
    _markers[key] = MarkerInfo(rotation, position, color);
}

void DebugDraw::removeMarker(const std::string& key) {
    _markers.erase(key);
}

void DebugDraw::addMyAvatarMarker(const std::string& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color) {
    _myAvatarMarkers[key] = MarkerInfo(rotation, position, color);
}

void DebugDraw::removeMyAvatarMarker(const std::string& key) {
    _myAvatarMarkers.erase(key);
}

