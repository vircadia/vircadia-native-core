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
#include <QtCore/QSharedPointer>
#include <QtCore/QThread>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AudioInjectorLocalBuffer.h"
#include "AudioInjectorOptions.h"
#include "Sound.h"

class AbstractAudioInterface;

// In order to make scripting cleaner for the AudioInjector, the script now holds on to the AudioInjector object
// until it dies. 

class AudioInjector : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(AudioInjectorOptions options WRITE setOptions READ getOptions)
public:
    AudioInjector(QObject* parent);
    AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions);
    AudioInjector(const QByteArray& audioData, const AudioInjectorOptions& injectorOptions);
    
    bool isFinished() const { return _isFinished; }
    
    int getCurrentSendPosition() const { return _currentSendPosition; }
    
    AudioInjectorLocalBuffer* getLocalBuffer() const { return _localBuffer; }
    bool isLocalOnly() const { return _options.localOnly; }
    
    void setLocalAudioInterface(AbstractAudioInterface* localAudioInterface) { _localAudioInterface = localAudioInterface; }

    static AudioInjector* playSound(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface);
    static AudioInjector* playSound(const QString& soundUrl, const float volume, const float stretchFactor, const glm::vec3 position);

public slots:
    void injectAudio();
    void restart();
    
    void stop();
    void triggerDeleteAfterFinish() { _shouldDeleteAfterFinish = true; }
    void stopAndDeleteLater();
    
    const AudioInjectorOptions& getOptions() const { return _options; }
    void setOptions(const AudioInjectorOptions& options) { _options = options; }
    
    void setCurrentSendPosition(int currentSendPosition) { _currentSendPosition = currentSendPosition; }
    float getLoudness() const { return _loudness; }
    bool isPlaying() const { return _isPlaying; }
    void restartPortionAfterFinished();
    
signals:
    void finished();

private:
    void injectToMixer();
    void injectLocally();
    
    void setIsFinished(bool isFinished);
    
    QByteArray _audioData;
    AudioInjectorOptions _options;
    bool _shouldStop = false;
    float _loudness = 0.0f;
    bool _isPlaying = false;
    bool _isStarted = false;
    bool _isFinished = false;
    bool _shouldDeleteAfterFinish = false;
    int _currentSendPosition = 0;
    AbstractAudioInterface* _localAudioInterface = NULL;
    AudioInjectorLocalBuffer* _localBuffer = NULL;
};


#endif // hifi_AudioInjector_h
