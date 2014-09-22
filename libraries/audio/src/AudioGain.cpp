//
//  AudioGain.cpp
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
#include "AudioGain.h"

AudioGain::AudioGain() {
    initialize();
}

AudioGain::~AudioGain() {
    finalize();
}

void AudioGain::initialize() {
    setParameters(1.0f,0.0f);
}

void AudioGain::finalize() {
}

void AudioGain::reset() {
    initialize();
}

void AudioGain::setParameters(const float gain, const float mute) {
    _gain = std::min(std::max(gain, 0.0f), 1.0f);
    _mute = mute != 0.0f;
    
}

void AudioGain::getParameters(float& gain, float& mute) {
    gain = _gain;
    mute = _mute ? 1.0f : 0.0f;
}
