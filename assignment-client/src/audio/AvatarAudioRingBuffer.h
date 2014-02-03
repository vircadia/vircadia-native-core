//
//  AvatarAudioRingBuffer.h
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
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
