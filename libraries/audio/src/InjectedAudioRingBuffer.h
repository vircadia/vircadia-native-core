//
//  InjectedAudioRingBuffer.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InjectedAudioRingBuffer_h
#define hifi_InjectedAudioRingBuffer_h

#include <QtCore/QUuid>

#include "PositionalAudioRingBuffer.h"

class InjectedAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    InjectedAudioRingBuffer(const QUuid& streamIdentifier = QUuid(), bool dynamicJitterBuffer = false);
    
    int parseData(const QByteArray& packet, int packetsSkipped = 0);
    
    const QUuid& getStreamIdentifier() const { return _streamIdentifier; }
    float getRadius() const { return _radius; }
    float getAttenuationRatio() const { return _attenuationRatio; }
private:
    // disallow copying of InjectedAudioRingBuffer objects
    InjectedAudioRingBuffer(const InjectedAudioRingBuffer&);
    InjectedAudioRingBuffer& operator= (const InjectedAudioRingBuffer&);
    
    QUuid _streamIdentifier;
    float _radius;
    float _attenuationRatio;
};

#endif // hifi_InjectedAudioRingBuffer_h
