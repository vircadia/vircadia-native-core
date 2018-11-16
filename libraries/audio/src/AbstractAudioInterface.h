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

#include <udt/PacketHeaders.h>

#include "AudioInjectorOptions.h"
#include "AudioInjector.h"
#include "AudioSolo.h"

class AudioInjector;
class AudioInjectorLocalBuffer;
class Transform;

class AbstractAudioInterface : public QObject {
    Q_OBJECT
public:
    AbstractAudioInterface(QObject* parent = 0) : QObject(parent) {};

    static void emitAudioPacket(const void* audioData, size_t bytes, quint16& sequenceNumber, bool isStereo,
                                const Transform& transform, glm::vec3 avatarBoundingBoxCorner, glm::vec3 avatarBoundingBoxScale,
                                PacketType packetType, QString codecName = QString(""));

    // threadsafe
    // moves injector->getLocalBuffer() to another thread (so removes its parent)
    // take care to delete it when ~AudioInjector, as parenting Qt semantics will not work
    virtual bool outputLocalInjector(const AudioInjectorPointer& injector) = 0;

    virtual AudioSolo& getAudioSolo() = 0;

public slots:
    virtual bool shouldLoopbackInjectors() { return false; }

    virtual bool setIsStereoInput(bool stereo) = 0;

    virtual bool isStereoInput() = 0;

signals:
    void isStereoInputChanged(bool isStereo);
};

Q_DECLARE_METATYPE(AbstractAudioInterface*)

#endif // hifi_AbstractAudioInterface_h
