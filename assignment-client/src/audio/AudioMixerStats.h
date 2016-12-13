//
//  AudioMixerStats.h
//  assignment-client/src/audio
//
//  Created by Zach Pomerantz on 11/22/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixerStats_h
#define hifi_AudioMixerStats_h

struct AudioMixerStats {
    int sumStreams { 0 };
    int sumListeners { 0 };

    int totalMixes { 0 };

    int hrtfRenders { 0 };
    int hrtfSilentRenders { 0 };
    int hrtfStruggleRenders { 0 };

    int manualStereoMixes { 0 };
    int manualEchoMixes { 0 };

    void reset();
    void accumulate(const AudioMixerStats& otherStats);
};

#endif // hifi_AudioMixerStats_h
