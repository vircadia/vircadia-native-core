//
//  AudioMixer.h
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioMixer__
#define __hifi__AudioMixer__

#include <Assignment.h>

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public Assignment {
public:
    AudioMixer(const unsigned char* dataBuffer, int numBytes);
    
    /// runs the audio mixer
    void run();
};

#endif /* defined(__hifi__AudioMixer__) */
