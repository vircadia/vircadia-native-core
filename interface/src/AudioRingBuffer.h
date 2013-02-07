//
//  AudioRingBuffer.h
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioRingBuffer__
#define __interface__AudioRingBuffer__

#include <iostream>

class AudioRingBuffer {
    public:
        int16_t *nextOutput;
        int16_t *endOfLastWrite;
        int16_t *buffer;
        short ringBufferLengthSamples;
    
        short diffLastWriteNextOutput();
        short bufferOverlap(int16_t *pointer, short addedDistance);
    
        AudioRingBuffer(short ringBufferSamples);
        ~AudioRingBuffer();
};

#endif /* defined(__interface__AudioRingBuffer__) */
