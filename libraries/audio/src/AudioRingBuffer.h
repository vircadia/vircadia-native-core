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

const float SAMPLE_RATE = 22050.0;

const int BUFFER_LENGTH_BYTES_STEREO = 1024;
const int BUFFER_LENGTH_BYTES_PER_CHANNEL = 512;
const int BUFFER_LENGTH_SAMPLES_PER_CHANNEL = BUFFER_LENGTH_BYTES_PER_CHANNEL / sizeof(int16_t);

const short RING_BUFFER_LENGTH_FRAMES = 20;
const short RING_BUFFER_LENGTH_SAMPLES = RING_BUFFER_LENGTH_FRAMES * BUFFER_LENGTH_SAMPLES_PER_CHANNEL;

class AudioRingBuffer : public NodeData {
public:

    static int const DEFAULT_LISTEN_LIST_SIZE = 100;
    enum {
        NORMAL,
        OMNI_DIRECTIONAL_POINT,
        SELECTED_SOURCES
    };

    AudioRingBuffer(bool isStereo);
    ~AudioRingBuffer();

    void reset();

    int parseData(unsigned char* sourceBuffer, int numBytes);
    int parseAudioSamples(unsigned char* sourceBuffer, int numBytes);

    int16_t* getNextOutput() const { return _nextOutput; }
    void setNextOutput(int16_t* nextOutput) { _nextOutput = nextOutput; }
    
    int16_t* getEndOfLastWrite() const { return _endOfLastWrite; }
    void setEndOfLastWrite(int16_t* endOfLastWrite) { _endOfLastWrite = endOfLastWrite; }
    
    int16_t* getBuffer() const { return _buffer; }
    
    bool isStarted() const { return _isStarted; }
    void setStarted(bool isStarted) { _isStarted = isStarted; }
    
    int diffLastWriteNextOutput() const;
    
    bool isStereo() const { return _isStereo; }
    
protected:
    // disallow copying of AudioRingBuffer objects
    AudioRingBuffer(const AudioRingBuffer&);
    AudioRingBuffer& operator= (const AudioRingBuffer&);
    
    int16_t* _nextOutput;
    int16_t* _endOfLastWrite;
    int16_t* _buffer;
    bool _isStarted;
    bool _isStereo;
};

#endif /* defined(__interface__AudioRingBuffer__) */
