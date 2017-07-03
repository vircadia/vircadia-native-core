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
#include <trackers/FaceTracker.h>

#include "Application.h"
#include "Menu.h"

HIFI_QML_DEF(AvatarInputs)

static AvatarInputs* INSTANCE{ nullptr };

Setting::Handle<bool> showAudioToolsSetting { QStringList { "AvatarInputs", "showAudioTools" }, false };

AvatarInputs* AvatarInputs::getInstance() {
    if (!INSTANCE) {
        AvatarInputs::registerType();
        AvatarInputs::show();
        Q_ASSERT(INSTANCE);
    }
    return INSTANCE;
}

AvatarInputs::AvatarInputs(QQuickItem* parent) :  QQuickItem(parent) {
    INSTANCE = this;
    _showAudioTools = showAudioToolsSetting.get();
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

    AI_UPDATE(cameraEnabled, !Menu::getInstance()->isOptionChecked(MenuOption::NoFaceTracking));
    AI_UPDATE(cameraMuted, Menu::getInstance()->isOptionChecked(MenuOption::MuteFaceTracking));
    AI_UPDATE(isHMD, qApp->isHMDMode());
}

void AvatarInputs::setShowAudioTools(bool showAudioTools) {
    if (_showAudioTools == showAudioTools)
        return;

    _showAudioTools = showAudioTools;
    showAudioToolsSetting.set(_showAudioTools);
    emit showAudioToolsChanged(_showAudioTools);
}

void AvatarInputs::toggleCameraMute() {
    FaceTracker* faceTracker = qApp->getSelectedFaceTracker();
    if (faceTracker) {
        faceTracker->toggleMute();
    }
}

void AvatarInputs::resetSensors() {
    qApp->resetSensors();
}
