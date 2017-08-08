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
#include "ui/overlays/Overlay.h"

#include <DependencyManager.h>
#include "RayPickManager.h"

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
    LaserPointer(const QVariantMap& rayProps, const QHash<QString, RenderState>& renderStates, QHash<QString, QPair<float, RenderState>>& defaultRenderStates,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled);
    ~LaserPointer();

    QUuid getRayUID() { return _rayPickUID; }
    void enable();
    void disable();
    const RayPickResult getPrevRayPickResult() { return DependencyManager::get<RayPickManager>()->getPrevRayPickResult(_rayPickUID); }

    void setRenderState(const QString& state);
    // You cannot use editRenderState to change the overlay type of any part of the laser pointer.  You can only edit the properties of the existing overlays.
    void editRenderState(const QString& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps);

    void setIgnoreEntities(const QScriptValue& ignoreEntities) { DependencyManager::get<RayPickManager>()->setIgnoreEntities(_rayPickUID, ignoreEntities); }
    void setIncludeEntities(const QScriptValue& includeEntities) { DependencyManager::get<RayPickManager>()->setIncludeEntities(_rayPickUID, includeEntities); }
    void setIgnoreOverlays(const QScriptValue& ignoreOverlays) { DependencyManager::get<RayPickManager>()->setIgnoreOverlays(_rayPickUID, ignoreOverlays); }
    void setIncludeOverlays(const QScriptValue& includeOverlays) { DependencyManager::get<RayPickManager>()->setIncludeOverlays(_rayPickUID, includeOverlays); }
    void setIgnoreAvatars(const QScriptValue& ignoreAvatars) { DependencyManager::get<RayPickManager>()->setIgnoreAvatars(_rayPickUID, ignoreAvatars); }
    void setIncludeAvatars(const QScriptValue& includeAvatars) { DependencyManager::get<RayPickManager>()->setIncludeAvatars(_rayPickUID, includeAvatars); }

    void setLockEndUUID(QUuid objectID, const bool isOverlay) { _objectLockEnd = QPair<QUuid, bool>(objectID, isOverlay); }

    void update();

private:
    bool _renderingEnabled;
    QString _currentRenderState { "" };
    QHash<QString, RenderState> _renderStates;
    QHash<QString, QPair<float, RenderState>> _defaultRenderStates;
    bool _faceAvatar;
    bool _centerEndY;
    bool _lockEnd;
    QPair<QUuid, bool> _objectLockEnd { QPair<QUuid, bool>(QUuid(), false)};

    QUuid _rayPickUID;

    void updateRenderStateOverlay(const OverlayID& id, const QVariant& props);
    void updateRenderState(const RenderState& renderState, const IntersectionType type, const float distance, const QUuid& objectID, const bool defaultState);
    void disableRenderState(const RenderState& renderState);
};

#endif // hifi_LaserPointer_h
