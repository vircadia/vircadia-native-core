//
//  AudioInjector.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioInjector_h
#define hifi_AudioInjector_h

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AudioInjectorOptions.h"
#include "Sound.h"

class AudioInjector : public QObject {
    Q_OBJECT
public:
    AudioInjector(QObject* parent);
    AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions);
    
    bool isFinished() const { return _isFinished; }
    int getCurrentSendPosition() const { return _currentSendPosition; }
public slots:
    void injectAudio();
    void stop() { _shouldStop = true; }
    void setOptions(AudioInjectorOptions& options);
    void setCurrentSendPosition(int currentSendPosition) { _currentSendPosition = currentSendPosition; }
signals:
    void finished();
private:
    Sound* _sound;
    AudioInjectorOptions _options;
    bool _shouldStop;
    bool _isFinished;
    int _currentSendPosition;
};

Q_DECLARE_METATYPE(AudioInjector*)

#endif // hifi_AudioInjector_h
