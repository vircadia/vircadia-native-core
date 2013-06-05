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
#include <glm/gtx/quaternion.hpp>
#include <Stk.h>
#include <FreeVerb.h>

#include "AgentData.h"

const int STREAM_IDENTIFIER_NUM_BYTES = 8;
typedef std::map<uint16_t, stk::FreeVerb*> FreeVerbAgentMap;

const char INJECT_AUDIO_AT_POINT_COMMAND = 'P';
const char INJECT_AUDIO_AT_CUBE_COMMAND = 'C';

class AudioRingBuffer : public AgentData {
public:
    AudioRingBuffer(int ringSamples, int bufferSamples);
    ~AudioRingBuffer();

    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    float getRadius() const { return _radius; }

    int16_t* getNextOutput() const { return _nextOutput; }
    void setNextOutput(int16_t* nextOutput) { _nextOutput = nextOutput; }
    
    int16_t* getEndOfLastWrite() const { return _endOfLastWrite; }
    void setEndOfLastWrite(int16_t* endOfLastWrite) { _endOfLastWrite = endOfLastWrite; }
    
    int16_t* getBuffer() const { return _buffer; }
    
    FreeVerbAgentMap& getFreeVerbs() { return _freeVerbs; }
    
    bool isStarted() const { return _started; }
    void setStarted(bool started) { _started = started; }
    
    bool shouldBeAddedToMix() const  { return _shouldBeAddedToMix; }
    void setShouldBeAddedToMix(bool shouldBeAddedToMix) { _shouldBeAddedToMix = shouldBeAddedToMix; }
    
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    float getAttenuationRatio() const { return _attenuationRatio; }
    
    bool shouldLoopbackForAgent() const { return _shouldLoopbackForAgent; }
    const unsigned char* getStreamIdentifier() const { return _streamIdentifier; }

    short diffLastWriteNextOutput();
private:
    // disallow copying of AudioRingBuffer objects
    AudioRingBuffer(const AudioRingBuffer&);
    AudioRingBuffer& operator= (const AudioRingBuffer&);
    
    int _ringBufferLengthSamples;
    int _bufferLengthSamples;
    glm::vec3 _position;
    glm::quat _orientation;
    float _radius;
    float _attenuationRatio;
    int16_t* _nextOutput;
    int16_t* _endOfLastWrite;
    int16_t* _buffer;
    bool _started;
    bool _shouldBeAddedToMix;
    bool _shouldLoopbackForAgent;
    unsigned char _streamIdentifier[STREAM_IDENTIFIER_NUM_BYTES];
    FreeVerbAgentMap _freeVerbs;
};

#endif /* defined(__interface__AudioRingBuffer__) */
