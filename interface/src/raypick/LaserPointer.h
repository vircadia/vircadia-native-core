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
#include <render/Scene.h>
#include "ui/overlays/Overlay.h"

class RayPickResult;

class RenderState {

public:
    RenderState() {}
    RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID) :
        _startID(startID), _pathID(pathID), _endID(endID) {}

    void render(RenderArgs* args);

private:
    OverlayID _startID;
    OverlayID _pathID;
    OverlayID _endID;
};


class LaserPointer {

public:
    LaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance,
        const QHash<QString, RenderState>& renderStates, const bool enabled);
    ~LaserPointer();

    unsigned int getUID() { return _rayPickUID; }
    void enable();
    void disable();

    void setRenderState(const QString& state) { _currentRenderState = state; }

    const RayPickResult& getPrevRayPickResult();

    void render(RenderArgs* args);
    const render::ShapeKey getShapeKey() { return render::ShapeKey::Builder::ownPipeline(); }

private:
    bool _renderingEnabled;
    QString _currentRenderState { "" };
    QHash<QString, RenderState> _renderStates;
    unsigned int _rayPickUID;
};

#endif hifi_LaserPointer_h