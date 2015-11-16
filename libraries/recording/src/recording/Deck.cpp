//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Deck.h"
 
#include <NumericalConstants.h>
#include <SharedUtil.h>

#include "Clip.h"
#include "Frame.h"
#include "Logging.h"

using namespace recording;

void Deck::queueClip(ClipPointer clip, Time timeOffset) {
    if (!clip) {
        qCWarning(recordingLog) << "Clip invalid, ignoring";
        return;
    }

    // FIXME if the time offset is not zero, wrap the clip in a OffsetClip wrapper
    _clips.push_back(clip);

    _length = std::max(_length, clip->duration());
}

void Deck::play() { 
    if (_pause) {
        _pause = false;
        _startEpoch = usecTimestampNow() - (_position * USECS_PER_MSEC);
        emit playbackStateChanged();
        processFrames();
    }
}

void Deck::pause() { 
    if (!_pause) {
        _pause = true;
        emit playbackStateChanged();
    }
}

Clip::Pointer Deck::getNextClip() {
    Clip::Pointer result;
    Time soonestFramePosition = INVALID_TIME;
    for (const auto& clip : _clips) {
        Time nextFramePosition = clip->position();
        if (nextFramePosition < soonestFramePosition) {
            result = clip;
            soonestFramePosition = nextFramePosition;
        }
    }
    return result;
}

void Deck::seek(Time position) {
    _position = position;
    // FIXME reset the frames to the appropriate spot
    for (auto& clip : _clips) {
        clip->seek(position);
    }

    if (!_pause) {
        // FIXME what if the timer is already running?
        processFrames();
    }
}

Time Deck::position() const {
    if (_pause) {
        return _position;
    }
    return (usecTimestampNow() - _startEpoch) / USECS_PER_MSEC;
}

static const Time MIN_FRAME_WAIT_INTERVAL_MS = 1;

void Deck::processFrames() {
    if (_pause) {
        return;
    }

    _position = position();
    auto triggerPosition = _position + MIN_FRAME_WAIT_INTERVAL_MS;
    Clip::Pointer nextClip;
    for (nextClip = getNextClip(); nextClip; nextClip = getNextClip()) {
        // If the clip is too far in the future, just break out of the handling loop
        Time framePosition = nextClip->position();
        if (framePosition > triggerPosition) {
            break;
        }

        // Handle the frame and advance the clip
        Frame::handleFrame(nextClip->nextFrame());
    }


    if (!nextClip) {
        qCDebug(recordingLog) << "No more frames available";
        // No more frames available, so handle the end of playback
        if (_loop) {
            qCDebug(recordingLog) << "Looping enabled, seeking back to beginning";
            // If we have looping enabled, start the playback over
            seek(0);
        } else {
            // otherwise pause playback
            pause();
        }
        return;
    } 

    // If we have more clip frames available, set the timer for the next one
    Time nextClipPosition = nextClip->position();
    Time interval = nextClipPosition - _position;
    _timer.singleShot(interval, [this] {
        processFrames();
    });
}
