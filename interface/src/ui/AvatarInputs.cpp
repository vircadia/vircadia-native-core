//
//  Created by Bradley Austin Davis 2015/06/19
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "AvatarInputs.h"

#include <AudioClient.h>
#include <SettingHandle.h>
#include <UsersScriptingInterface.h>

#include "Application.h"
#include "Menu.h"

static AvatarInputs* INSTANCE{ nullptr };

Setting::Handle<bool> showAudioToolsSetting { QStringList { "AvatarInputs", "showAudioTools" }, true };
Setting::Handle<bool> showBubbleToolsSetting{ QStringList { "AvatarInputs", "showBubbleTools" }, true };

AvatarInputs* AvatarInputs::getInstance() {
    if (!INSTANCE) {
        INSTANCE = new AvatarInputs();
        Q_ASSERT(INSTANCE);
    }
    return INSTANCE;
}

AvatarInputs::AvatarInputs(QObject* parent) : QObject(parent) {
    _showAudioTools = showAudioToolsSetting.get();
    _showBubbleTools = showBubbleToolsSetting.get();
    auto nodeList = DependencyManager::get<NodeList>();
    auto usersScriptingInterface = DependencyManager::get<UsersScriptingInterface>();
    connect(nodeList.data(), &NodeList::ignoreRadiusEnabledChanged, this, &AvatarInputs::ignoreRadiusEnabledChanged);
    connect(usersScriptingInterface.data(), &UsersScriptingInterface::enteredIgnoreRadius, this, &AvatarInputs::enteredIgnoreRadiusChanged);
}

#define AI_UPDATE(name, src) \
    { \
        auto val = src; \
        if (_##name != val) { \
            _##name = val; \
            emit name##Changed(); \
        } \
    }

float AvatarInputs::loudnessToAudioLevel(float loudness) {
    const float AUDIO_METER_AVERAGING = 0.5;
    const float LOG2 = log(2.0f);
    const float METER_LOUDNESS_SCALE = 2.8f / 5.0f;
    const float LOG2_LOUDNESS_FLOOR = 11.0f;
    float audioLevel = 0.0f;
    loudness += 1.0f;

    _trailingAudioLoudness = AUDIO_METER_AVERAGING * _trailingAudioLoudness + (1.0f - AUDIO_METER_AVERAGING) * loudness;

    float log2loudness = logf(_trailingAudioLoudness) / LOG2;

    if (log2loudness <= LOG2_LOUDNESS_FLOOR) {
        audioLevel = (log2loudness / LOG2_LOUDNESS_FLOOR) * METER_LOUDNESS_SCALE;
    } else {
        audioLevel = (log2loudness - (LOG2_LOUDNESS_FLOOR - 1.0f)) * METER_LOUDNESS_SCALE;
    }
    if (audioLevel > 1.0f) {
        audioLevel = 1.0;
    }
    return audioLevel;
}

void AvatarInputs::update() {
    if (!Menu::getInstance()) {
        return;
    }

    AI_UPDATE(isHMD, qApp->isHMDMode());
}

void AvatarInputs::setShowAudioTools(bool showAudioTools) {
    if (_showAudioTools == showAudioTools)
        return;

    _showAudioTools = showAudioTools;
    showAudioToolsSetting.set(_showAudioTools);
    emit showAudioToolsChanged(_showAudioTools);
}

void AvatarInputs::setShowBubbleTools(bool showBubbleTools) {
    if (_showBubbleTools == showBubbleTools)
        return;

    _showBubbleTools = showBubbleTools;
    showBubbleToolsSetting.set(_showBubbleTools);
    emit showBubbleToolsChanged(_showBubbleTools);
}

bool AvatarInputs::getIgnoreRadiusEnabled() const {
    return DependencyManager::get<NodeList>()->getIgnoreRadiusEnabled();
}

void AvatarInputs::resetSensors() {
    qApp->resetSensors();
}
