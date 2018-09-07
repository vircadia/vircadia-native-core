//
//  Created by Sam Gondelman 7/17/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PathPointer_h
#define hifi_PathPointer_h

#include <QVariant>
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

class StartEndRenderState {
public:
    StartEndRenderState() {}
    StartEndRenderState(const OverlayID& startID, const OverlayID& endID);

    const OverlayID& getStartID() const { return _startID; }
    const OverlayID& getEndID() const { return _endID; }
    const bool& doesStartIgnoreRays() const { return _startIgnoreRays; }
    const bool& doesEndIgnoreRays() const { return _endIgnoreRays; }

    void setStartDim(const glm::vec3& startDim) { _startDim = startDim; }
    const glm::vec3& getStartDim() const { return _startDim; }

    void setEndDim(const glm::vec3& endDim) { _endDim = endDim; }
    const glm::vec3& getEndDim() const { return _endDim; }

    void setEndRot(const glm::quat& endRot) { _endRot = endRot; }
    const glm::quat& getEndRot() const { return _endRot; }

    virtual void cleanup();
    virtual void disable();
    virtual void update(const glm::vec3& origin, const glm::vec3& end, const glm::vec3& surfaceNormal, bool scaleWithAvatar, bool distanceScaleEnd, bool centerEndY,
                        bool faceAvatar, bool followNormal, float followNormalStrength, float distance, const PickResultPointer& pickResult);

    bool isEnabled() const { return _enabled; }

protected:
    OverlayID _startID;
    OverlayID _endID;
    bool _startIgnoreRays;
    bool _endIgnoreRays;

    glm::vec3 _startDim;
    glm::vec3 _endDim;
    glm::quat _endRot;

    glm::quat _avgEndRot;
    bool _avgEndRotInitialized { false };

    bool _enabled { true };
};

typedef std::unordered_map<std::string, std::shared_ptr<StartEndRenderState>> RenderStateMap;
typedef std::unordered_map<std::string, std::pair<float, std::shared_ptr<StartEndRenderState>>> DefaultRenderStateMap;

class PathPointer : public Pointer {
    using Parent = Pointer;
public:
    PathPointer(PickQuery::PickType type, const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates,
                bool hover, const PointerTriggers& triggers, bool faceAvatar, bool followNormal, float followNormalStrength, bool centerEndY, bool lockEnd,
                bool distanceScaleEnd, bool scaleWithAvatar, bool enabled);
    virtual ~PathPointer();

    void setRenderState(const std::string& state) override;
    // You cannot use editRenderState to change the type of any part of the pointer.  You can only edit the properties of the existing overlays.
    void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) override;

    void setLength(float length) override;
    void setLockEndUUID(const QUuid& objectID, bool isOverlay, const glm::mat4& offsetMat = glm::mat4()) override;

    void updateVisuals(const PickResultPointer& prevRayPickResult) override;

protected:
    RenderStateMap _renderStates;
    DefaultRenderStateMap _defaultRenderStates;
    std::string _currentRenderState { "" };
    PointerTriggers _triggers;
    float _pathLength { 0.0f };
    bool _faceAvatar;
    bool _followNormal;
    float _followNormalStrength;
    bool _centerEndY;
    bool _lockEnd;
    bool _distanceScaleEnd;
    bool _scaleWithAvatar;
    LockEndObject _lockEndObject;

    struct TriggerState {
        PickedObject triggeredObject;
        glm::vec3 intersection { NAN };
        glm::vec3 surfaceNormal { NAN };
        glm::vec2 triggerPos2D { NAN };
        quint64 triggerStartTime { 0 };
        bool deadspotExpired { true };
        bool triggering { false };
        bool wasTriggering { false };
    };

    Pointer::Buttons _previousButtons;
    std::unordered_map<std::string, TriggerState> _states;
    TriggerState _latestState;

    bool shouldHover(const PickResultPointer& pickResult) override { return _currentRenderState != ""; }
    bool shouldTrigger(const PickResultPointer& pickResult) override { return _currentRenderState != ""; }

    void updateRenderStateOverlay(const OverlayID& id, const QVariant& props);
    virtual void editRenderStatePath(const std::string& state, const QVariant& pathProps) = 0;

    PickedObject getHoveredObject(const PickResultPointer& pickResult) override;
    Pointer::Buttons getPressedButtons(const PickResultPointer& pickResult) override;

    PickResultPointer getVisualPickResult(const PickResultPointer& pickResult) override;
    virtual glm::vec3 getPickOrigin(const PickResultPointer& pickResult) const = 0;
    virtual glm::vec3 getPickEnd(const PickResultPointer& pickResult, float distance = 0.0f) const = 0;
    virtual glm::vec3 getPickedObjectNormal(const PickResultPointer& pickResult) const = 0;
    virtual IntersectionType getPickedObjectType(const PickResultPointer& pickResult) const = 0;
    virtual QUuid getPickedObjectID(const PickResultPointer& pickResult) const = 0;
    virtual void setVisualPickResultInternal(PickResultPointer pickResult, IntersectionType type, const QUuid& id,
                                             const glm::vec3& intersection, float distance, const glm::vec3& surfaceNormal) = 0;

    static glm::vec2 findPos2D(const PickedObject& pickedObject, const glm::vec3& origin);
};

#endif // hifi_PathPointer_h
