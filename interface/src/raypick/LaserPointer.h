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
#include <glm/glm.hpp>

#include "ui/overlays/Overlay.h"

#include <Pointer.h>
#include <Pick.h>

struct LockEndObject {
    QUuid id { QUuid() };
    bool isOverlay { false };
    glm::mat4 offsetMat { glm::mat4() };
};

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

    void setEndDim(const glm::vec3& endDim) { _endDim = endDim; }
    const glm::vec3& getEndDim() const { return _endDim; }

    void setLineWidth(const float& lineWidth) { _lineWidth = lineWidth; }
    const float& getLineWidth() const { return _lineWidth; }

    void deleteOverlays();

private:
    OverlayID _startID;
    OverlayID _pathID;
    OverlayID _endID;
    bool _startIgnoreRays;
    bool _pathIgnoreRays;
    bool _endIgnoreRays;

    glm::vec3 _endDim;
    float _lineWidth;
};

class LaserPointer : public Pointer {
    using Parent = Pointer;
public:
    typedef std::unordered_map<std::string, RenderState> RenderStateMap;
    typedef std::unordered_map<std::string, std::pair<float, RenderState>> DefaultRenderStateMap;

    LaserPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates, bool hover, const PointerTriggers& triggers,
        bool faceAvatar, bool centerEndY, bool lockEnd, bool distanceScaleEnd, bool scaleWithAvatar, bool enabled);
    ~LaserPointer();

    void setRenderState(const std::string& state) override;
    // You cannot use editRenderState to change the overlay type of any part of the laser pointer.  You can only edit the properties of the existing overlays.
    void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) override;

    void setLength(float length) override;
    void setLockEndUUID(const QUuid& objectID, bool isOverlay, const glm::mat4& offsetMat = glm::mat4()) override;

    void updateVisuals(const PickResultPointer& prevRayPickResult) override;

    static RenderState buildRenderState(const QVariantMap& propMap);

protected:
    PointerEvent buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult, bool hover = true) const override;

    PickedObject getHoveredObject(const PickResultPointer& pickResult) override;
    Pointer::Buttons getPressedButtons() override;

    bool shouldHover(const PickResultPointer& pickResult) override { return _currentRenderState != ""; }
    bool shouldTrigger(const PickResultPointer& pickResult) override { return _currentRenderState != ""; }

private:
    PointerTriggers _triggers;
    float _laserLength { 0.0f };
    std::string _currentRenderState { "" };
    RenderStateMap _renderStates;
    DefaultRenderStateMap _defaultRenderStates;
    bool _faceAvatar;
    bool _centerEndY;
    bool _lockEnd;
    bool _distanceScaleEnd;
    bool _scaleWithAvatar;
    LockEndObject _lockEndObject;

    void updateRenderStateOverlay(const OverlayID& id, const QVariant& props);
    void updateRenderState(const RenderState& renderState, const IntersectionType type, float distance, const QUuid& objectID, const PickRay& pickRay, bool defaultState);
    void disableRenderState(const RenderState& renderState);

};

#endif // hifi_LaserPointer_h
