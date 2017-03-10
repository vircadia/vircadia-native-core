//
//  AudioNoiseGate.cpp
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstdlib>
#include <string.h>

#include <AudioConstants.h>

#include "AudioNoiseGate.h"

const float AudioNoiseGate::CLIPPING_THRESHOLD = 0.90f;

AudioNoiseGate::AudioNoiseGate() :
    _inputFrameCounter(0),
    _lastLoudness(0.0f),
    _quietestFrame(std::numeric_limits<float>::max()),
    _loudestFrame(0.0f),
    _didClipInLastFrame(false),
    _dcOffset(0.0f),
    _measuredFloor(0.0f),
    _sampleCounter(0),
    _isOpen(false),
    _framesToClose(0)
{
    
}

void AudioNoiseGate::removeDCOffset(int16_t* samples, int numSamples) {
    //
    //  DC Offset correction
    //
    //  Measure the DC offset over a trailing number of frames, and remove it from the input signal.
    //  This causes the noise background measurements and server muting to be more accurate.  Many off-board
    //  ADC's have a noticeable DC offset.
    //
    const float DC_OFFSET_AVERAGING = 0.99f;
    float measuredDcOffset = 0.0f;
    //  Remove trailing DC offset from samples
    for (int i = 0; i < numSamples; i++) {
        measuredDcOffset += samples[i];
        samples[i] -= (int16_t) _dcOffset;
    }
    // Update measured DC offset
    measuredDcOffset /= numSamples;
    if (_dcOffset == 0.0f) {
        // On first frame, copy over measured offset
        _dcOffset = measuredDcOffset;
    } else {
        _dcOffset = DC_OFFSET_AVERAGING * _dcOffset + (1.0f - DC_OFFSET_AVERAGING) * measuredDcOffset;
    }
}

void AudioNoiseGate::gateSamples(int16_t* samples, int numSamples) {
    //
    //  Impose Noise Gate
    //
    //  The Noise Gate is used to reject constant background noise by measuring the noise
    //  floor observed at the microphone and then opening the 'gate' to allow microphone
    //  signals to be transmitted when the microphone samples average level exceeds a multiple
    //  of the noise floor.
    //
    //  NOISE_GATE_HEIGHT:  How loud you have to speak relative to noise background to open the gate.
    //                      Make this value lower for more sensitivity and less rejection of noise.
    //  NOISE_GATE_WIDTH:   The number of samples in an audio frame for which the height must be exceeded
    //                      to open the gate.
    //  NOISE_GATE_CLOSE_FRAME_DELAY:  Once the noise is below the gate height for the frame, how many frames
    //                      will we wait before closing the gate.
    //  NOISE_GATE_FRAMES_TO_AVERAGE:  How many audio frames should we average together to compute noise floor.
    //                      More means better rejection but also can reject continuous things like singing.
    // NUMBER_OF_NOISE_SAMPLE_FRAMES:  How often should we re-evaluate the noise floor?

    float loudness = 0;
    int thisSample = 0;
    int samplesOverNoiseGate = 0;
    
    const float NOISE_GATE_HEIGHT = 7.0f;
    const int NOISE_GATE_WIDTH = 5;
    const int NOISE_GATE_CLOSE_FRAME_DELAY = 5;
    const int NOISE_GATE_FRAMES_TO_AVERAGE = 5;

    //  Check clipping, and check if should open noise gate
    _didClipInLastFrame = false;
    
    for (int i = 0; i < numSamples; i++) {
        thisSample = std::abs(samples[i]);
        if (thisSample >= ((float) AudioConstants::MAX_SAMPLE_VALUE * CLIPPING_THRESHOLD)) {
            _didClipInLastFrame = true;
        }
        
        loudness += thisSample;
        //  Noise Reduction:  Count peaks above the average loudness
        if (thisSample > (_measuredFloor * NOISE_GATE_HEIGHT)) {
            samplesOverNoiseGate++;
        }
    }
    
    _lastLoudness = fabs(loudness / numSamples);
    
    if (_quietestFrame > _lastLoudness) {
        _quietestFrame = _lastLoudness;
    }
    if (_loudestFrame < _lastLoudness) {
        _loudestFrame = _lastLoudness;
    }
    
    const int FRAMES_FOR_NOISE_DETECTION = 400;
    if (_inputFrameCounter++ > FRAMES_FOR_NOISE_DETECTION) {
        _quietestFrame = std::numeric_limits<float>::max();
        _loudestFrame = 0.0f;
        _inputFrameCounter = 0;
    }
    
    //  If Noise Gate is enabled, check and turn the gate on and off
    float averageOfAllSampleFrames = 0.0f;
    _sampleFrames[_sampleCounter++] = _lastLoudness;
    if (_sampleCounter == NUMBER_OF_NOISE_SAMPLE_FRAMES) {
        float smallestSample = std::numeric_limits<float>::max();
        for (int i = 0; i <= NUMBER_OF_NOISE_SAMPLE_FRAMES - NOISE_GATE_FRAMES_TO_AVERAGE; i += NOISE_GATE_FRAMES_TO_AVERAGE) {
            float thisAverage = 0.0f;
            for (int j = i; j < i + NOISE_GATE_FRAMES_TO_AVERAGE; j++) {
                thisAverage += _sampleFrames[j];
                averageOfAllSampleFrames += _sampleFrames[j];
            }
            thisAverage /= NOISE_GATE_FRAMES_TO_AVERAGE;
            
            if (thisAverage < smallestSample) {
                smallestSample = thisAverage;
            }
        }
        averageOfAllSampleFrames /= NUMBER_OF_NOISE_SAMPLE_FRAMES;
        _measuredFloor = smallestSample;
        _sampleCounter = 0;
        
    }

    if (samplesOverNoiseGate > NOISE_GATE_WIDTH) {
        _isOpen = true;
        _framesToClose = NOISE_GATE_CLOSE_FRAME_DELAY;
    } else {
        if (--_framesToClose == 0) {
            _closedInLastFrame = !_isOpen;
            _isOpen = false;
        }
    }
    if (!_isOpen) {
        memset(samples, 0, numSamples * sizeof(int16_t));
        _lastLoudness = 0;
    }
}
