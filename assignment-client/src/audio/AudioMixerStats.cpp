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
    hrtfResets = 0;
    hrtfUpdates = 0;

    manualStereoMixes = 0;
    manualEchoMixes = 0;

    skippedToActive = 0;
    skippedToInactive = 0;
    inactiveToSkipped = 0;
    inactiveToActive = 0;
    activeToSkipped = 0;
    activeToInactive = 0;

    skipped = 0;
    inactive = 0;
    active = 0;

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
    hrtfResets += otherStats.hrtfResets;
    hrtfUpdates += otherStats.hrtfUpdates;

    manualStereoMixes += otherStats.manualStereoMixes;
    manualEchoMixes += otherStats.manualEchoMixes;

    skippedToActive += otherStats.skippedToActive;
    skippedToInactive += otherStats.skippedToInactive;
    inactiveToSkipped += otherStats.inactiveToSkipped;
    inactiveToActive += otherStats.inactiveToActive;
    activeToSkipped += otherStats.activeToSkipped;
    activeToInactive += otherStats.activeToInactive;

    skipped += otherStats.skipped;
    inactive += otherStats.inactive;
    active += otherStats.active;

#ifdef HIFI_AUDIO_MIXER_DEBUG
    mixTime += otherStats.mixTime;
#endif
}
