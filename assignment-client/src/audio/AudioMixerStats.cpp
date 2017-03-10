//
//  AudioMixerStats.cpp
//  assignment-client/src/audio
//
//  Created by Zach Pomerantz on 11/22/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioMixerStats.h"

void AudioMixerStats::reset() {
    sumStreams = 0;
    sumListeners = 0;
    sumListenersSilent = 0;
    totalMixes = 0;
    hrtfRenders = 0;
    hrtfSilentRenders = 0;
    hrtfThrottleRenders = 0;
    manualStereoMixes = 0;
    manualEchoMixes = 0;
#ifdef HIFI_AUDIO_MIXER_DEBUG
    mixTime = 0;
#endif
}

void AudioMixerStats::accumulate(const AudioMixerStats& otherStats) {
    sumStreams += otherStats.sumStreams;
    sumListeners += otherStats.sumListeners;
    sumListenersSilent += otherStats.sumListenersSilent;
    totalMixes += otherStats.totalMixes;
    hrtfRenders += otherStats.hrtfRenders;
    hrtfSilentRenders += otherStats.hrtfSilentRenders;
    hrtfThrottleRenders += otherStats.hrtfThrottleRenders;
    manualStereoMixes += otherStats.manualStereoMixes;
    manualEchoMixes += otherStats.manualEchoMixes;
#ifdef HIFI_AUDIO_MIXER_DEBUG
    mixTime += otherStats.mixTime;
#endif
}
