//
//  AbstractAudioInterface.h
//  libraries/audio/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AbstractAudioInterface_h
#define hifi_AbstractAudioInterface_h

#include <QtCore/QObject>
#include <QtMultimedia/qaudiooutput.h>

#include "AudioInjectorOptions.h"

class AudioInjector;
class AudioInjectorLocalBuffer;

class AbstractAudioInterface : public QObject {
    Q_OBJECT
public:
    AbstractAudioInterface(QObject* parent = 0) : QObject(parent) {};
    
public slots:
    virtual bool outputLocalInjector(bool isStereo, AudioInjector* injector) = 0;
    
    virtual void enableAudioSourceInject(bool enable) = 0;
    virtual void selectAudioSourcePinkNoise() = 0;
    virtual void selectAudioSourceSine440() = 0;

    virtual void setIsStereoInput(bool stereo) = 0;
};

Q_DECLARE_METATYPE(AbstractAudioInterface*)

#endif // hifi_AbstractAudioInterface_h
