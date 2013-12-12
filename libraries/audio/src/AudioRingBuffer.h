//
//  AudioRingBuffer.h
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioRingBuffer__
#define __interface__AudioRingBuffer__

#include <stdint.h>
#include <map>

#include <glm/glm.hpp>

#include "NodeData.h"

const int SAMPLE_RATE = 24000;

const int BUFFER_LENGTH_BYTES_STEREO = 1024;
const int BUFFER_LENGTH_BYTES_PER_CHANNEL = 512;
const int BUFFER_LENGTH_SAMPLES_PER_CHANNEL = BUFFER_LENGTH_BYTES_PER_CHANNEL / sizeof(int16_t);

const short RING_BUFFER_LENGTH_FRAMES = 20;
const short RING_BUFFER_LENGTH_SAMPLES = RING_BUFFER_LENGTH_FRAMES * BUFFER_LENGTH_SAMPLES_PER_CHANNEL;

class AudioRingBuffer : public NodeData {
public:
    AudioRingBuffer(bool isStereo);
    ~AudioRingBuffer();

    void reset();

    int parseData(unsigned char* sourceBuffer, int numBytes);
    int parseAudioSamples(unsigned char* sourceBuffer, int numBytes);
    
    int16_t& operator[](const int index);
    
    void read(int16_t* destination, unsigned int numSamples);
    
    void shiftReadPosition(unsigned int numSamples);
    
    unsigned int samplesAvailable() const;
    
    bool isNotStarvedOrHasMinimumSamples(unsigned int numRequiredSamples) const;
    
    bool isStarved() const { return _isStarved; }
    void setIsStarved(bool isStarved) { _isStarved = isStarved; }
protected:
    // disallow copying of AudioRingBuffer objects
    AudioRingBuffer(const AudioRingBuffer&);
    AudioRingBuffer& operator= (const AudioRingBuffer&);
    
    int16_t* shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const;
    
    int16_t* _nextOutput;
    int16_t* _endOfLastWrite;
    int16_t* _buffer;
    bool _isStarved;
};

#endif /* defined(__interface__AudioRingBuffer__) */
