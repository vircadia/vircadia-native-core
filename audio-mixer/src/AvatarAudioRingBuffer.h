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

typedef std::map<uint16_t, stk::TwoPole*> TwoPoleNodeMap;

class AvatarAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    AvatarAudioRingBuffer();
    ~AvatarAudioRingBuffer();
    
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    TwoPoleNodeMap& getTwoPoles() { return _twoPoles; }
    
    bool shouldLoopbackForNode() const { return _shouldLoopbackForNode; }
private:
    // disallow copying of AvatarAudioRingBuffer objects
    AvatarAudioRingBuffer(const AvatarAudioRingBuffer&);
    AvatarAudioRingBuffer& operator= (const AvatarAudioRingBuffer&);
    
    TwoPoleNodeMap _twoPoles;
    bool _shouldLoopbackForNode;
};

#endif /* defined(__hifi__AvatarAudioRingBuffer__) */
