//
//  Created by Anthony J. Thibault 2018/08/06
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimStats.h"

#include <avatar/AvatarManager.h>
#include <OffscreenUi.h>
#include "Menu.h"

HIFI_QML_DEF(AnimStats)

static AnimStats* INSTANCE{ nullptr };

AnimStats* AnimStats::getInstance() {
    Q_ASSERT(INSTANCE);
    return INSTANCE;
}

AnimStats::AnimStats(QQuickItem* parent) :  QQuickItem(parent) {
    INSTANCE = this;
}

void AnimStats::updateStats(bool force) {
    QQuickItem* parent = parentItem();
    if (!force) {
        if (!Menu::getInstance()->isOptionChecked(MenuOption::AnimStats)) {
            if (parent->isVisible()) {
                parent->setVisible(false);
            }
            return;
        } else if (!parent->isVisible()) {
            parent->setVisible(true);
        }
    }

    auto avatarManager = DependencyManager::get<AvatarManager>();
    auto myAvatar = avatarManager->getMyAvatar();
    auto debugAlphaMap = myAvatar->getSkeletonModel()->getRig().getDebugAlphaMap();

    glm::vec3 position = myAvatar->getWorldPosition();
    glm::quat rotation = myAvatar->getWorldOrientation();
    glm::vec3 velocity = myAvatar->getWorldVelocity();

    _positionText = QString("Position: (%1, %2, %3)").
        arg(QString::number(position.x, 'f', 2)).
        arg(QString::number(position.y, 'f', 2)).
        arg(QString::number(position.z, 'f', 2));
    emit positionTextChanged();

    glm::vec3 eulerRotation = safeEulerAngles(rotation);
    _rotationText = QString("Heading: %1").
        arg(QString::number(glm::degrees(eulerRotation.y), 'f', 2));
    emit rotationTextChanged();

    // transform velocity into rig coordinate frame. z forward.
    glm::vec3 localVelocity = Quaternions::Y_180 * glm::inverse(rotation) * velocity;
    _velocityText = QString("Local Vel: (%1, %2, %3)").
        arg(QString::number(localVelocity.x, 'f', 2)).
        arg(QString::number(localVelocity.y, 'f', 2)).
        arg(QString::number(localVelocity.z, 'f', 2));
    emit velocityTextChanged();

    // print if we are recentering or not.
    _recenterText = "Recenter: ";
    if (myAvatar->isFollowActive(CharacterController::FollowType::Rotation)) {
        _recenterText += "Rotation ";
    }
    if (myAvatar->isFollowActive(CharacterController::FollowType::Horizontal)) {
        _recenterText += "Horizontal ";
    }
    if (myAvatar->isFollowActive(CharacterController::FollowType::Vertical)) {
        _recenterText += "Vertical ";
    }
    emit recenterTextChanged();

    // print current standing vs sitting state.
    if (myAvatar->getIsInSittingState()) {
        _sittingText = "SittingState: Sit";
    } else {
        _sittingText = "SittingState: Stand";
    }
    emit sittingTextChanged();

    // print current walking vs leaning state.
    if (myAvatar->getIsInWalkingState()) {
        _walkingText = "WalkingState: Walk";
    } else {
        _walkingText = "WalkingState: Lean";
    }
    emit walkingTextChanged();

    // print current overrideJointText
    int overrideJointCount = myAvatar->getOverrideJointCount();
    _overrideJointText = QString("Override Joint Count: %1").arg(overrideJointCount);
    emit overrideJointTextChanged();

    // print current flowText
    bool flowActive = myAvatar->getFlowActive();
    _flowText = QString("Flow: %1").arg(flowActive ? "enabled" : "disabled");
    emit flowTextChanged();

    // print current networkGraphText
    bool networkGraphActive = myAvatar->getNetworkGraphActive();
    _networkGraphText = QString("Network Graph: %1").arg(networkGraphActive ? "enabled" : "disabled");
    emit networkGraphTextChanged();

    // update animation debug alpha values
    QStringList newAnimAlphaValues;
    qint64 now = usecTimestampNow();
    for (auto& iter : debugAlphaMap) {
        QString key = iter.first;
        float alpha = std::get<0>(iter.second);

        auto prevIter = _prevDebugAlphaMap.find(key);
        if (prevIter != _prevDebugAlphaMap.end()) {
            float prevAlpha = std::get<0>(prevIter->second);
            if (prevAlpha != alpha) {
                // change detected: reset timer
                _animAlphaValueChangedTimers[key] = now;
            }
        } else {
            // new value: start timer
            _animAlphaValueChangedTimers[key] = now;
        }

        AnimNodeType type = std::get<1>(iter.second);
        if (type == AnimNodeType::Clip) {

            // figure out the grayScale color of this line.
            const float LIT_TIME = 20.0f;
            const float FADE_OUT_TIME = 1.0f;
            float grayScale = 0.0f;
            float secondsElapsed = (float)(now - _animAlphaValueChangedTimers[key]) / (float)USECS_PER_SECOND;
            if (secondsElapsed < LIT_TIME) {
                grayScale = 1.0f;
            } else if (secondsElapsed < LIT_TIME + FADE_OUT_TIME) {
                grayScale = (FADE_OUT_TIME - (secondsElapsed - LIT_TIME)) / FADE_OUT_TIME;
            } else {
                grayScale = 0.0f;
            }

            if (grayScale > 0.0f) {
                // append grayScaleColor to start of debug string
                newAnimAlphaValues << QString::number(grayScale, 'f', 2) + "|" + key + ": " + QString::number(alpha, 'f', 3);
            }
        }
    }

    _animAlphaValues = newAnimAlphaValues;
    _prevDebugAlphaMap = debugAlphaMap;

    emit animAlphaValuesChanged();

    // update animation anim vars
    _animVarsList.clear();
    auto animVars = myAvatar->getSkeletonModel()->getRig().getAnimVars().toDebugMap();
    for (auto& iter : animVars) {
        QString key = iter.first;
        QString value = iter.second;

        auto prevIter = _prevAnimVars.find(key);
        if (prevIter != _prevAnimVars.end()) {
            QString prevValue = prevIter->second;
            if (value != prevValue) {
                // change detected: reset timer
                _animVarChangedTimers[key] = now;
            }
        } else {
            // new value: start timer
            _animVarChangedTimers[key] = now;
        }

        // figure out the grayScale color of this line.
        const float LIT_TIME = 20.0f;
        const float FADE_OUT_TIME = 0.5f;
        float grayScale = 0.0f;
        float secondsElapsed = (float)(now - _animVarChangedTimers[key]) / (float)USECS_PER_SECOND;
        if (secondsElapsed < LIT_TIME) {
            grayScale = 1.0f;
        } else if (secondsElapsed < LIT_TIME + FADE_OUT_TIME) {
            grayScale = (FADE_OUT_TIME - (secondsElapsed - LIT_TIME)) / FADE_OUT_TIME;
        } else {
            grayScale = 0.0f;
        }

        if (grayScale > 0.0f) {
            // append grayScaleColor to start of debug string
            _animVarsList << QString::number(grayScale, 'f', 2) + "|" + key + ": " + value;
        }
    }
    _prevAnimVars = animVars;
    emit animVarsChanged();

    // animation state machines
    _animStateMachines.clear();
    auto stateMachineMap = myAvatar->getSkeletonModel()->getRig().getStateMachineMap();
    for (auto& iter : stateMachineMap) {
        _animStateMachines << iter.second;
    }
    emit animStateMachinesChanged();
}


