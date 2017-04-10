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

using Lock = std::unique_lock<std::mutex>;

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
    Lock lock(_mapMutex);
    _rays.push_back(Ray(start, end, color));
}

void DebugDraw::addMarker(const QString& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color) {
    Lock lock(_mapMutex);
    _markers[key] = MarkerInfo(rotation, position, color);
}

void DebugDraw::removeMarker(const QString& key) {
    Lock lock(_mapMutex);
    _markers.erase(key);
}

void DebugDraw::addMyAvatarMarker(const QString& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color) {
    Lock lock(_mapMutex);
    _myAvatarMarkers[key] = MarkerInfo(rotation, position, color);
}

void DebugDraw::removeMyAvatarMarker(const QString& key) {
    Lock lock(_mapMutex);
    _myAvatarMarkers.erase(key);
}

//
// accessors used by renderer
//

DebugDraw::MarkerMap DebugDraw::getMarkerMap() const {
    Lock lock(_mapMutex);
    return _markers;
}

DebugDraw::MarkerMap DebugDraw::getMyAvatarMarkerMap() const {
    Lock lock(_mapMutex);
    return _myAvatarMarkers;
}

DebugDraw::Rays DebugDraw::getRays() const {
    Lock lock(_mapMutex);
    return _rays;
}

void DebugDraw::clearRays() {
    Lock lock(_mapMutex);
    _rays.clear();
}
