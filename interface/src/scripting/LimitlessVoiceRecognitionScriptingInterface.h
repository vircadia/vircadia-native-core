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

#ifndef hifi_SpeechRecognitionScriptingInterface_h
#define hifi_SpeechRecognitionScriptingInterface_h

#include <AudioClient.h>
#include <QObject>
#include <QFuture>
#include "LimitlessConnection.h"

class LimitlessVoiceRecognitionScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    LimitlessVoiceRecognitionScriptingInterface();

    void update();

    QString authCode;

public slots:
    void setListeningToVoice(bool listening);
    void setAuthKey(QString key);

signals:
    void onReceivedTranscription(QString speech);
    void onFinishedSpeaking(QString speech);

private:

    bool _shouldStartListeningForVoice;
    static const float _audioLevelThreshold;
    static const int _voiceTimeoutDuration;

    QTimer _voiceTimer;
    LimitlessConnection _connection;

    void voiceTimeout();
};

#endif //hifi_SpeechRecognitionScriptingInterface_h
