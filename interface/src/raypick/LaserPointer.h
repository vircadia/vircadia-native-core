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
#include "RayPickManager.h"

class RayPickResult;

class RenderState {

public:
    RenderState() {}
    RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID);

    const OverlayID& getStartID() { return _startID; }
    const OverlayID& getPathID() { return _pathID; }
    const OverlayID& getEndID() { return _endID; }
    const bool& doesStartIgnoreRays() { return _startIgnoreRays; }
    const bool& doesPathIgnoreRays() { return _pathIgnoreRays; }
    const bool& doesEndIgnoreRays() { return _endIgnoreRays; }

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
    LaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance,
        const QHash<QString, RenderState>& renderStates, const bool faceAvatar, const bool centerEndY, const bool enabled);
    LaserPointer(const glm::vec3& position, const glm::vec3& direction, const uint16_t filter, const float maxDistance,
        const QHash<QString, RenderState>& renderStates, const bool faceAvatar, const bool centerEndY, const bool enabled);
    ~LaserPointer();

    unsigned int getUID() { return _rayPickUID; }
    void enable();
    void disable();
    const RayPickResult getPrevRayPickResult() { return RayPickManager::getInstance().getPrevRayPickResult(_rayPickUID); }

    void setRenderState(const QString& state);
    void disableRenderState(const QString& renderState);

    void update();

private:
    bool _renderingEnabled;
    QString _currentRenderState { "" };
    QHash<QString, RenderState> _renderStates;
    bool _faceAvatar;
    bool _centerEndY;

    unsigned int _rayPickUID;
};

#endif hifi_LaserPointer_h
