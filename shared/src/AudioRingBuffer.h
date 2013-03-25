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
#include <stdint.h>
#include "AgentData.h"

class AudioRingBuffer : public AgentData {
    public:
        AudioRingBuffer(int ringSamples, int bufferSamples);
        ~AudioRingBuffer();
        AudioRingBuffer(const AudioRingBuffer &otherRingBuffer);
    
        void parseData(void *data, int size);
        AudioRingBuffer* clone() const;

        int16_t* getNextOutput();
        void setNextOutput(int16_t *newPointer);
        int16_t* getEndOfLastWrite();
        void setEndOfLastWrite(int16_t *newPointer);
        int16_t* getBuffer();
        bool isStarted();
        void setStarted(bool status);
        bool wasAddedToMix();
        void setAddedToMix(bool added);
        float* getPosition();
        void setPosition(float newPosition[]);
        float getAttenuationRatio();
        void setAttenuationRatio(float newAttenuation);
        float getBearing();
        void setBearing(float newBearing);
    
        short diffLastWriteNextOutput();
    private:
        int ringBufferLengthSamples;
        int bufferLengthSamples;
        float position[3];
        float attenuationRatio;
        float bearing;
        int16_t *nextOutput;
        int16_t *endOfLastWrite;
        int16_t *buffer;
        bool started;
        bool addedToMix;
};

#endif /* defined(__interface__AudioRingBuffer__) */
