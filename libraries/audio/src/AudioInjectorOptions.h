//
//  AudioInjectorOptions.h
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjectorOptions__
#define __hifi__AudioInjectorOptions__

#include <QtCore/QObject>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AbstractAudioInterface.h"

class AudioInjectorOptions : public QObject {
    Q_OBJECT
public:
    AudioInjectorOptions(QObject* parent = 0);
    AudioInjectorOptions(const AudioInjectorOptions& other);
    
    glm::vec3 position;
    float volume;
    glm::quat orientation;
    bool shouldLoopback;
    AbstractAudioInterface* loopbackAudioInterface;
};

#endif /* defined(__hifi__AudioInjectorOptions__) */
