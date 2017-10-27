//
//  SpeechRecognitionScriptingInterface.h
//  interface/src/scripting
//
//  Created by Trevor Berninger on 3/20/17.
//  Copyright 2017 Limitless ltd.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtConcurrent/QtConcurrentRun>

#include <ThreadHelpers.h>
#include <src/InterfaceLogging.h>
#include <src/ui/AvatarInputs.h>

#include "LimitlessVoiceRecognitionScriptingInterface.h"

const float LimitlessVoiceRecognitionScriptingInterface::_audioLevelThreshold = 0.33f;
const int LimitlessVoiceRecognitionScriptingInterface::_voiceTimeoutDuration = 2000;

LimitlessVoiceRecognitionScriptingInterface::LimitlessVoiceRecognitionScriptingInterface() :
        _shouldStartListeningForVoice(false)
{
    _voiceTimer.setSingleShot(true);
    connect(&_voiceTimer, &QTimer::timeout, this, &LimitlessVoiceRecognitionScriptingInterface::voiceTimeout);
    connect(&_connection, &LimitlessConnection::onReceivedTranscription, this, [this](QString transcription){emit onReceivedTranscription(transcription);});
    connect(&_connection, &LimitlessConnection::onFinishedSpeaking, this, [this](QString transcription){emit onFinishedSpeaking(transcription);});
    moveToNewNamedThread(&_connection, "Limitless Connection");
}

void LimitlessVoiceRecognitionScriptingInterface::update() {
    const float audioLevel = AvatarInputs::getInstance()->loudnessToAudioLevel(DependencyManager::get<AudioClient>()->getAudioAverageInputLoudness());

    if (_shouldStartListeningForVoice) {
        if (_connection._streamingAudioForTranscription) {
            if (audioLevel > _audioLevelThreshold) {
                if (_voiceTimer.isActive()) {
                    _voiceTimer.stop();
                }
            } else if (!_voiceTimer.isActive()){
                _voiceTimer.start(_voiceTimeoutDuration);
            }
        } else if (audioLevel > _audioLevelThreshold) {
            // to make sure invoke doesn't get called twice before the method actually gets called
            _connection._streamingAudioForTranscription = true;
            QMetaObject::invokeMethod(&_connection, "startListening", Q_ARG(QString, authCode));
        }
    }
}

void LimitlessVoiceRecognitionScriptingInterface::setListeningToVoice(bool listening) {
    _shouldStartListeningForVoice = listening;
}

void LimitlessVoiceRecognitionScriptingInterface::setAuthKey(QString key) {
    authCode = key;
}

void LimitlessVoiceRecognitionScriptingInterface::voiceTimeout() {
    if (_connection._streamingAudioForTranscription) {
        QMetaObject::invokeMethod(&_connection, "stopListening");
    }
}
