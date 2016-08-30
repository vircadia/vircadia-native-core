//
//  AudioConstants.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioConstants_h
#define hifi_AudioConstants_h

#include <limits>
#include <math.h>
#include <stdint.h>

namespace AudioConstants {
    const int SAMPLE_RATE = 24000;
    const int MONO = 1;
    const int STEREO = 2;


    typedef int16_t AudioSample;

    inline const char* getAudioFrameName() { return "com.highfidelity.recording.Audio"; }

    const int MAX_CODEC_NAME_LENGTH = 30;
    const int MAX_CODEC_NAME_LENGTH_ON_WIRE = MAX_CODEC_NAME_LENGTH + sizeof(uint32_t);
    const int NETWORK_FRAME_BYTES_STEREO = 1024;
    const int NETWORK_FRAME_SAMPLES_STEREO = NETWORK_FRAME_BYTES_STEREO / sizeof(AudioSample);
    const int NETWORK_FRAME_BYTES_PER_CHANNEL = 512;
    const int NETWORK_FRAME_SAMPLES_PER_CHANNEL = NETWORK_FRAME_BYTES_PER_CHANNEL / sizeof(AudioSample);
    const float NETWORK_FRAME_SECS = (AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL / float(AudioConstants::SAMPLE_RATE));
    const float NETWORK_FRAME_MSECS = NETWORK_FRAME_SECS * 1000.0f;
    const float NETWORK_FRAMES_PER_SEC =  1.0f / NETWORK_FRAME_SECS;

    // be careful with overflows when using this constant
    const int NETWORK_FRAME_USECS = static_cast<int>(NETWORK_FRAME_MSECS * 1000.0f);
    
    const int MIN_SAMPLE_VALUE = std::numeric_limits<AudioSample>::min();
    const int MAX_SAMPLE_VALUE = std::numeric_limits<AudioSample>::max();
}


#endif // hifi_AudioConstants_h
