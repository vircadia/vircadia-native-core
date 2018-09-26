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

#ifdef HIFI_AUDIO_MIXER_DEBUG
#include <cstdint>
#endif

struct AudioMixerStats {
    int sumStreams { 0 };
    int sumListeners { 0 };
    int sumListenersSilent { 0 };

    int totalMixes { 0 };

    int hrtfRenders { 0 };
    int hrtfResets { 0 };
    int hrtfUpdates { 0 };

    int manualStereoMixes { 0 };
    int manualEchoMixes { 0 };

    int skippedToActive { 0 };
    int skippedToInactive { 0 };
    int inactiveToSkipped { 0 };
    int inactiveToActive { 0 };
    int activeToSkipped { 0 };
    int activeToInactive { 0 };

    int skipped { 0 };
    int inactive { 0 };
    int active { 0 };

#ifdef HIFI_AUDIO_MIXER_DEBUG
    uint64_t mixTime { 0 };
#endif

    void reset();
    void accumulate(const AudioMixerStats& otherStats);
};

#endif // hifi_AudioMixerStats_h
