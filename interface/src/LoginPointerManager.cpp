//
//  LoginPointerManager.cpp
//  interface/src
//
//  Created by Wayne Chen on 11/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LoginPointerManager.h"

#include <QtCore/QString>
#include <QtCore/QVariantMap>

#include <RegisteredMetaTypes.h>

#include "controllers/StateController.h"
#include "controllers/UserInputMapper.h"
#include "raypick/PointerScriptingInterface.h"
#include "raypick/PickScriptingInterface.h"
#include "scripting/ControllerScriptingInterface.h"

static const float SEARCH_SPHERE_SIZE = 0.0132f;
static const QVariantMap SEARCH_SPHERE = {{"x", SEARCH_SPHERE_SIZE},
                                            {"y", SEARCH_SPHERE_SIZE},
                                            {"z", SEARCH_SPHERE_SIZE}};

static const int DEFAULT_SEARCH_SPHERE_DISTANCE = 1000; // how far from camera to search intersection?

static const QVariantMap COLORS_GRAB_SEARCHING_HALF_SQUEEZE = {{"red", 10},
                                                                {"green", 10},
                                                                {"blue", 255}};

static const QVariantMap COLORS_GRAB_SEARCHING_FULL_SQUEEZE = {{"red", 250},
                                                                {"green", 10},
                                                                {"blue", 10}};

static const QVariantMap COLORS_GRAB_DISTANCE_HOLD = {{"red", 238},
                                                        {"green", 75},
                                                        {"blue", 214}};



LoginPointerManager::~LoginPointerManager() {
    auto pointers = DependencyManager::get<PointerManager>();
    if (pointers) {
        if (leftLoginPointerID() != PointerEvent::INVALID_POINTER_ID) {
            pointers->removePointer(leftLoginPointerID());
        }
        if (rightLoginPointerID() != PointerEvent::INVALID_POINTER_ID) {
            pointers->removePointer(rightLoginPointerID());
        }
    }
}

void LoginPointerManager::init() {
    QVariantMap fullPathRenderState {
        {"type", "line3d"},
        {"color", COLORS_GRAB_SEARCHING_FULL_SQUEEZE},
        {"visible", true},
        {"alpha", 1.0f},
        {"solid", true},
        {"glow", 1.0f},
        {"ignoreRayIntersection", true}, // always ignore this
        {"drawInFront", true}, // Even when burried inside of something, show it.
        {"drawHUDLayer", false}
    };
    QVariantMap fullEndRenderState {
        {"type", "sphere"},
        {"dimensions", SEARCH_SPHERE},
        {"solid", true},
        {"color", COLORS_GRAB_SEARCHING_FULL_SQUEEZE},
        {"alpha", 0.9f},
        {"ignoreRayIntersection", true},
        {"drawInFront", true}, // Even when burried inside of something, show it.
        {"drawHUDLayer", false},
        {"visible", true}
    };
    QVariantMap halfPathRenderState {
        {"type", "line3d"},
        {"color", COLORS_GRAB_SEARCHING_HALF_SQUEEZE},
        {"visible", true},
        {"alpha", 1.0f},
        {"solid", true},
        {"glow", 1.0f},
        {"ignoreRayIntersection", true}, // always ignore this
        {"drawInFront", true}, // Even when burried inside of something, show it.
        {"drawHUDLayer", false}
    };
    QVariantMap halfEndRenderState {
        {"type", "sphere"},
        {"dimensions", SEARCH_SPHERE},
        {"solid", true},
        {"color", COLORS_GRAB_SEARCHING_HALF_SQUEEZE},
        {"alpha", 0.9f},
        {"ignoreRayIntersection", true},
        {"drawInFront", true}, // Even when burried inside of something, show it.
        {"drawHUDLayer", false},
        {"visible", true}
    };
    QVariantMap holdPathRenderState {
        {"type", "line3d"},
        {"color", COLORS_GRAB_DISTANCE_HOLD},
        {"visible", true},
        {"alpha", 1.0f},
        {"solid", true},
        {"glow", 1.0f},
        {"ignoreRayIntersection", true}, // always ignore this
        {"drawInFront", true}, // Even when burried inside of something, show it.
        {"drawHUDLayer", false},
    };
    QVariantMap halfRenderStateIdentifier {
        {"name", "half"},
        {"path", halfPathRenderState},
        {"end", halfEndRenderState}
    };
    QVariantMap fullRenderStateIdentifier {
        {"name", "full"},
        {"path", fullPathRenderState},
        {"end", fullEndRenderState}
    };
    QVariantMap holdRenderStateIdentifier {
        {"name", "hold"},
        {"path", holdPathRenderState},
    };

    QVariantMap halfDefaultRenderStateIdentifier {
        {"name", "half"},
        {"distance", DEFAULT_SEARCH_SPHERE_DISTANCE},
        {"path", halfPathRenderState}
    };
    QVariantMap fullDefaultRenderStateIdentifier {
        {"name", "full"},
        {"distance", DEFAULT_SEARCH_SPHERE_DISTANCE},
        {"path", fullPathRenderState}
    };
    QVariantMap holdDefaultRenderStateIdentifier {
        {"name", "hold"},
        {"distance", DEFAULT_SEARCH_SPHERE_DISTANCE},
        {"path", holdPathRenderState}
    };

    _renderStates = QList<QVariant>({halfRenderStateIdentifier, fullRenderStateIdentifier, holdRenderStateIdentifier});
    _defaultRenderStates = QList<QVariant>({halfDefaultRenderStateIdentifier, fullDefaultRenderStateIdentifier, holdDefaultRenderStateIdentifier});

    auto pointers = DependencyManager::get<PointerScriptingInterface>().data();
    auto controller = DependencyManager::get<controller::ScriptingInterface>().data();

    glm::vec3 grabPointSphereOffsetLeft { 0.04, 0.13, 0.039 };  // x = upward, y = forward, z = lateral
    glm::vec3 grabPointSphereOffsetRight { -0.04, 0.13, 0.039 };  // x = upward, y = forward, z = lateral

    QList<QVariant> leftPointerTriggerProperties;
    QVariantMap ltClick1 {
        //{ "action", controller::StandardButtonChannel::LT_CLICK },
        { "action", controller->getStandard()["LTClick"] },
        { "button", "Focus" }
    };
    QVariantMap ltClick2 {
        //{ "action", controller::StandardButtonChannel::LT_CLICK },
        { "action", controller->getStandard()["LTClick"] },
        { "button", "Primary" }
    };

    leftPointerTriggerProperties.append(ltClick1);
    leftPointerTriggerProperties.append(ltClick2);
    const unsigned int leftHand = 0;
    QVariantMap leftPointerProperties {
        { "joint", "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND" },
        // { "filter", PickFilter::PICK_OVERLAYS },
        { "filter", PickScriptingInterface::PICK_OVERLAYS() },
        { "triggers", leftPointerTriggerProperties },
        { "posOffset", vec3toVariant(grabPointSphereOffsetLeft) },
        { "hover", true },
        { "scaleWithParent", true },
        { "distanceScaleEnd", true },
        { "hand", leftHand }
    };
    leftPointerProperties["renderStates"] = _renderStates;
    leftPointerProperties["defaultRenderStates"] = _defaultRenderStates;
    withWriteLock([&] { _leftLoginPointerID = pointers->createPointer(PickQuery::PickType::Ray, leftPointerProperties); });
    pointers->setRenderState(leftLoginPointerID(), "");
    pointers->enablePointer(leftLoginPointerID());
    const unsigned int rightHand = 1;
    QList<QVariant> rightPointerTriggerProperties;

    QVariantMap rtClick1 {
        //{ "action", controller::StandardButtonChannel::RT_CLICK },
        { "action", controller->getStandard()["RTClick"] },
        { "button", "Focus" }
    };
    QVariantMap rtClick2 {
        //{ "action", controller::StandardButtonChannel::RT_CLICK },
        { "action", controller->getStandard()["RTClick"] },
        { "button", "Primary" }
    };
    rightPointerTriggerProperties.append(rtClick1);
    rightPointerTriggerProperties.append(rtClick2);
    QVariantMap rightPointerProperties{
        { "joint", "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" },
        // { "filter", PickFilter::PICK_OVERLAYS },
        { "filter", PickScriptingInterface::PICK_OVERLAYS() },
        { "triggers", rightPointerTriggerProperties },
        { "posOffset", vec3toVariant(grabPointSphereOffsetRight) },
        { "hover", true },
        { "scaleWithParent", true },
        { "distanceScaleEnd", true },
        { "hand", rightHand }
    };
    rightPointerProperties["renderStates"] = _renderStates;
    rightPointerProperties["defaultRenderStates"] = _defaultRenderStates;
    withWriteLock([&] { _rightLoginPointerID = pointers->createPointer(PickQuery::PickType::Ray, rightPointerProperties); });
    pointers->setRenderState(rightLoginPointerID(), "");
    pointers->enablePointer(rightLoginPointerID());


}

void LoginPointerManager::update() {
    auto pointers = DependencyManager::get<PointerScriptingInterface>();
    if (pointers) {
        QString mode = "";
        // if (this.visible) {
            // if (this.locked) {
                // mode = "hold";
            // if (triggerClicks[this.hand]) {
        mode = "full";
            // } else if (triggerValues[this.hand] > TRIGGER_ON_VALUE || this.allwaysOn) {
                // mode = "half";
            // }
        // }
        if (leftLoginPointerID() > PointerEvent::INVALID_POINTER_ID) {
            pointers->setRenderState(leftLoginPointerID(), mode);
        }
        if (rightLoginPointerID() > PointerEvent::INVALID_POINTER_ID) {
            pointers->setRenderState(rightLoginPointerID(), mode);
        }
    }
}
