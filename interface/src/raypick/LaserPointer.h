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

#include "PathPointer.h"

class LaserPointer : public PathPointer {
    using Parent = PathPointer;
public:
    class RenderState : public StartEndRenderState {
    public:
        RenderState() {}
        RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID);

        const OverlayID& getPathID() const { return _pathID; }
        const bool& doesPathIgnoreRays() const { return _pathIgnoreRays; }

        void setLineWidth(const float& lineWidth) { _lineWidth = lineWidth; }
        const float& getLineWidth() const { return _lineWidth; }

        void cleanup() override;
        void disable() override;
        void update(const glm::vec3& origin, const glm::vec3& end, const glm::vec3& surfaceNormal, bool scaleWithAvatar, bool distanceScaleEnd, bool centerEndY,
                    bool faceAvatar, bool followNormal, float followNormalStrength, float distance, const PickResultPointer& pickResult) override;

    private:
        OverlayID _pathID;
        bool _pathIgnoreRays;

        float _lineWidth;
    };

    LaserPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates, bool hover, const PointerTriggers& triggers,
        bool faceAvatar, bool followNormal, float followNormalStrength, bool centerEndY, bool lockEnd, bool distanceScaleEnd, bool scaleWithAvatar, bool enabled);

    static std::shared_ptr<StartEndRenderState> buildRenderState(const QVariantMap& propMap);

protected:
    void editRenderStatePath(const std::string& state, const QVariant& pathProps) override;

    glm::vec3 getPickOrigin(const PickResultPointer& pickResult) const override;
    glm::vec3 getPickEnd(const PickResultPointer& pickResult, float distance) const override;
    glm::vec3 getPickedObjectNormal(const PickResultPointer& pickResult) const override;
    IntersectionType getPickedObjectType(const PickResultPointer& pickResult) const override;
    QUuid getPickedObjectID(const PickResultPointer& pickResult) const override;
    void setVisualPickResultInternal(PickResultPointer pickResult, IntersectionType type, const QUuid& id,
                                     const glm::vec3& intersection, float distance, const glm::vec3& surfaceNormal) override;

    PointerEvent buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult, const std::string& button = "", bool hover = true) override;

private:
    static glm::vec3 findIntersection(const PickedObject& pickedObject, const glm::vec3& origin, const glm::vec3& direction);

};

#endif // hifi_LaserPointer_h
