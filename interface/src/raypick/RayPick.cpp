//
//  RayPick.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPick.h"

RayPick::RayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled) :
    _filter(filter),
    _maxDistance(maxDistance),
    _enabled(enabled)
{
}
void RayPick::enable() {
    QWriteLocker lock(getLock());
    _enabled = true;
}

void RayPick::disable() {
    QWriteLocker lock(getLock());
    _enabled = false;
}

const RayPickResult& RayPick::getPrevRayPickResult() {
    QReadLocker lock(getLock());
    return _prevResult;
}

void RayPick::setIgnoreEntities(const QScriptValue& ignoreEntities) {
    QWriteLocker lock(getLock());
    _ignoreEntities = qVectorEntityItemIDFromScriptValue(ignoreEntities);
}

void RayPick::setIncludeEntities(const QScriptValue& includeEntities) {
    QWriteLocker lock(getLock());
    _includeEntities = qVectorEntityItemIDFromScriptValue(includeEntities);
}

void RayPick::setIgnoreOverlays(const QScriptValue& ignoreOverlays) {
    QWriteLocker lock(getLock());
    _ignoreOverlays = qVectorOverlayIDFromScriptValue(ignoreOverlays);
}

void RayPick::setIncludeOverlays(const QScriptValue& includeOverlays) {
    QWriteLocker lock(getLock());
    _includeOverlays = qVectorOverlayIDFromScriptValue(includeOverlays);
}

void RayPick::setIgnoreAvatars(const QScriptValue& ignoreAvatars) {
    QWriteLocker lock(getLock());
    _ignoreAvatars = qVectorEntityItemIDFromScriptValue(ignoreAvatars);
}

void RayPick::setIncludeAvatars(const QScriptValue& includeAvatars) {
    QWriteLocker lock(getLock());
    _includeAvatars = qVectorEntityItemIDFromScriptValue(includeAvatars);
}