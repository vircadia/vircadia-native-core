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
#include "AgentData.h"

struct Position {
    float x;
    float y;
    float z;
};

class AudioRingBuffer : public AgentData {
public:
    AudioRingBuffer(int ringSamples, int bufferSamples);
    ~AudioRingBuffer();
    AudioRingBuffer(const AudioRingBuffer &otherRingBuffer);

    int parseData(unsigned char* sourceBuffer, int numBytes);
    AudioRingBuffer* clone() const;

    int16_t* getNextOutput() const { return _nextOutput; }
    void setNextOutput(int16_t *nextOutput) { _nextOutput = nextOutput; }
    
    int16_t* getEndOfLastWrite() const { return _endOfLastWrite; }
    void setEndOfLastWrite(int16_t *endOfLastWrite) { _endOfLastWrite = endOfLastWrite; }
    
    int16_t* getBuffer() const { return _buffer; }
    
    bool isStarted() const { return _started; }
    void setStarted(bool started) { _started = started; }
    
    bool shouldBeAddedToMix() const  { return _shouldBeAddedToMix; }
    void setShouldBeAddedToMix(bool shouldBeAddedToMix) { _shouldBeAddedToMix = shouldBeAddedToMix; }
    
    const Position& getPosition() const { return _position; }
    float getAttenuationRatio() const { return _attenuationRatio; }
    float getBearing() const { return _bearing; }
    bool shouldLoopbackForAgent() const { return _shouldLoopbackForAgent; }

    short diffLastWriteNextOutput();
private:
    int _ringBufferLengthSamples;
    int _bufferLengthSamples;
    Position _position;
    float _attenuationRatio;
    float _bearing;
    int16_t* _nextOutput;
    int16_t* _endOfLastWrite;
    int16_t* _buffer;
    bool _started;
    bool _shouldBeAddedToMix;
    bool _shouldLoopbackForAgent;
};

#endif /* defined(__interface__AudioRingBuffer__) */
