//
//  AudioInjectorOptions.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioInjectorOptions.h"

#include "AudioLogging.h"

AudioInjectorOptions::AudioInjectorOptions() :
    position(0.0f, 0.0f, 0.0f),
    positionSet(true),  // default to legacy behavior
    volume(1.0f),
    loop(false),
    orientation(glm::vec3(0.0f, 0.0f, 0.0f)),
    stereo(false),
    ambisonic(false),
    ignorePenumbra(false),
    localOnly(false),
    secondOffset(0.0f),
    pitch(1.0f)
{
}

