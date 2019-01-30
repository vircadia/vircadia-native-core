//
//  LoginStateManager.cpp
//  interface/src
//
//  Created by Wayne Chen on 11/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LoginStateManager.h"

#include <QtCore/QString>
#include <QtCore/QVariantMap>

#include <plugins/PluginUtils.h>
#include <RegisteredMetaTypes.h>

#include "controllers/StateController.h"
#include "controllers/UserInputMapper.h"
#include "raypick/PointerScriptingInterface.h"
#include "raypick/RayPickScriptingInterface.h"
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



void LoginStateManager::tearDown() {
    auto pointers = DependencyManager::get<PointerManager>().data();
    if (pointers) {
        if (_leftLoginPointerID > PointerEvent::INVALID_POINTER_ID) {
            pointers->removePointer(_leftLoginPointerID);
            _leftLoginPointerID = PointerEvent::INVALID_POINTER_ID;
        }
        if (_rightLoginPointerID > PointerEvent::INVALID_POINTER_ID) {
            pointers->removePointer(_rightLoginPointerID);
            _rightLoginPointerID = PointerEvent::INVALID_POINTER_ID;
        }
    }
}

void LoginStateManager::setUp() {
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

    auto pointers = DependencyManager::get<PointerScriptingInterface>();
    auto controller = DependencyManager::get<controller::ScriptingInterface>();

    const glm::vec3 grabPointSphereOffsetLeft { -0.04f, 0.13f, 0.039f };  // x = upward, y = forward, z = lateral
    const glm::vec3 grabPointSphereOffsetRight { 0.04f, 0.13f, 0.039f };  // x = upward, y = forward, z = lateral
    const glm::vec3 malletOffset {glm::vec3(0.0f, 0.18f - 0.050f, 0.0f)};

    QList<QVariant> leftPointerTriggerProperties;
    QVariantMap ltClick1 {
        { "action", controller->getStandard()["LTClick"] },
        { "button", "Focus" }
    };
    QVariantMap ltClick2 {
        { "action", controller->getStandard()["LTClick"] },
        { "button", "Primary" }
    };

    leftPointerTriggerProperties = QList<QVariant>({ltClick1, ltClick2});
    const unsigned int leftHand = 0;
    QVariantMap leftPointerProperties {
        { "joint", "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND" },
        { "filter", PickScriptingInterface::PICK_OVERLAYS() },
        { "triggers", leftPointerTriggerProperties },
        { "posOffset", vec3toVariant(grabPointSphereOffsetLeft + malletOffset) },
        { "hover", true },
        { "scaleWithParent", true },
        { "distanceScaleEnd", true },
        { "hand", leftHand }
    };
    leftPointerProperties["renderStates"] = _renderStates;
    leftPointerProperties["defaultRenderStates"] = _defaultRenderStates;
    _leftLoginPointerID = pointers->createPointer(PickQuery::PickType::Ray, leftPointerProperties);
    pointers->setRenderState(_leftLoginPointerID, "");
    pointers->enablePointer(_leftLoginPointerID);
    const unsigned int rightHand = 1;
    QList<QVariant> rightPointerTriggerProperties;

    QVariantMap rtClick1 {
        { "action", controller->getStandard()["RTClick"] },
        { "button", "Focus" }
    };
    QVariantMap rtClick2 {
        { "action", controller->getStandard()["RTClick"] },
        { "button", "Primary" }
    };
    rightPointerTriggerProperties = QList<QVariant>({rtClick1, rtClick2});
    QVariantMap rightPointerProperties{
        { "joint", "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" },
        { "filter", PickScriptingInterface::PICK_OVERLAYS() },
        { "triggers", rightPointerTriggerProperties },
        { "posOffset", vec3toVariant(grabPointSphereOffsetRight + malletOffset) },
        { "hover", true },
        { "scaleWithParent", true },
        { "distanceScaleEnd", true },
        { "hand", rightHand }
    };
    rightPointerProperties["renderStates"] = _renderStates;
    rightPointerProperties["defaultRenderStates"] = _defaultRenderStates;
    _rightLoginPointerID = pointers->createPointer(PickQuery::PickType::Ray, rightPointerProperties);
    pointers->setRenderState(_rightLoginPointerID, "");
    pointers->enablePointer(_rightLoginPointerID);
}

void LoginStateManager::update(const QString dominantHand, const QUuid loginOverlayID) {
    if (!isSetUp()) {
        return;
    }
    if (_dominantHand != dominantHand) {
        _dominantHand = dominantHand;
    }
    auto pointers = DependencyManager::get<PointerScriptingInterface>();
    auto raypicks = DependencyManager::get<RayPickScriptingInterface>();
    if (pointers && raypicks) {
        const auto rightObjectID = raypicks->getPrevRayPickResult(_rightLoginPointerID)["objectID"].toUuid();
        const auto leftObjectID = raypicks->getPrevRayPickResult(_leftLoginPointerID)["objectID"].toUuid();
        const QString leftMode = (leftObjectID.isNull() || leftObjectID != loginOverlayID) ? "" : "full";
        const QString rightMode = (rightObjectID.isNull() || rightObjectID != loginOverlayID) ? "" : "full";
        pointers->setRenderState(_leftLoginPointerID, leftMode);
        pointers->setRenderState(_rightLoginPointerID, rightMode);
        if (_dominantHand == "left" && !leftObjectID.isNull()) {
            // dominant is left.
            pointers->setRenderState(_rightLoginPointerID, "");
            pointers->setRenderState(_leftLoginPointerID, leftMode);
        } else if (_dominantHand == "right" && !rightObjectID.isNull()) {
            // dominant is right.
            pointers->setRenderState(_leftLoginPointerID, "");
            pointers->setRenderState(_rightLoginPointerID, rightMode);
        }
    }
}
