//
//  HMDScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Thijs Wenker on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HMDScriptingInterface.h"

#include <QtScript/QScriptContext>

#include <avatar/AvatarManager.h>
#include <display-plugins/DisplayPlugin.h>
#include <display-plugins/CompositorHelper.h>
#include <OffscreenUi.h>
#include <plugins/PluginUtils.h>

#include "Application.h"

HMDScriptingInterface::HMDScriptingInterface() {
    connect(qApp, &Application::activeDisplayPluginChanged, [this]{
        emit displayModeChanged(isHMDMode());
    });
}

glm::vec3 HMDScriptingInterface::calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction) const {
    glm::vec3 result;
    qApp->getApplicationCompositor().calculateRayUICollisionPoint(position, direction, result);
    return result;
}

glm::vec2 HMDScriptingInterface::overlayFromWorldPoint(const glm::vec3& position) const {
    return qApp->getApplicationCompositor().overlayFromSphereSurface(position);
}

glm::vec3 HMDScriptingInterface::worldPointFromOverlay(const glm::vec2& overlay) const {
    return qApp->getApplicationCompositor().sphereSurfaceFromOverlay(overlay);
}

glm::vec2 HMDScriptingInterface::sphericalToOverlay(const glm::vec2 & position) const {
    return qApp->getApplicationCompositor().sphericalToOverlay(position);
}

glm::vec2 HMDScriptingInterface::overlayToSpherical(const glm::vec2 & position) const {
    return qApp->getApplicationCompositor().overlayToSpherical(position);
}

bool HMDScriptingInterface::isHMDAvailable() {
    return PluginUtils::isHMDAvailable();
}

bool HMDScriptingInterface::isHandControllerAvailable() {
    return PluginUtils::isHandControllerAvailable();
}

QScriptValue HMDScriptingInterface::getHUDLookAtPosition2D(QScriptContext* context, QScriptEngine* engine) {
    glm::vec3 hudIntersection;
    auto instance = DependencyManager::get<HMDScriptingInterface>();
    if (instance->getHUDLookAtPosition3D(hudIntersection)) {
        glm::vec2 overlayPos = qApp->getApplicationCompositor().overlayFromSphereSurface(hudIntersection);
        return qScriptValueFromValue<glm::vec2>(engine, overlayPos);
    }
    return QScriptValue::NullValue;
}

QScriptValue HMDScriptingInterface::getHUDLookAtPosition3D(QScriptContext* context, QScriptEngine* engine) {
    glm::vec3 result;
    auto instance = DependencyManager::get<HMDScriptingInterface>();
    if (instance->getHUDLookAtPosition3D(result)) {
        return qScriptValueFromValue<glm::vec3>(engine, result);
    }
    return QScriptValue::NullValue;
}

bool HMDScriptingInterface::getHUDLookAtPosition3D(glm::vec3& result) const {
    Camera* camera = qApp->getCamera();
    glm::vec3 position = camera->getPosition();
    glm::quat orientation = camera->getOrientation();

    glm::vec3 direction = orientation * glm::vec3(0.0f, 0.0f, -1.0f);

    const auto& compositor = qApp->getApplicationCompositor();

    return compositor.calculateRayUICollisionPoint(position, direction, result);
}

glm::mat4 HMDScriptingInterface::getWorldHMDMatrix() const {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    return myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
}

glm::vec3 HMDScriptingInterface::getPosition() const {
    if (qApp->getActiveDisplayPlugin()->isHmd()) {
        return extractTranslation(getWorldHMDMatrix());
    }
    return glm::vec3();
}

void HMDScriptingInterface::setPosition(const glm::vec3& position) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPosition", Qt::QueuedConnection, Q_ARG(const glm::vec3&, position));
        return;
    } else {
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        glm::mat4 hmdToSensor = myAvatar->getHMDSensorMatrix();
        glm::mat4 sensorToWorld = myAvatar->getSensorToWorldMatrix();
        glm::mat4 hmdToWorld = sensorToWorld * hmdToSensor;
        setTranslation(hmdToWorld, position);
        sensorToWorld = hmdToWorld * glm::inverse(hmdToSensor);
        myAvatar->setSensorToWorldMatrix(sensorToWorld);
    }
}

glm::quat HMDScriptingInterface::getOrientation() const {
    if (qApp->getActiveDisplayPlugin()->isHmd()) {
        return glm::normalize(glm::quat_cast(getWorldHMDMatrix()));
    }
    return glm::quat();
}

bool HMDScriptingInterface::isMounted() const{
    auto displayPlugin = qApp->getActiveDisplayPlugin();
    return (displayPlugin->isHmd() && displayPlugin->isDisplayVisible());
}

QString HMDScriptingInterface::preferredAudioInput() const {
    return qApp->getActiveDisplayPlugin()->getPreferredAudioInDevice();
}

QString HMDScriptingInterface::preferredAudioOutput() const {
    return qApp->getActiveDisplayPlugin()->getPreferredAudioOutDevice();
}

bool HMDScriptingInterface::setHandLasers(int hands, bool enabled, const glm::vec4& color, const glm::vec3& direction) const {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->executeOnUiThread([offscreenUi, enabled] {
        offscreenUi->getDesktop()->setProperty("hmdHandMouseActive", enabled);
    });
    return qApp->getActiveDisplayPlugin()->setHandLaser(hands,
        enabled ? DisplayPlugin::HandLaserMode::Overlay : DisplayPlugin::HandLaserMode::None,
        color, direction);
}

void HMDScriptingInterface::disableHandLasers(int hands) const {
    setHandLasers(hands, false, vec4(0), vec3(0));
}

bool HMDScriptingInterface::suppressKeyboard() {
    return qApp->getActiveDisplayPlugin()->suppressKeyboard();
}

void HMDScriptingInterface::unsuppressKeyboard() {
    qApp->getActiveDisplayPlugin()->unsuppressKeyboard();
}

bool HMDScriptingInterface::isKeyboardVisible() {
    return qApp->getActiveDisplayPlugin()->isKeyboardVisible();
}

void HMDScriptingInterface::centerUI() {
    QMetaObject::invokeMethod(qApp, "centerUI", Qt::QueuedConnection);
}

void HMDScriptingInterface::snapToAvatar() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    myAvatar->updateSensorToWorldMatrix();
}
