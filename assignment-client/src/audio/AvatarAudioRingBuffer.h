//
//  AvatarAudioRingBuffer.h
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarAudioRingBuffer_h
#define hifi_AvatarAudioRingBuffer_h

#include <QtCore/QUuid>

#include "PositionalAudioRingBuffer.h"

class AvatarAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    AvatarAudioRingBuffer(bool isStereo = false, bool dynamicJitterBuffer = false);
    
    int parseDataAndHandleDroppedPackets(const QByteArray& packet, int packetsSkipped);
private:
    // disallow copying of AvatarAudioRingBuffer objects
    AvatarAudioRingBuffer(const AvatarAudioRingBuffer&);
    AvatarAudioRingBuffer& operator= (const AvatarAudioRingBuffer&);
};

#endif // hifi_AvatarAudioRingBuffer_h
