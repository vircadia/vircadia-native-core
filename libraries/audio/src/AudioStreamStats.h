//
//  AudioStreamStats.h
//  libraries/audio/src
//
//  Created by Yixin Wang on 6/25/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioStreamStats_h
#define hifi_AudioStreamStats_h

class AudioMixerJitterBuffersStats {
public:
    AudioMixerJitterBuffersStats()
        : _avatarJitterBufferFrames(0), _maxJitterBufferFrames(0), _avgJitterBufferFrames(0)
    {}

    quint16 _avatarJitterBufferFrames;
    quint16 _maxJitterBufferFrames;
    float _avgJitterBufferFrames;
};

#endif  // hifi_AudioStreamStats_h
