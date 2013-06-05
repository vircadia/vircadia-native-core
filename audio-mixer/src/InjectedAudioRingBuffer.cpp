//
//  InjectedAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "InjectedAudioRingBuffer.h"

InjectedAudioRingBuffer::InjectedAudioRingBuffer() :
    _radius(0.0f),
    _attenuationRatio(0),
    _streamIdentifier()
{
    
}