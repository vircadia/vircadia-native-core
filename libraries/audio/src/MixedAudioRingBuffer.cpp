//
//  MixedAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-03-26.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "MixedAudioRingBuffer.h"

MixedAudioRingBuffer::MixedAudioRingBuffer(int numFrameSamples) :
    AudioRingBuffer(numFrameSamples),
    _lastReadFrameAverageLoudness(0.0f)
{
    
}

qint64 MixedAudioRingBuffer::readSamples(int16_t* destination, qint64 maxSamples) {
    // calculate the average loudness for the frame about to go out
    
    // read from _nextOutput either _numFrameSamples or to the end of the buffer
    int samplesFromNextOutput = _buffer + _sampleCapacity - _nextOutput;
    if (samplesFromNextOutput > _numFrameSamples) {
        samplesFromNextOutput = _numFrameSamples;
    }
    
    float averageLoudness = 0.0f;
    
    for (int s = 0; s < samplesFromNextOutput; s++) {
        averageLoudness += fabsf(_nextOutput[s]);
    }
    
    // read samples from the beginning of the buffer, if any
    int samplesFromBeginning = _numFrameSamples - samplesFromNextOutput;
    
    if (samplesFromBeginning > 0) {
        for (int b = 0; b < samplesFromBeginning; b++) {
            averageLoudness += fabsf(_buffer[b]);
        }
    }
    
    // divide by the number of samples and the MAX_SAMPLE_VALUE to get a float from 0 - 1
    averageLoudness /= (float) _numFrameSamples;
    averageLoudness /= (float) MAX_SAMPLE_VALUE;
    
    _lastReadFrameAverageLoudness = averageLoudness;
    
    return AudioRingBuffer::readSamples(destination, maxSamples);
}