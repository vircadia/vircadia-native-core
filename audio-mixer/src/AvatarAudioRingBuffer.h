//
//  AvatarAudioRingBuffer.h
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarAudioRingBuffer__
#define __hifi__AvatarAudioRingBuffer__

#include "PositionalAudioRingBuffer.h"

class AvatarAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    AvatarAudioRingBuffer();
    
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    bool shouldLoopbackForAgent() const { return _shouldLoopbackForAgent; }
private:
    // disallow copying of AvatarAudioRingBuffer objects
    AvatarAudioRingBuffer(const AvatarAudioRingBuffer&);
    AvatarAudioRingBuffer& operator= (const AvatarAudioRingBuffer&);
    
    bool _shouldLoopbackForAgent;
};

#endif /* defined(__hifi__AvatarAudioRingBuffer__) */
