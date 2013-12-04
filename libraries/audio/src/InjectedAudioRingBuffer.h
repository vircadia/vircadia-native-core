//
//  InjectedAudioRingBuffer.h
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__InjectedAudioRingBuffer__
#define __hifi__InjectedAudioRingBuffer__

#include <QtCore/QUuid>

#include "PositionalAudioRingBuffer.h"

class InjectedAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    InjectedAudioRingBuffer(const QUuid& streamIdentifier = QUuid());
    
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
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

#endif /* defined(__hifi__InjectedAudioRingBuffer__) */
