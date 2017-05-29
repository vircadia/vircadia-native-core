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
}

#define AI_UPDATE(name, src) \
    { \
        auto val = src; \
        if (_##name != val) { \
            _##name = val; \
            emit name##Changed(); \
        } \
    }

#define AI_UPDATE_WRITABLE(name, src) \
    { \
        auto val = src; \
        if (_##name != val) { \
            _##name = val; \
            qDebug() << "AvatarInputs" << val; \
            emit name##Changed(val); \
        } \
    }

#define AI_UPDATE_FLOAT(name, src, epsilon) \
    { \
        float val = src; \
        if (fabsf(_##name - val) >= epsilon) { \
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

    AI_UPDATE_WRITABLE(showAudioTools, Menu::getInstance()->isOptionChecked(MenuOption::AudioTools));

    auto audioIO = DependencyManager::get<AudioClient>();

    const float audioLevel = loudnessToAudioLevel(DependencyManager::get<AudioClient>()->getLastInputLoudness());

    AI_UPDATE_FLOAT(audioLevel, audioLevel, 0.01f);
    AI_UPDATE(audioClipping, ((audioIO->getTimeSinceLastClip() > 0.0f) && (audioIO->getTimeSinceLastClip() < 1.0f)));
    AI_UPDATE(audioMuted, audioIO->isMuted());

    //// Make muted icon pulsate
    //static const float PULSE_MIN = 0.4f;
    //static const float PULSE_MAX = 1.0f;
    //static const float PULSE_FREQUENCY = 1.0f; // in Hz
    //qint64 now = usecTimestampNow();
    //if (now - _iconPulseTimeReference > (qint64)USECS_PER_SECOND) {
    //    // Prevents t from getting too big, which would diminish glm::cos precision
    //    _iconPulseTimeReference = now - ((now - _iconPulseTimeReference) % USECS_PER_SECOND);
    //}
    //float t = (float)(now - _iconPulseTimeReference) / (float)USECS_PER_SECOND;
    //float pulseFactor = (glm::cos(t * PULSE_FREQUENCY * 2.0f * PI) + 1.0f) / 2.0f;
    //iconColor = PULSE_MIN + (PULSE_MAX - PULSE_MIN) * pulseFactor;
}

void AvatarInputs::setShowAudioTools(bool showAudioTools) {
    if (_showAudioTools == showAudioTools)
        return;

    Menu::getInstance()->setIsOptionChecked(MenuOption::AudioTools, showAudioTools);
    update();
}

void AvatarInputs::toggleCameraMute() {
    FaceTracker* faceTracker = qApp->getSelectedFaceTracker();
    if (faceTracker) {
        faceTracker->toggleMute();
    }
}

void AvatarInputs::toggleAudioMute() {
    DependencyManager::get<AudioClient>()->toggleMute();
}

void AvatarInputs::resetSensors() {
    qApp->resetSensors();
}
