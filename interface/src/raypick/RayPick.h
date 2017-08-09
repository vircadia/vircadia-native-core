//
//  RayPick.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPick_h
#define hifi_RayPick_h

#include <stdint.h>
#include "RegisteredMetaTypes.h"

#include "EntityItemID.h"
#include "ui/overlays/Overlay.h"

class RayPick {

public:
    RayPick(const uint16_t filter, const float maxDistance, const bool enabled);

    virtual const PickRay getPickRay(bool& valid) const = 0;

    void enable() { _enabled = true; }
    void disable() { _enabled = false; }

    const uint16_t& getFilter() { return _filter; }
    const float& getMaxDistance() { return _maxDistance; }
    const bool& isEnabled() { return _enabled; }
    const RayPickResult& getPrevRayPickResult() { return _prevResult; }

    void setRayPickResult(const RayPickResult& rayPickResult) { _prevResult = rayPickResult; }

    const QVector<EntityItemID>& getIgnoreEntites() { return _ignoreEntities; }
    const QVector<EntityItemID>& getIncludeEntites() { return _includeEntities; }
    const QVector<OverlayID>& getIgnoreOverlays() { return _ignoreOverlays; }
    const QVector<OverlayID>& getIncludeOverlays() { return _includeOverlays; }
    const QVector<EntityItemID>& getIgnoreAvatars() { return _ignoreAvatars; }
    const QVector<EntityItemID>& getIncludeAvatars() { return _includeAvatars; }
    void setIgnoreEntities(const QScriptValue& ignoreEntities) { _ignoreEntities = qVectorEntityItemIDFromScriptValue(ignoreEntities); }
    void setIncludeEntities(const QScriptValue& includeEntities) { _includeEntities = qVectorEntityItemIDFromScriptValue(includeEntities); }
    void setIgnoreOverlays(const QScriptValue& ignoreOverlays) { _ignoreOverlays = qVectorOverlayIDFromScriptValue(ignoreOverlays); }
    void setIncludeOverlays(const QScriptValue& includeOverlays) { _includeOverlays = qVectorOverlayIDFromScriptValue(includeOverlays); }
    void setIgnoreAvatars(const QScriptValue& ignoreAvatars) { _ignoreAvatars = qVectorEntityItemIDFromScriptValue(ignoreAvatars); }
    void setIncludeAvatars(const QScriptValue& includeAvatars) { _includeAvatars = qVectorEntityItemIDFromScriptValue(includeAvatars); }

private:
    uint16_t _filter;
    float _maxDistance;
    bool _enabled;
    RayPickResult _prevResult;

    QVector<EntityItemID> _ignoreEntities;
    QVector<EntityItemID> _includeEntities;
    QVector<OverlayID> _ignoreOverlays;
    QVector<OverlayID> _includeOverlays;
    QVector<EntityItemID> _ignoreAvatars;
    QVector<EntityItemID> _includeAvatars;
};

#endif // hifi_RayPick_h
