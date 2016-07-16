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

#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QThread>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <NLPacket.h>

#include "AudioInjectorLocalBuffer.h"
#include "AudioInjectorOptions.h"
#include "AudioHRTF.h"
#include "Sound.h"

class AbstractAudioInterface;
class AudioInjectorManager;


enum class AudioInjectorState : uint8_t {
    NotFinished = 1,
    Finished = 2,
    PendingDelete = 4,
    LocalInjectionFinished = 8,
    NetworkInjectionFinished = 16
};

AudioInjectorState operator& (AudioInjectorState lhs, AudioInjectorState rhs);
AudioInjectorState& operator|= (AudioInjectorState& lhs, AudioInjectorState rhs);

// In order to make scripting cleaner for the AudioInjector, the script now holds on to the AudioInjector object
// until it dies. 
class AudioInjector : public QObject {
    Q_OBJECT
    
public:
    static const uint8_t NotFinished = 1;
    static const uint8_t Finished = 2;
    static const uint8_t PendingDelete = 4;
    static const uint8_t LocalInjectionFinished = 8;
    static const uint8_t NetworkInjectionFinished = 16;

    AudioInjector(QObject* parent);
    AudioInjector(const Sound& sound, const AudioInjectorOptions& injectorOptions);
    AudioInjector(const QByteArray& audioData, const AudioInjectorOptions& injectorOptions);
    
    bool isFinished() const { return (stateHas(AudioInjectorState::Finished)); }
    
    int getCurrentSendOffset() const { return _currentSendOffset; }
    void setCurrentSendOffset(int currentSendOffset) { _currentSendOffset = currentSendOffset; }
    
    AudioInjectorLocalBuffer* getLocalBuffer() const { return _localBuffer; }
    AudioHRTF& getLocalHRTF() { return _localHRTF; }

    bool isLocalOnly() const { return _options.localOnly; }
    float getVolume() const { return _options.volume; }
    glm::vec3 getPosition() const { return _options.position; }
    bool isStereo() const { return _options.stereo; }
    void setLocalAudioInterface(AbstractAudioInterface* localAudioInterface) { _localAudioInterface = localAudioInterface; }

    bool stateHas(AudioInjectorState state) const ;
    static AudioInjector* playSoundAndDelete(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface);
    static AudioInjector* playSound(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface);
    static AudioInjector* playSound(SharedSoundPointer sound, const float volume, const float stretchFactor, const glm::vec3 position);

public slots:
    void restart();
    
    void stop();
    void triggerDeleteAfterFinish();
    void stopAndDeleteLater();
    
    const AudioInjectorOptions& getOptions() const { return _options; }
    void setOptions(const AudioInjectorOptions& options);
    
    float getLoudness() const { return _loudness; }
    bool isPlaying() const { return stateHas(AudioInjectorState::NotFinished); }
    void finish();
    void finishLocalInjection();
    void finishNetworkInjection();
    
signals:
    void finished();
    void restarting();
    
private:
    void setupInjection();
    int64_t injectNextFrame();
    bool injectLocally();
    
    QByteArray _audioData;
    AudioInjectorOptions _options;
    AudioInjectorState _state { AudioInjectorState::NotFinished };
    bool _hasSentFirstFrame { false };
    bool _hasSetup { false };
    bool _shouldStop { false };
    float _loudness { 0.0f };
    int _currentSendOffset { 0 };
    std::unique_ptr<NLPacket> _currentPacket { nullptr };
    AbstractAudioInterface* _localAudioInterface { nullptr };
    AudioInjectorLocalBuffer* _localBuffer { nullptr };
    
    int64_t _nextFrame { 0 };
    std::unique_ptr<QElapsedTimer> _frameTimer { nullptr };
    quint16 _outgoingSequenceNumber { 0 };
    
    // when the injector is local, we need this
    AudioHRTF _localHRTF;
    friend class AudioInjectorManager;
};

    
#endif // hifi_AudioInjector_h
