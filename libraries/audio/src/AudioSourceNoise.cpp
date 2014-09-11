//
//  AudioSourceNoise.cpp
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
#include "AudioSourceNoise.h"

template<> 
uint32_t AudioSourcePinkNoise::_randomSeed = 1974; // a truly random number
