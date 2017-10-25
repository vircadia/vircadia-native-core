//
//  Created by Bradley Austin Davis on 2017/10/24
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "StylusPointer.h"

#include <array>

#include <QtCore/QThread>

#include <DependencyManager.h>
#include <pointers/PickManager.h>
#include <GLMHelpers.h>
#include <Transform.h>
#include <shared/QtHelpers.h>
#include <controllers/StandardControls.h>
#include <controllers/UserInputMapper.h>
#include <RegisteredMetaTypes.h>
#include <MessagesClient.h>
#include <EntityItemID.h>

#include "Application.h"
#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"

#include "scripting/HMDScriptingInterface.h"
#include "ui/overlays/Web3DOverlay.h"
#include "ui/overlays/Sphere3DOverlay.h"
#include "avatar/AvatarManager.h"
#include "InterfaceLogging.h"
#include "PickScriptingInterface.h"

using namespace controller;
using namespace bilateral;

static Setting::Handle<double> USE_FINGER_AS_STYLUS("preferAvatarFingerOverStylus", false);
static const float WEB_STYLUS_LENGTH = 0.2f;
static const float WEB_TOUCH_Y_OFFSET = 0.105f;  // how far forward (or back with a negative number) to slide stylus in hand
static const vec3 TIP_OFFSET{ 0.0f, WEB_STYLUS_LENGTH - WEB_TOUCH_Y_OFFSET, 0.0f };
static const float TABLET_MIN_HOVER_DISTANCE = 0.01f;
static const float TABLET_MAX_HOVER_DISTANCE = 0.1f;
static const float TABLET_MIN_TOUCH_DISTANCE = -0.05f;
static const float TABLET_MAX_TOUCH_DISTANCE = TABLET_MIN_HOVER_DISTANCE;
static const float EDGE_BORDER = 0.075f;

static const float HOVER_HYSTERESIS = 0.01f;
static const float NEAR_HYSTERESIS = 0.05f;
static const float TOUCH_HYSTERESIS = 0.002f;

// triggered when stylus presses a web overlay/entity
static const float HAPTIC_STYLUS_STRENGTH = 1.0f;
static const float HAPTIC_STYLUS_DURATION = 20.0f;
static const float POINTER_PRESS_TO_MOVE_DELAY = 0.33f;  // seconds

static const float WEB_DISPLAY_STYLUS_DISTANCE = 0.5f;
static const float TOUCH_PRESS_TO_MOVE_DEADSPOT = 0.0481f;
static const float TOUCH_PRESS_TO_MOVE_DEADSPOT_SQUARED = TOUCH_PRESS_TO_MOVE_DEADSPOT * TOUCH_PRESS_TO_MOVE_DEADSPOT;

std::array<StylusPointer*, 2> STYLUSES;

static OverlayID getHomeButtonID() {
    return DependencyManager::get<HMDScriptingInterface>()->getCurrentHomeButtonID();
}

static OverlayID getTabletScreenID() {
    return DependencyManager::get<HMDScriptingInterface>()->getCurrentTabletScreenID();
}

struct SideData {
    QString avatarJoint;
    QString cameraJoint;
    controller::StandardPoseChannel channel;
    controller::Hand hand;
    vec3 grabPointSphereOffset;

    int getJointIndex(bool finger) {
        const auto& jointName = finger ? avatarJoint : cameraJoint;
        return DependencyManager::get<AvatarManager>()->getMyAvatar()->getJointIndex(jointName);
    }
};

static const std::array<SideData, 2> SIDES{ { { "LeftHandIndex4",
                                                "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
                                                StandardPoseChannel::LEFT_HAND,
                                                Hand::LEFT,
                                                { -0.04f, 0.13f, 0.039f } },
                                              { "RightHandIndex4",
                                                "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND",
                                                StandardPoseChannel::RIGHT_HAND,
                                                Hand::RIGHT,
                                                { 0.04f, 0.13f, 0.039f } } } };

static StylusTip getFingerWorldLocation(Side side) {
    const auto& sideData = SIDES[index(side)];
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto fingerJointIndex = myAvatar->getJointIndex(sideData.avatarJoint);

    if (-1 == fingerJointIndex) {
        return StylusTip();
    }

    auto fingerPosition = myAvatar->getAbsoluteJointTranslationInObjectFrame(fingerJointIndex);
    auto fingerRotation = myAvatar->getAbsoluteJointRotationInObjectFrame(fingerJointIndex);
    auto avatarOrientation = myAvatar->getOrientation();
    auto avatarPosition = myAvatar->getPosition();
    StylusTip result;
    result.side = side;
    result.orientation = avatarOrientation * fingerRotation;
    result.position = avatarPosition + (avatarOrientation * fingerPosition);
    return result;
}

// controllerWorldLocation is where the controller would be, in-world, with an added offset
static StylusTip getControllerWorldLocation(Side side, float sensorToWorldScale) {
    static const std::array<Input, 2> INPUTS{ { UserInputMapper::makeStandardInput(SIDES[0].channel),
                                                UserInputMapper::makeStandardInput(SIDES[1].channel) } };
    const auto sideIndex = index(side);
    const auto& input = INPUTS[sideIndex];

    const auto pose = DependencyManager::get<UserInputMapper>()->getPose(input);
    const auto& valid = pose.valid;
    StylusTip result;
    if (valid) {
        result.side = side;
        const auto& sideData = SIDES[sideIndex];
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        auto controllerJointIndex = myAvatar->getJointIndex(sideData.cameraJoint);

        const auto avatarOrientation = myAvatar->getOrientation();
        const auto avatarPosition = myAvatar->getPosition();
        result.orientation = avatarOrientation * myAvatar->getAbsoluteJointRotationInObjectFrame(controllerJointIndex);
        result.position =
            avatarPosition + (avatarOrientation * myAvatar->getAbsoluteJointTranslationInObjectFrame(controllerJointIndex));
        // add to the real position so the grab-point is out in front of the hand, a bit
        result.position += result.orientation * (sideData.grabPointSphereOffset * sensorToWorldScale);
        auto worldControllerPos = avatarPosition + avatarOrientation * pose.translation;
        // compute tip velocity from hand controller motion, it is more accurate than computing it from previous positions.
        auto worldControllerLinearVel = avatarOrientation * pose.velocity;
        auto worldControllerAngularVel = avatarOrientation * pose.angularVelocity;
        result.velocity =
            worldControllerLinearVel + glm::cross(worldControllerAngularVel, result.position - worldControllerPos);
    }

    return result;
}

bool StylusPickResult::isNormalized() const {
    return valid && (normalizedPosition == glm::clamp(normalizedPosition, vec3(0), vec3(1)));
}

bool StylusPickResult::isNearNormal(float min, float max, float hystersis) const {
    return valid && (distance == glm::clamp(distance, min - hystersis, max + hystersis));
}

bool StylusPickResult::isNear2D(float border, float hystersis) const {
    return valid && position2D == glm::clamp(position2D, vec2(0) - border - hystersis, vec2(dimensions) + border + hystersis);
}

bool StylusPickResult::isNear(float min, float max, float border, float hystersis) const {
    // check to see if the projected stylusTip is within within the 2d border
    return isNearNormal(min, max, hystersis) && isNear2D(border, hystersis);
}

StylusPickResult::operator bool() const {
    return valid;
}

bool StylusPickResult::hasKeyboardFocus() const {
    if (!overlayID.isNull()) {
        return qApp->getOverlays().getKeyboardFocusOverlay() == overlayID;
    }
#if 0
    if (!entityID.isNull()) {
        return qApp->getKeyboardFocusEntity() == entityID;
    }
#endif
    return false;
}

void StylusPickResult::setKeyboardFocus() const {
    if (!overlayID.isNull()) {
        qApp->getOverlays().setKeyboardFocusOverlay(overlayID);
        qApp->setKeyboardFocusEntity(EntityItemID());
#if 0
    } else if (!entityID.isNull()) {
        qApp->getOverlays().setKeyboardFocusOverlay(OverlayID());
        qApp->setKeyboardFocusEntity(entityID);
#endif
    }
}

void StylusPickResult::sendHoverOverEvent() const {
    if (!overlayID.isNull()) {
        qApp->getOverlays().hoverOverOverlay(overlayID, PointerEvent{ PointerEvent::Move, deviceId(), position2D, position,
                                                                      normal, -normal });
    }
    // FIXME support entity
}

void StylusPickResult::sendHoverEnterEvent() const {
    if (!overlayID.isNull()) {
        qApp->getOverlays().hoverEnterOverlay(overlayID, PointerEvent{ PointerEvent::Move, deviceId(), position2D, position,
                                                                       normal, -normal });
    }
    // FIXME support entity
}

void StylusPickResult::sendTouchStartEvent() const {
    if (!overlayID.isNull()) {
        qApp->getOverlays().sendMousePressOnOverlay(overlayID, PointerEvent{ PointerEvent::Press, deviceId(), position2D, position,
                                                                         normal, -normal, PointerEvent::PrimaryButton,
                                                                         PointerEvent::PrimaryButton });
    }
    // FIXME support entity
}

void StylusPickResult::sendTouchEndEvent() const {
    if (!overlayID.isNull()) {
        qApp->getOverlays().sendMouseReleaseOnOverlay(overlayID,
                                                  PointerEvent{ PointerEvent::Release, deviceId(), position2D, position, normal,
                                                                -normal, PointerEvent::PrimaryButton });
    }
    // FIXME support entity
}

void StylusPickResult::sendTouchMoveEvent() const {
    if (!overlayID.isNull()) {
        qApp->getOverlays().sendMouseMoveOnOverlay(overlayID, PointerEvent{ PointerEvent::Move, deviceId(), position2D, position,
                                                                        normal, -normal, PointerEvent::PrimaryButton,
                                                                        PointerEvent::PrimaryButton });
    }
    // FIXME support entity
}

bool StylusPickResult::doesIntersect() const {
    return true;
}

// for example: if we want the closest result, compare based on distance
// if we want all results, combine them
// must return a new pointer
std::shared_ptr<PickResult> StylusPickResult::compareAndProcessNewResult(const std::shared_ptr<PickResult>& newRes) {
    auto newStylusResult = std::static_pointer_cast<StylusPickResult>(newRes);
    if (newStylusResult && newStylusResult->distance < distance) {
        return std::make_shared<StylusPickResult>(*newStylusResult);
    } else {
        return std::make_shared<StylusPickResult>(*this);
    }
}

// returns true if this result contains any valid results with distance < maxDistance
// can also filter out results with distance >= maxDistance
bool StylusPickResult::checkOrFilterAgainstMaxDistance(float maxDistance) {
    return distance < maxDistance;
}

uint32_t StylusPickResult::deviceId() const {
    // 0 is reserved for hardware mouse
    return index(tip.side) + 1;
}

StylusPick::StylusPick(Side side)
    : Pick(PickFilter(PickScriptingInterface::PICK_OVERLAYS()), FLT_MAX, true)
    , _side(side) {
}

StylusTip StylusPick::getMathematicalPick() const {
    StylusTip result;
    if (_useFingerInsteadOfStylus) {
        result = getFingerWorldLocation(_side);
    } else {
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        float sensorScaleFactor = myAvatar->getSensorToWorldScale();
        result = getControllerWorldLocation(_side, sensorScaleFactor);
        result.position += result.orientation * (TIP_OFFSET * sensorScaleFactor);
    }
    return result;
}

PickResultPointer StylusPick::getDefaultResult(const QVariantMap& pickVariant) const {
    return std::make_shared<StylusPickResult>();
}

PickResultPointer StylusPick::getEntityIntersection(const StylusTip& pick) {
    return PickResultPointer();
}

PickResultPointer StylusPick::getOverlayIntersection(const StylusTip& pick) {
    if (!getFilter().doesPickOverlays()) {
        return PickResultPointer();
    }

    std::vector<StylusPickResult> results;
    for (const auto& target : getIncludeItems()) {
        if (target.isNull()) {
            continue;
        }

        auto overlay = qApp->getOverlays().getOverlay(target);
        // Don't interact with non-3D or invalid overlays
        if (!overlay || !overlay->is3D()) {
            continue;
        }

        if (!overlay->getVisible() && !getFilter().doesPickInvisible()) {
            continue;
        }

        auto overlayType = overlay->getType();
        auto overlay3D = std::static_pointer_cast<Base3DOverlay>(overlay);
        const auto overlayRotation = overlay3D->getRotation();
        const auto overlayPosition = overlay3D->getPosition();

        StylusPickResult result;
        result.tip = pick;
        result.overlayID = target;
        result.normal = overlayRotation * Vectors::UNIT_Z;
        result.distance = glm::dot(pick.position - overlayPosition, result.normal);
        result.position = pick.position - (result.normal * result.distance);
        if (overlayType == Web3DOverlay::TYPE) {
            result.dimensions = vec3(std::static_pointer_cast<Web3DOverlay>(overlay3D)->getSize(), 0.01f);
        } else if (overlayType == Sphere3DOverlay::TYPE) {
            result.dimensions = std::static_pointer_cast<Sphere3DOverlay>(overlay3D)->getDimensions();
        } else {
            result.dimensions = vec3(0.01f);
        }
        auto tipRelativePosition = result.position - overlayPosition;
        auto localPos = glm::inverse(overlayRotation) * tipRelativePosition;
        auto normalizedPosition = localPos / result.dimensions;
        result.normalizedPosition = normalizedPosition + 0.5f;
        result.position2D = { result.normalizedPosition.x * result.dimensions.x,
                              (1.0f - result.normalizedPosition.y) * result.dimensions.y };
        result.valid = true;
        results.push_back(result);
    }

    StylusPickResult nearestTarget;
    for (const auto& result : results) {
        if (result && result.isNormalized() && result.distance < nearestTarget.distance) {
            nearestTarget = result;
        }
    }
    return std::make_shared<StylusPickResult>(nearestTarget);
}

PickResultPointer StylusPick::getAvatarIntersection(const StylusTip& pick) {
    return PickResultPointer();
}

PickResultPointer StylusPick::getHUDIntersection(const StylusTip& pick) {
    return PickResultPointer();
}

StylusPointer::StylusPointer(Side side)
    : Pointer(DependencyManager::get<PickManager>()->addPick(PickQuery::Stylus, std::make_shared<StylusPick>(side)),
              false,
              true)
    , _side(side)
    , _sideData(SIDES[index(side)]) {
    setIncludeItems({ { getHomeButtonID(), getTabletScreenID() } });
    STYLUSES[index(_side)] = this;
}

StylusPointer::~StylusPointer() {
    if (!_stylusOverlay.isNull()) {
        qApp->getOverlays().deleteOverlay(_stylusOverlay);
    }
    STYLUSES[index(_side)] = nullptr;
}

StylusPointer* StylusPointer::getOtherStylus() {
    return STYLUSES[((index(_side) + 1) % 2)];
}

void StylusPointer::enable() {
    Parent::enable();
    withWriteLock([&] { _renderingEnabled = true; });
}

void StylusPointer::disable() {
    Parent::disable();
    withWriteLock([&] { _renderingEnabled = false; });
}

void StylusPointer::updateStylusTarget() {
    const float minNearDistance = TABLET_MIN_TOUCH_DISTANCE * _sensorScaleFactor;
    const float maxNearDistance = WEB_DISPLAY_STYLUS_DISTANCE * _sensorScaleFactor;
    const float edgeBorder = EDGE_BORDER * _sensorScaleFactor;

    auto pickResult = DependencyManager::get<PickManager>()->getPrevPickResultTyped<StylusPickResult>(_pickUID);

    if (pickResult) {
        _state.target = *pickResult;
        float hystersis = 0.0f;
        // If we're already near the target, add hystersis to ensure we don't rapidly toggle between near and not near
        // but only for the current near target
        if (_previousState.nearTarget && pickResult->overlayID == _previousState.target.overlayID) {
            hystersis = _nearHysteresis;
        }
        _state.nearTarget = pickResult->isNear(minNearDistance, maxNearDistance, edgeBorder, hystersis);
    }

    // Not near anything, short circuit the rest
    if (!_state.nearTarget) {
        relinquishTouchFocus();
        hide();
        return;
    }

    show();

    auto minTouchDistance = TABLET_MIN_TOUCH_DISTANCE * _sensorScaleFactor;
    auto maxTouchDistance = TABLET_MAX_TOUCH_DISTANCE * _sensorScaleFactor;
    auto maxHoverDistance = TABLET_MAX_HOVER_DISTANCE * _sensorScaleFactor;

    float hystersis = 0.0f;
    if (_previousState.nearTarget && _previousState.target.overlayID == _previousState.target.overlayID) {
        hystersis = _nearHysteresis;
    }

    // If we're in hover distance (calculated as the normal distance from the XY plane of the overlay)
    if ((getOtherStylus() && getOtherStylus()->_state.touchingTarget) || !_state.target.isNearNormal(minTouchDistance, maxHoverDistance, hystersis)) {
        relinquishTouchFocus();
        return;
    }

    requestTouchFocus(_state.target);

    if (!_state.target.hasKeyboardFocus()) {
        _state.target.setKeyboardFocus();
    }

    if (hasTouchFocus(_state.target) && !_previousState.touchingTarget) {
        _state.target.sendHoverOverEvent();
    }

    hystersis = 0.0f;
    if (_previousState.touchingTarget && _previousState.target.overlayID == _state.target.overlayID) {
        hystersis = _touchHysteresis;
    }

    // If we're in touch distance
    if (_state.target.isNearNormal(minTouchDistance, maxTouchDistance, _touchHysteresis) && _state.target.isNormalized()) {
        _state.touchingTarget = true;
    }
}

void StylusPointer::update(float deltaTime) {
    // This only needs to be a read lock because update won't change any of the properties that can be modified from scripts
    withReadLock([&] {
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

        // Store and reset the state
        {
            _previousState = _state;
            _state = State();
        }

#if 0
        // Update finger as stylus setting 
        {
            useFingerInsteadOfStylus = (USE_FINGER_AS_STYLUS.get() && myAvatar->getJointIndex(sideData.avatarJoint) != -1);
        }
#endif

        // Update scale factor
        {
            _sensorScaleFactor = myAvatar->getSensorToWorldScale();
            _hoverHysteresis = HOVER_HYSTERESIS * _sensorScaleFactor;
            _nearHysteresis = NEAR_HYSTERESIS * _sensorScaleFactor;
            _touchHysteresis = TOUCH_HYSTERESIS * _sensorScaleFactor;
        }

        // Identify the current near or touching target
        updateStylusTarget();

        // If we stopped touching, or if the target overlay ID changed, send a touching exit to the previous touch target
        if (_previousState.touchingTarget &&
            (!_state.touchingTarget || _state.target.overlayID != _previousState.target.overlayID)) {
            stylusTouchingExit();
        }

        // Handle new or continuing touch
        if (_state.touchingTarget) {
            // If we were previously not touching, or we were touching a different overlay, add a touch enter
            if (!_previousState.touchingTarget || _previousState.target.overlayID != _state.target.overlayID) {
                stylusTouchingEnter();
            } else {
                _touchingEnterTimer += deltaTime;
            }

            stylusTouching();
        }
    });

    setIncludeItems({ { getHomeButtonID(), getTabletScreenID() } });
}

void StylusPointer::show() {
    if (!_stylusOverlay.isNull()) {
        return;
    }

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    // FIXME perhaps instantiate a stylus and use show / hide instead of create / destroy
    // however, the current design doesn't really allow for this because it assumes that
    // hide / show are idempotent and low cost, but constantly querying the visibility
    QVariantMap overlayProperties;
    overlayProperties["name"] = "stylus";
    overlayProperties["url"] = PathUtils::resourcesPath() + "/meshes/tablet-stylus-fat.fbx";
    overlayProperties["loadPriority"] = 10.0f;
    overlayProperties["dimensions"] = vec3toVariant(_sensorScaleFactor * vec3(0.01f, 0.01f, WEB_STYLUS_LENGTH));
    overlayProperties["solid"] = true;
    overlayProperties["visible"] = true;
    overlayProperties["ignoreRayIntersection"] = true;
    overlayProperties["drawInFront"] = false;
    overlayProperties["parentID"] = AVATAR_SELF_ID;
    overlayProperties["parentJointIndex"] = myAvatar->getJointIndex(_sideData.cameraJoint);

    static const glm::quat X_ROT_NEG_90{ 0.70710678f, -0.70710678f, 0.0f, 0.0f };
    auto modelOrientation = _state.target.tip.orientation * X_ROT_NEG_90;
    auto modelPositionOffset = modelOrientation * (vec3(0.0f, 0.0f, -WEB_STYLUS_LENGTH / 2.0f) * _sensorScaleFactor);
    overlayProperties["position"] = vec3toVariant(_state.target.tip.position + modelPositionOffset);
    overlayProperties["rotation"] = quatToVariant(modelOrientation);
    _stylusOverlay = qApp->getOverlays().addOverlay("model", overlayProperties);
}

void StylusPointer::hide() {
    if (_stylusOverlay.isNull()) {
        return;
    }

    qApp->getOverlays().deleteOverlay(_stylusOverlay);
    _stylusOverlay = OverlayID();
}
#if 0
        void pointFinger(bool value) {
            static const QString HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index";
            static const std::array<QString, 2> KEYS{ { "pointLeftIndex", "pointLeftIndex" } };
            if (fingerPointing != value) {
                QString message = QJsonDocument(QJsonObject{ { KEYS[index(side)], value } }).toJson();
                DependencyManager::get<MessagesClient>()->sendMessage(HIFI_POINT_INDEX_MESSAGE_CHANNEL, message);
                fingerPointing = value;
            }
        }
#endif
void StylusPointer::requestTouchFocus(const StylusPickResult& pickResult) {
    if (!pickResult) {
        return;
    }
    // send hover events to target if we can.
    // record the entity or overlay we are hovering over.
    if (!pickResult.overlayID.isNull() && pickResult.overlayID != _hoverOverlay &&
        getOtherStylus() && pickResult.overlayID != getOtherStylus()->_hoverOverlay) {
        _hoverOverlay = pickResult.overlayID;
        pickResult.sendHoverEnterEvent();
    }
}

bool StylusPointer::hasTouchFocus(const StylusPickResult& pickResult) {
    return (!pickResult.overlayID.isNull() && pickResult.overlayID == _hoverOverlay);
}

void StylusPointer::relinquishTouchFocus() {
    // send hover leave event.
    if (!_hoverOverlay.isNull()) {
        PointerEvent pointerEvent{ PointerEvent::Move, (uint32_t)(index(_side) + 1) };
        auto& overlays = qApp->getOverlays();
        overlays.sendMouseMoveOnOverlay(_hoverOverlay, pointerEvent);
        overlays.sendHoverOverOverlay(_hoverOverlay, pointerEvent);
        overlays.sendHoverLeaveOverlay(_hoverOverlay, pointerEvent);
        _hoverOverlay = OverlayID();
    }
};

void StylusPointer::stealTouchFocus() {
    // send hover events to target
    if (getOtherStylus() && _state.target.overlayID == getOtherStylus()->_hoverOverlay) {
        getOtherStylus()->relinquishTouchFocus();
    }
    requestTouchFocus(_state.target);
}

void StylusPointer::stylusTouchingEnter() {
    stealTouchFocus();
    _state.target.sendTouchStartEvent();
    DependencyManager::get<UserInputMapper>()->triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION,
                                                                  _sideData.hand);
    _touchingEnterTimer = 0;
    _touchingEnterPosition = _state.target.position2D;
    _deadspotExpired = false;
}

void StylusPointer::stylusTouchingExit() {
    if (!_previousState.target) {
        return;
    }

    // special case to handle home button.
    if (_previousState.target.overlayID == getHomeButtonID()) {
        DependencyManager::get<MessagesClient>()->sendLocalMessage("home", _previousState.target.overlayID.toString());
    }

    // send touch end event
    _state.target.sendTouchEndEvent();
}

void StylusPointer::stylusTouching() {
    qDebug() << "QQQ " << __FUNCTION__;
    if (_state.target.overlayID.isNull()) {
        return;
    }

    if (!_deadspotExpired) {
        _deadspotExpired =
            (_touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY) ||
            glm::distance2(_state.target.position2D, _touchingEnterPosition) > TOUCH_PRESS_TO_MOVE_DEADSPOT_SQUARED;
    }

    // Only send moves if the target moves more than the deadspot position or if we've timed out the deadspot
    if (_deadspotExpired) {
        _state.target.sendTouchMoveEvent();
    }
}
