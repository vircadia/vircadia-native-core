//
//  AudioPan.cpp
//  hifi
//
//  Created by Craig Hansen-Sturm on 9/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <math.h>
#include <SharedUtil.h>
#include "AudioRingBuffer.h"
#include "AudioFormat.h"
#include "AudioBuffer.h"
#include "AudioPan.h"

float32_t AudioPan::ONE_MINUS_EPSILON = 1.0f - EPSILON;
float32_t AudioPan::ZERO_PLUS_EPSILON = 0.0f + EPSILON;
float32_t AudioPan::ONE_HALF_MINUS_EPSILON = 0.5f - EPSILON;
float32_t AudioPan::ONE_HALF_PLUS_EPSILON = 0.5f + EPSILON;

AudioPan::AudioPan() {
    initialize();
}

AudioPan::~AudioPan() {
    finalize();
}

void AudioPan::initialize() {
    setParameters(0.5f);
}

void AudioPan::finalize() {
}

void AudioPan::reset() {
    initialize();
}

void AudioPan::setParameters(const float32_t pan) {
    // pan ranges between 0.0 and 1.0f inclusive.  0.5f is midpoint between full left and full right
    _pan = std::min(std::max(pan, 0.0f), 1.0f);
    updateCoefficients();
}

void AudioPan::getParameters(float32_t& pan) {
    pan = _pan;
}
