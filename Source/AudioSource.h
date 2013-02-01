//
//  AudioSource.h
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioSource__
#define __interface__AudioSource__

#include <iostream>
#include "glm.hpp"

class AudioSource {
    public:
        glm::vec3 position;
    
        struct JitterBuffer {
            int16_t *audioData;
            timeval receiveTime;
            
            ~JitterBuffer() { delete[] audioData; }
        } *oldestData, *newestData;
    
//        int lengthInSamples;
//        int samplePointer;
    
//        AudioSource() { samplePointer = 0; lengthInSamples = 0; }
        AudioSource() { oldestData = NULL; newestData = NULL; }
        ~AudioSource();
        
        int loadDataFromFile(const char *filename);
};

#endif /* defined(__interface__AudioSource__) */
