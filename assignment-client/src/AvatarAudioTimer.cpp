//
//  AvatarAudioTimer.cpp
//  assignment-client/src
//
//  Created by David Kelly on 10/12/13.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <QDebug>
#include <SharedUtil.h>
#include "AvatarAudioTimer.h"

// this should send a signal every 10ms, with pretty good precision.  Hardcoding
// to 10ms since that's what you'd want for audio.  
void AvatarAudioTimer::start() {
    qDebug() << __FUNCTION__;
    auto startTime = usecTimestampNow();
    quint64 frameCounter = 0;
    const int TARGET_INTERVAL_USEC = 10000; // 10ms
    while (!_quit) {
        ++frameCounter;

        // tick every 10ms from startTime
        quint64 targetTime = startTime + frameCounter * TARGET_INTERVAL_USEC;
        quint64 now = usecTimestampNow();

        // avoid quint64 underflow
        if (now < targetTime) {
            usleep(targetTime - now);
        }

        emit avatarTick();
    }
    qDebug() << "AvatarAudioTimer is finished";
}
