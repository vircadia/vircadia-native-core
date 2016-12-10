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
    totalMixes = 0;
    hrtfRenders = 0;
    hrtfSilentRenders = 0;
    hrtfStruggleRenders = 0;
    manualStereoMixes = 0;
    manualEchoMixes = 0;
}

void AudioMixerStats::accumulate(const AudioMixerStats& otherStats) {
    sumStreams += otherStats.sumStreams;
    sumListeners += otherStats.sumListeners;
    totalMixes += otherStats.totalMixes;
    hrtfRenders += otherStats.hrtfRenders;
    hrtfSilentRenders += otherStats.hrtfSilentRenders;
    hrtfStruggleRenders += otherStats.hrtfStruggleRenders;
    manualStereoMixes += otherStats.manualStereoMixes;
    manualEchoMixes += otherStats.manualEchoMixes;
}
