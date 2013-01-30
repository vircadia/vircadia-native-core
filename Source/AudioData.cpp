//
//  AudioData.cpp
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "AudioData.h"

AudioData::AudioData(int numberOfSources, int bufferLength) {
    _numberOfSources = numberOfSources;
    
    sources = new AudioSource*[numberOfSources];
    
    for(int s = 0; s < numberOfSources; s++) {
        sources[s] = new AudioSource();
        std::cout << "Created a new audio source!\n";
    }
    
    samplesToQueue = new int16_t[bufferLength / sizeof(int16_t)];
}

AudioData::~AudioData() {
    for (int s = 0; s < _numberOfSources; s++) {
        delete sources[s];
    }
    
    delete[] samplesToQueue;
}