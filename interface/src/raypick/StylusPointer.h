//
//  Created by Bradley Austin Davis on 2017/10/24
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_StylusPointer_h
#define hifi_StylusPointer_h

#include <QString>
#include <glm/glm.hpp>

#include <pointers/Pointer.h>
#include <pointers/Pick.h>
#include <shared/Bilateral.h>
#include <RegisteredMetaTypes.h>
#include <pointers/Pick.h>

#include "ui/overlays/Overlay.h"


class StylusPick : public Pick<StylusTip> {
    using Side = bilateral::Side;
public:
    StylusPick(Side side);

    StylusTip getMathematicalPick() const override;
    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override;
    PickResultPointer getEntityIntersection(const StylusTip& pick) override;
    PickResultPointer getOverlayIntersection(const StylusTip& pick) override;
    PickResultPointer getAvatarIntersection(const StylusTip& pick) override;
    PickResultPointer getHUDIntersection(const StylusTip& pick) override;

private:
    const Side _side;
    const bool _useFingerInsteadOfStylus{ false };
};

struct SideData;

struct StylusPickResult : public PickResult {
    using Side = bilateral::Side;
    // FIXME make into a single ID
    OverlayID overlayID;
    // FIXME restore entity functionality
#if 0
    EntityItemID entityID;
#endif
    StylusTip tip;
    float distance{ FLT_MAX };
    vec3 position;
    vec2 position2D;
    vec3 normal;
    vec3 normalizedPosition;
    vec3 dimensions;
    bool valid{ false };

    virtual bool doesIntersect() const override;
    virtual std::shared_ptr<PickResult> compareAndProcessNewResult(const std::shared_ptr<PickResult>& newRes) override;
    virtual bool checkOrFilterAgainstMaxDistance(float maxDistance) override;

    bool isNormalized() const;
    bool isNearNormal(float min, float max, float hystersis) const;
    bool isNear2D(float border, float hystersis) const;
    bool isNear(float min, float max, float border, float hystersis) const;
    operator bool() const;
    bool hasKeyboardFocus() const;
    void setKeyboardFocus() const;
    void sendHoverOverEvent() const;
    void sendHoverEnterEvent() const;
    void sendTouchStartEvent() const;
    void sendTouchEndEvent() const;
    void sendTouchMoveEvent() const;

private:
    uint32_t deviceId() const;
};


class StylusPointer : public Pointer {
    using Parent = Pointer;
    using Side = bilateral::Side;
    using Ptr = std::shared_ptr<StylusPointer>;

public:
    StylusPointer(Side side);
    ~StylusPointer();

    void enable() override;
    void disable() override;
    void update(unsigned int pointerID, float deltaTime) override;

private:

    virtual void setRenderState(const std::string& state) override {}
    virtual void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) override {}
    virtual PickedObject getHoveredObject(const PickResultPointer& pickResult) override { return PickedObject();  }
    virtual Buttons getPressedButtons() override { return {}; }
    bool shouldHover() override { return true; }
    bool shouldTrigger() override { return true; }
    virtual PointerEvent buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult) const override { return PointerEvent();  }


    StylusPointer* getOtherStylus();

    void updateStylusTarget();
    void requestTouchFocus(const StylusPickResult& pickResult);
    bool hasTouchFocus(const StylusPickResult& pickResult);
    void relinquishTouchFocus();
    void stealTouchFocus();
    void stylusTouchingEnter();
    void stylusTouchingExit();
    void stylusTouching();
    void show();
    void hide();

    struct State {
        StylusPickResult target;
        bool nearTarget{ false };
        bool touchingTarget{ false };
    };

    State _state;
    State _previousState;

    float _nearHysteresis{ 0.0f };
    float _touchHysteresis{ 0.0f };
    float _hoverHysteresis{ 0.0f };

    float _sensorScaleFactor{ 1.0f };
    float _touchingEnterTimer{ 0 };
    vec2 _touchingEnterPosition;
    bool _deadspotExpired{ false };

    bool _renderingEnabled;
    OverlayID _stylusOverlay;
    OverlayID _hoverOverlay;
    const Side _side;
    const SideData& _sideData;
};

#endif // hifi_StylusPointer_h




