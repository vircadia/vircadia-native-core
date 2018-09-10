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

    // update animation debug alpha values
    QStringList newAnimAlphaValues;
    qint64 now = usecTimestampNow();
    for (auto& iter : debugAlphaMap) {
        QString key = iter.first;
        float alpha = std::get<0>(iter.second);

        auto prevIter = _prevDebugAlphaMap.find(key);
        if (prevIter != _prevDebugAlphaMap.end()) {
            float prevAlpha = std::get<0>(iter.second);
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
            const float LIT_TIME = 2.0f;
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
        const float LIT_TIME = 2.0f;
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


