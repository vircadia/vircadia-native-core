//
//  AudioInjectorOptions.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioInjectorOptions_h
#define hifi_AudioInjectorOptions_h

#include <QtCore/QObject>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <RegisteredMetaTypes.h>

#include "AbstractAudioInterface.h"

class AudioInjectorOptions : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(float volume READ getVolume WRITE setVolume)
    Q_PROPERTY(bool loop READ getLoop WRITE setLoop)
public:
    AudioInjectorOptions(QObject* parent = 0);
    AudioInjectorOptions(const AudioInjectorOptions& other);
    void operator=(const AudioInjectorOptions& other);
    
    const glm::vec3& getPosition() const { return _position; }
    void setPosition(const glm::vec3& position) { _position = position; }
    
    float getVolume() const { return _volume; }
    void setVolume(float volume) { _volume = volume; }
    
    bool getLoop() const { return _loop; }
    void setLoop(bool loop) { _loop = loop; }

    const glm::quat& getOrientation() const { return _orientation; }
    void setOrientation(const glm::quat& orientation) { _orientation = orientation; }
    
    AbstractAudioInterface* getLoopbackAudioInterface() const { return _loopbackAudioInterface; }
    void setLoopbackAudioInterface(AbstractAudioInterface* loopbackAudioInterface)
        { _loopbackAudioInterface = loopbackAudioInterface; }
private:
    glm::vec3 _position;
    float _volume;
    bool _loop;
    glm::quat _orientation;
    AbstractAudioInterface* _loopbackAudioInterface;
};

#endif // hifi_AudioInjectorOptions_h
