//
//  PrivacyShield.cpp
//  interface/src/ui
//
//  Created by Wayne Chen on 2/27/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PrivacyShield.h"

#include <avatar/AvatarManager.h>
#include <EntityScriptingInterface.h>
#include <SoundCacheScriptingInterface.h>
#include <UserActivityLoggerScriptingInterface.h>
#include <UsersScriptingInterface.h>
#include <AudioInjectorManager.h>

#include "Application.h"
#include "PathUtils.h"
#include "GLMHelpers.h"

const int PRIVACY_SHIELD_VISIBLE_DURATION_MS = 3000;
const int PRIVACY_SHIELD_RAISE_ANIMATION_DURATION_MS = 750;
const int PRIVACY_SHIELD_SOUND_RATE_LIMIT_MS = 15000;
const float PRIVACY_SHIELD_HEIGHT_SCALE = 0.15f;

PrivacyShield::PrivacyShield() {
    auto usersScriptingInterface = DependencyManager::get<UsersScriptingInterface>();
    //connect(usersScriptingInterface.data(), &UsersScriptingInterface::ignoreRadiusEnabledChanged, [this](bool enabled) {
    //    onPrivacyShieldToggled(enabled);
    //});
    //connect(usersScriptingInterface.data(), &UsersScriptingInterface::enteredIgnoreRadius, this, &PrivacyShield::enteredIgnoreRadius);
}

void PrivacyShield::createPrivacyShield() {
    // Affects bubble height
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto avatarScale = myAvatar->getTargetScale();
    auto avatarSensorToWorldScale = myAvatar->getSensorToWorldScale();
    auto avatarWorldPosition = myAvatar->getWorldPosition();
    auto avatarWorldOrientation = myAvatar->getWorldOrientation();
    EntityItemProperties properties;
    properties.setName("Privacy-Shield");
    properties.setModelURL(PathUtils::resourcesUrl("assets/models/Bubble-v14.fbx").toString());
    properties.setDimensions(glm::vec3(avatarSensorToWorldScale, 0.75 * avatarSensorToWorldScale, avatarSensorToWorldScale));
    properties.setPosition(glm::vec3(avatarWorldPosition.x,
        -avatarScale * 2 + avatarWorldPosition.y + avatarScale * PRIVACY_SHIELD_HEIGHT_SCALE, avatarWorldPosition.z));
    properties.setRotation(avatarWorldOrientation * Quaternions::Y_180);
    properties.setModelScale(glm::vec3(2.0,  0.5 * (avatarScale + 1.0), 2.0));
    properties.setVisible(false);

    _localPrivacyShieldID = DependencyManager::get<EntityScriptingInterface>()->addEntityInternal(properties, entity::HostType::LOCAL);
    //_bubbleActivateSound = DependencyManager::get<SoundCache>()->getSound(PathUtils::resourcesUrl() + "assets/sounds/bubble.wav");

    //onPrivacyShieldToggled(DependencyManager::get<UsersScriptingInterface>()->getIgnoreRadiusEnabled(), true);
}

void PrivacyShield::destroyPrivacyShield() {
    DependencyManager::get<EntityScriptingInterface>()->deleteEntity(_localPrivacyShieldID);
}

void PrivacyShield::update(float deltaTime) {
    if (_updateConnected) {
        auto now = usecTimestampNow();
        auto delay = (now - _privacyShieldTimestamp);
        auto privacyShieldAlpha = 1.0 - (delay / PRIVACY_SHIELD_VISIBLE_DURATION_MS);
        if (privacyShieldAlpha > 0) {
            auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
            auto avatarScale = myAvatar->getTargetScale();
            auto avatarSensorToWorldScale = myAvatar->getSensorToWorldScale();
            auto avatarWorldPosition = myAvatar->getWorldPosition();
            auto avatarWorldOrientation = myAvatar->getWorldOrientation();
            EntityItemProperties properties;
            properties.setDimensions(glm::vec3(avatarSensorToWorldScale, 0.75 * avatarSensorToWorldScale, avatarSensorToWorldScale));
            properties.setRotation(avatarWorldOrientation * Quaternions::Y_180);
            if (delay < PRIVACY_SHIELD_RAISE_ANIMATION_DURATION_MS) {
                properties.setPosition(glm::vec3(avatarWorldPosition.x, 
                    (-((PRIVACY_SHIELD_RAISE_ANIMATION_DURATION_MS - delay) / PRIVACY_SHIELD_RAISE_ANIMATION_DURATION_MS)) * avatarScale *  2.0 +
                    avatarWorldPosition.y + avatarScale * PRIVACY_SHIELD_HEIGHT_SCALE, avatarWorldPosition.z));
                properties.setModelScale(glm::vec3(2.0,
                    ((1 - ((PRIVACY_SHIELD_RAISE_ANIMATION_DURATION_MS - delay) / PRIVACY_SHIELD_RAISE_ANIMATION_DURATION_MS)) *
                    (0.5 * (avatarScale + 1.0))), 2.0));
            } else {
                properties.setPosition(glm::vec3(avatarWorldPosition.x, avatarWorldPosition.y + avatarScale * PRIVACY_SHIELD_HEIGHT_SCALE, avatarWorldPosition.z));
                properties.setModelScale(glm::vec3(2.0,  0.5 * (avatarScale + 1.0), 2.0));
            }
            DependencyManager::get<EntityScriptingInterface>()->editEntity(_localPrivacyShieldID, properties);
        }
        else {
            hidePrivacyShield();
            if (_updateConnected) {
                _updateConnected = false;
            }
        }
    }
}

void PrivacyShield::enteredIgnoreRadius() {
    showPrivacyShield();
    DependencyManager::get<UserActivityLoggerScriptingInterface>()->privacyShieldActivated();
}

void PrivacyShield::onPrivacyShieldToggled(bool enabled, bool doNotLog) {
    if (!doNotLog) {
        DependencyManager::get<UserActivityLoggerScriptingInterface>()->privacyShieldToggled(enabled);
    }
    if (enabled) {
        showPrivacyShield();
    } else {
        hidePrivacyShield();
        if (_updateConnected) {
            _updateConnected = false;
        }
    }
}

void PrivacyShield::showPrivacyShield() {
    auto now = usecTimestampNow();
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto avatarScale = myAvatar->getTargetScale();
    auto avatarSensorToWorldScale = myAvatar->getSensorToWorldScale();
    auto avatarWorldPosition = myAvatar->getWorldPosition();
    auto avatarWorldOrientation = myAvatar->getWorldOrientation();
    if (now - _lastPrivacyShieldSoundTimestamp >= PRIVACY_SHIELD_SOUND_RATE_LIMIT_MS) {
        AudioInjectorOptions options;
        options.position = avatarWorldPosition;
        options.localOnly = true;
        options.volume = 0.2f;
        AudioInjector::playSoundAndDelete(_bubbleActivateSound, options);
        _lastPrivacyShieldSoundTimestamp = now;
    }
    hidePrivacyShield();
    if (_updateConnected) {
        _updateConnected = false;
    }

    EntityItemProperties properties;
    properties.setDimensions(glm::vec3(avatarSensorToWorldScale, 0.75 * avatarSensorToWorldScale, avatarSensorToWorldScale));
    properties.setPosition(glm::vec3(avatarWorldPosition.x,
        -avatarScale * 2 + avatarWorldPosition.y + avatarScale * PRIVACY_SHIELD_HEIGHT_SCALE, avatarWorldPosition.z));
    properties.setModelScale(glm::vec3(2.0,  0.5 * (avatarScale + 1.0), 2.0));
    properties.setVisible(true);

    DependencyManager::get<EntityScriptingInterface>()->editEntity(_localPrivacyShieldID, properties);

    _privacyShieldTimestamp = now;
    _updateConnected = true;
}

void PrivacyShield::hidePrivacyShield() {
    EntityTreePointer entityTree = qApp->getEntities()->getTree();
    EntityItemPointer privacyShieldEntity = entityTree->findEntityByEntityItemID(EntityItemID(_localPrivacyShieldID));
    if (privacyShieldEntity) {
        privacyShieldEntity->setVisible(false);
    }
}
