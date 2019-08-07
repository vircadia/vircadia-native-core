//
//  Created by Bradley Austin Davis on 2017/10/24
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_StylusPointer_h
#define hifi_StylusPointer_h

#include <Pointer.h>
#include <shared/Bilateral.h>
#include <RegisteredMetaTypes.h>

#include "StylusPick.h"

class StylusPointer : public Pointer {
    using Parent = Pointer;
    using Ptr = std::shared_ptr<StylusPointer>;

public:
    StylusPointer(const QVariant& props, const QUuid& stylus, bool hover, bool enabled,
                  const glm::vec3& modelPositionOffset, const glm::quat& modelRotationOffset, const glm::vec3& modelDimensions);
    ~StylusPointer();

    PickQuery::PickType getType() const override;

    void updateVisuals(const PickResultPointer& pickResult) override;

    // Styluses have three render states:
    // default: "events on" -> render and hover/trigger
    // "events off" -> render, don't hover/trigger
    // "disabled" -> don't render, don't hover/trigger
    void setRenderState(const std::string& state) override;
    void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) override {}

    QVariantMap toVariantMap() const override;

    static QUuid buildStylus(const QVariantMap& properties);

protected:
    PickedObject getHoveredObject(const PickResultPointer& pickResult) override;
    Buttons getPressedButtons(const PickResultPointer& pickResult) override;
    bool shouldHover(const PickResultPointer& pickResult) override;
    bool shouldTrigger(const PickResultPointer& pickResult) override;
    virtual PickResultPointer getPickResultCopy(const PickResultPointer& pickResult) const override;

    PointerEvent buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult, const std::string& button = "", bool hover = true) override;

private:
    void show(const StylusTip& tip);
    void hide();

    struct TriggerState {
        PickedObject triggeredObject;
        glm::vec3 intersection { NAN };
        glm::vec2 triggerPos2D { NAN };
        glm::vec3 surfaceNormal { NAN };
        quint64 triggerStartTime { 0 };
        bool deadspotExpired { true };
        bool triggering { false };
        bool wasTriggering { false };

        bool hovering { false };
    };

    TriggerState _state;

    enum RenderState {
        EVENTS_ON = 0,
        EVENTS_OFF,
        DISABLED
    };

    RenderState _renderState { EVENTS_ON };

    QUuid _stylus;

    static bool isWithinBounds(float distance, float min, float max, float hysteresis);
    static glm::vec3 findIntersection(const PickedObject& pickedObject, const glm::vec3& origin, const glm::vec3& direction);
    static glm::vec2 findPos2D(const PickedObject& pickedObject, const glm::vec3& origin);

    bool _showing { true };

    glm::vec3 _modelPositionOffset;
    glm::vec3 _modelDimensions;
    glm::quat _modelRotationOffset;

};

#endif // hifi_StylusPointer_h




