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

#include <pointers/Pointer.h>
#include <pointers/Pick.h>

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

    void deleteOverlays();

private:
    OverlayID _startID;
    OverlayID _pathID;
    OverlayID _endID;
    bool _startIgnoreRays;
    bool _pathIgnoreRays;
    bool _endIgnoreRays;

    glm::vec3 _endDim;
};

class LaserPointer : public Pointer {
    using Parent = Pointer;
public:
    typedef std::unordered_map<std::string, RenderState> RenderStateMap;
    typedef std::unordered_map<std::string, std::pair<float, RenderState>> DefaultRenderStateMap;

    LaserPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates, const PointerTriggers& triggers,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool distanceScaleEnd, const bool enabled);
    ~LaserPointer();

    void setRenderState(const std::string& state) override;
    // You cannot use editRenderState to change the overlay type of any part of the laser pointer.  You can only edit the properties of the existing overlays.
    void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) override;

    void setLength(const float length) override;
    void setLockEndUUID(QUuid objectID, const bool isOverlay) override;

    void updateVisuals(const QVariantMap& prevRayPickResult) override;

    PickedObject getHoveredObject(const QVariantMap& pickResult) override;
    Pointer::Buttons getPressedButtons() override;

    static RenderState buildRenderState(const QVariantMap& propMap);

protected:
    PointerEvent buildPointerEvent(const QUuid& uid, const QVariantMap& pickResult) const override;

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
    std::pair<QUuid, bool> _objectLockEnd { std::pair<QUuid, bool>(QUuid(), false)};

    void updateRenderStateOverlay(const OverlayID& id, const QVariant& props);
    void updateRenderState(const RenderState& renderState, const IntersectionType type, const float distance, const QUuid& objectID, const PickRay& pickRay, const bool defaultState);
    void disableRenderState(const RenderState& renderState);

    glm::vec2 projectOntoEntityXYPlane(const QUuid& entity, const glm::vec3& worldPos) const;
    glm::vec2 projectOntoOverlayXYPlane(const QUuid& overlayID, const glm::vec3& worldPos) const;
    glm::vec2 projectOntoXYPlane(const glm::vec3& worldPos, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions, const glm::vec3& registrationPoint) const;

};

#endif // hifi_LaserPointer_h
