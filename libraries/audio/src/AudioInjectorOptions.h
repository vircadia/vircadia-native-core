//
//  AudioInjectorOptions.h
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjectorOptions__
#define __hifi__AudioInjectorOptions__

#include <QtCore/QObject>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <RegisteredMetaTypes.h>

#include "AbstractAudioInterface.h"

class AudioInjectorOptions : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
public:
    AudioInjectorOptions(QObject* parent = 0);
    AudioInjectorOptions(const AudioInjectorOptions& other);
    
    const glm::vec3& getPosition() const { return _position; }
    void setPosition(const glm::vec3& position) { _position = position; }
    
    float getVolume() const { return _volume; }
    void setVolume(float volume) { _volume = volume; }
    
    const glm::quat& getOrientation() const { return _orientation; }
    void setOrientation(const glm::quat& orientation) { _orientation = orientation; }
    
    AbstractAudioInterface* getLoopbackAudioInterface() const { return _loopbackAudioInterface; }
    void setLoopbackAudioInterface(AbstractAudioInterface* loopbackAudioInterface)
        { _loopbackAudioInterface = loopbackAudioInterface; }
private:
    glm::vec3 _position;
    float _volume;
    glm::quat _orientation;
    AbstractAudioInterface* _loopbackAudioInterface;
};

#endif /* defined(__hifi__AudioInjectorOptions__) */
