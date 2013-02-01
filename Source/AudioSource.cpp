//
//  AudioSource.cpp
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "AudioSource.h"

AudioSource::~AudioSource()
{
    delete oldestData;
    delete newestData;
}


int AudioSource::loadDataFromFile(const char *filename) {
//    FILE *soundFile = fopen(filename, "r");
//    
//    // get length of file:
//    std::fseek(soundFile, 0, SEEK_END);
//    lengthInSamples = std::ftell(soundFile) / sizeof(int16_t);
//    std::rewind(soundFile);
//    
//    sourceData = new int16_t[lengthInSamples];
//    std::fread(sourceData, sizeof(int16_t), lengthInSamples, soundFile);
//    
//    std::fclose(soundFile);
//    
    return 0;
}