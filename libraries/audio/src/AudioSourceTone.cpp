//
//  AudioSourceTone.cpp
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/10/14.
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
#include "AudioSourceTone.h"

AudioSourceTone::AudioSourceTone() {
    initialize();
}

AudioSourceTone::~AudioSourceTone() {
    finalize();
}

void AudioSourceTone::finalize() {
}

void AudioSourceTone::reset() {
}

void AudioSourceTone::updateCoefficients() {
    _omega = _frequency / _sampleRate * TWO_PI;
    _epsilon = 2.0f * sinf(_omega / 2.0f);
    _yq1 = cosf(-1.0f * _omega);
    _y1 = sinf(+1.0f * _omega);   
}

void AudioSourceTone::initialize() {
    const float32_t FREQUENCY_220_HZ = 220.0f;
    const float32_t GAIN_MINUS_3DB = 0.708f;
    setParameters(SAMPLE_RATE, FREQUENCY_220_HZ, GAIN_MINUS_3DB);
}

void AudioSourceTone::setParameters(const float32_t sampleRate, const float32_t frequency,  const float32_t amplitude) {
    _sampleRate = std::max(sampleRate, 1.0f);
    _frequency = std::max(frequency, 1.0f);
    _amplitude = std::max(amplitude, 1.0f);
    updateCoefficients();
}

void AudioSourceTone::getParameters(float32_t& sampleRate, float32_t& frequency, float32_t& amplitude) {
    sampleRate = _sampleRate;
    frequency = _frequency;
    amplitude = _amplitude;
}

