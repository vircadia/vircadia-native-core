//
//  AudioNoiseGate.cpp
//  libraries/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioNoiseGate.h"

#include <cstdlib>
#include <string.h>

#include "AudioConstants.h"

const float AudioNoiseGate::CLIPPING_THRESHOLD = 0.90f;

AudioNoiseGate::AudioNoiseGate() :
    _lastLoudness(0.0f),
    _didClipInLastBlock(false),
    _dcOffset(0.0f),
    _measuredFloor(0.0f),
    _sampleCounter(0),
    _isOpen(false),
    _blocksToClose(0) {}

void AudioNoiseGate::removeDCOffset(int16_t* samples, int numSamples) {
    //
    //  DC Offset correction
    //
    //  Measure the DC offset over a trailing number of blocks, and remove it from the input signal.
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
        // On first block, copy over measured offset
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
    //  NOISE_GATE_WIDTH:   The number of samples in an audio block for which the height must be exceeded
    //                      to open the gate.
    //  NOISE_GATE_CLOSE_BLOCK_DELAY:  Once the noise is below the gate height for the block, how many blocks
    //                      will we wait before closing the gate.
    //  NOISE_GATE_BLOCKS_TO_AVERAGE:  How many audio blocks should we average together to compute noise floor.
    //                      More means better rejection but also can reject continuous things like singing.
    // NUMBER_OF_NOISE_SAMPLE_BLOCKS:  How often should we re-evaluate the noise floor?

    float loudness = 0;
    int thisSample = 0;
    int samplesOverNoiseGate = 0;

    const float NOISE_GATE_HEIGHT = 7.0f;
    const int NOISE_GATE_WIDTH = 5;
    const int NOISE_GATE_CLOSE_BLOCK_DELAY = 5;
    const int NOISE_GATE_BLOCKS_TO_AVERAGE = 5;

    //  Check clipping, and check if should open noise gate
    _didClipInLastBlock = false;

    for (int i = 0; i < numSamples; i++) {
        thisSample = std::abs(samples[i]);
        if (thisSample >= ((float) AudioConstants::MAX_SAMPLE_VALUE * CLIPPING_THRESHOLD)) {
            _didClipInLastBlock = true;
        }

        loudness += thisSample;
        //  Noise Reduction:  Count peaks above the average loudness
        if (thisSample > (_measuredFloor * NOISE_GATE_HEIGHT)) {
            samplesOverNoiseGate++;
        }
    }

    _lastLoudness = fabs(loudness / numSamples);

    //  If Noise Gate is enabled, check and turn the gate on and off
    float averageOfAllSampleBlocks = 0.0f;
    _sampleBlocks[_sampleCounter++] = _lastLoudness;
    if (_sampleCounter == NUMBER_OF_NOISE_SAMPLE_BLOCKS) {
        float smallestSample = std::numeric_limits<float>::max();
        for (int i = 0; i <= NUMBER_OF_NOISE_SAMPLE_BLOCKS - NOISE_GATE_BLOCKS_TO_AVERAGE; i += NOISE_GATE_BLOCKS_TO_AVERAGE) {
            float thisAverage = 0.0f;
            for (int j = i; j < i + NOISE_GATE_BLOCKS_TO_AVERAGE; j++) {
                thisAverage += _sampleBlocks[j];
                averageOfAllSampleBlocks += _sampleBlocks[j];
            }
            thisAverage /= NOISE_GATE_BLOCKS_TO_AVERAGE;

            if (thisAverage < smallestSample) {
                smallestSample = thisAverage;
            }
        }
        averageOfAllSampleBlocks /= NUMBER_OF_NOISE_SAMPLE_BLOCKS;
        _measuredFloor = smallestSample;
        _sampleCounter = 0;

    }

    _closedInLastBlock = false;
    _openedInLastBlock = false;

    if (samplesOverNoiseGate > NOISE_GATE_WIDTH) {
        _openedInLastBlock = !_isOpen;
        _isOpen = true;
        _blocksToClose = NOISE_GATE_CLOSE_BLOCK_DELAY;
    } else {
        if (--_blocksToClose == 0) {
            _closedInLastBlock = _isOpen;
            _isOpen = false;
        }
    }
    if (!_isOpen) {
        // First block after being closed gets faded to silence, we fade across
        // the entire block on fading out. All subsequent blocks are muted by being slammed
        // to zeros
        if (_closedInLastBlock) {
            float fadeSlope = (1.0f / numSamples);
            for (int i = 0; i < numSamples; i++) {
                float fadedSample = (1.0f - ((float)i * fadeSlope)) * (float)samples[i];
                samples[i] = (int16_t)fadedSample;
            }
        } else {
            memset(samples, 0, numSamples * sizeof(int16_t));
        }
        _lastLoudness = 0;
    }

    if (_openedInLastBlock) {
        // would be nice to do a little crossfade from silence, but we only want to fade
        // across the first 1/10th of the block, because we don't want to miss early
        // transients.
        int fadeSamples = numSamples / 10; // fade over 1/10th of the samples
        float fadeSlope = (1.0f / fadeSamples);
        for (int i = 0; i < fadeSamples; i++) {
            float fadedSample = (float)i * fadeSlope * (float)samples[i];
            samples[i] = (int16_t)fadedSample;
        }
    }
}
