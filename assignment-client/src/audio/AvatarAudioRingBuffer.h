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

#ifndef __hifi__AvatarAudioRingBuffer__
#define __hifi__AvatarAudioRingBuffer__

#include <QtCore/QUuid>

#include "PositionalAudioRingBuffer.h"

class AvatarAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    AvatarAudioRingBuffer();
    
    int parseData(const QByteArray& packet);
private:
    // disallow copying of AvatarAudioRingBuffer objects
    AvatarAudioRingBuffer(const AvatarAudioRingBuffer&);
    AvatarAudioRingBuffer& operator= (const AvatarAudioRingBuffer&);
};

#endif /* defined(__hifi__AvatarAudioRingBuffer__) */
