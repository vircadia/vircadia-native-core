//
//  LaserPointer.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointer_h
#define hifi_LaserPointer_h

#include <QString>
#include "glm/glm.hpp"

#include <DependencyManager.h>
#include "raypick/RayPickScriptingInterface.h"

class RayPickResult;

class RenderState {

public:
    RenderState() {}
    RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID);

    const OverlayID& getStartID() const { return _startID; }
    const OverlayID& getPathID() const { return _pathID; }
    const OverlayID& getEndID() const { return _endID; }
    const bool& doesStartIgnoreRays() const { return _startIgnoreRays; }
    const bool& doesPathIgnoreRays() const { return _pathIgnoreRays; }
    const bool& doesEndIgnoreRays() const { return _endIgnoreRays; }

    void deleteOverlays();

private:
    OverlayID _startID;
    OverlayID _pathID;
    OverlayID _endID;
    bool _startIgnoreRays;
    bool _pathIgnoreRays;
    bool _endIgnoreRays;
};


class LaserPointer {

public:

    typedef std::unordered_map<std::string, RenderState> RenderStateMap;
    typedef std::unordered_map<std::string, std::pair<float, RenderState>> DefaultRenderStateMap;

    LaserPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled);
    ~LaserPointer();

    QUuid getRayUID() { return _rayPickUID; }
    void enable();
    void disable();
    const RayPickResult getPrevRayPickResult() { return DependencyManager::get<RayPickScriptingInterface>()->getPrevRayPickResult(_rayPickUID); }

    void setRenderState(const std::string& state);
    // You cannot use editRenderState to change the overlay type of any part of the laser pointer.  You can only edit the properties of the existing overlays.
    void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps);

    void setPrecisionPicking(const bool precisionPicking) { DependencyManager::get<RayPickScriptingInterface>()->setPrecisionPicking(_rayPickUID, precisionPicking); }
    void setIgnoreEntities(const QScriptValue& ignoreEntities) { DependencyManager::get<RayPickScriptingInterface>()->setIgnoreEntities(_rayPickUID, ignoreEntities); }
    void setIncludeEntities(const QScriptValue& includeEntities) { DependencyManager::get<RayPickScriptingInterface>()->setIncludeEntities(_rayPickUID, includeEntities); }
    void setIgnoreOverlays(const QScriptValue& ignoreOverlays) { DependencyManager::get<RayPickScriptingInterface>()->setIgnoreOverlays(_rayPickUID, ignoreOverlays); }
    void setIncludeOverlays(const QScriptValue& includeOverlays) { DependencyManager::get<RayPickScriptingInterface>()->setIncludeOverlays(_rayPickUID, includeOverlays); }
    void setIgnoreAvatars(const QScriptValue& ignoreAvatars) { DependencyManager::get<RayPickScriptingInterface>()->setIgnoreAvatars(_rayPickUID, ignoreAvatars); }
    void setIncludeAvatars(const QScriptValue& includeAvatars) { DependencyManager::get<RayPickScriptingInterface>()->setIncludeAvatars(_rayPickUID, includeAvatars); }

    void setLockEndUUID(QUuid objectID, const bool isOverlay) { _objectLockEnd = std::pair<QUuid, bool>(objectID, isOverlay); }

    void update();

private:
    bool _renderingEnabled;
    std::string _currentRenderState { "" };
    RenderStateMap _renderStates;
    DefaultRenderStateMap _defaultRenderStates;
    bool _faceAvatar;
    bool _centerEndY;
    bool _lockEnd;
    std::pair<QUuid, bool> _objectLockEnd { std::pair<QUuid, bool>(QUuid(), false)};

    QUuid _rayPickUID;

    void updateRenderStateOverlay(const OverlayID& id, const QVariant& props);
    void updateRenderState(const RenderState& renderState, const IntersectionType type, const float distance, const QUuid& objectID, const bool defaultState);
    void disableRenderState(const RenderState& renderState);

};

#endif // hifi_LaserPointer_h
