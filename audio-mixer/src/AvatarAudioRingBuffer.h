//
//  AvatarAudioRingBuffer.h
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarAudioRingBuffer__
#define __hifi__AvatarAudioRingBuffer__

#include <Stk.h>
#include <TwoPole.h>

#include "PositionalAudioRingBuffer.h"

typedef std::map<uint16_t, stk::TwoPole*> TwoPoleAgentMap;

class AvatarAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    AvatarAudioRingBuffer();
    ~AvatarAudioRingBuffer();
    
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    TwoPoleAgentMap& getTwoPoles() { return _twoPoles; }
    
    bool shouldLoopbackForAgent() const { return _shouldLoopbackForAgent; }
private:
    // disallow copying of AvatarAudioRingBuffer objects
    AvatarAudioRingBuffer(const AvatarAudioRingBuffer&);
    AvatarAudioRingBuffer& operator= (const AvatarAudioRingBuffer&);
    
    TwoPoleAgentMap _twoPoles;
    bool _shouldLoopbackForAgent;
};

#endif /* defined(__hifi__AvatarAudioRingBuffer__) */
