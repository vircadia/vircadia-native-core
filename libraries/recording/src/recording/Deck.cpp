//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Deck.h"
 
#include <QtCore/QThread>

#include <NumericalConstants.h>
#include <SharedUtil.h>

#include "Clip.h"
#include "Frame.h"
#include "Logging.h"
#include "impl/OffsetClip.h"

using namespace recording;

Deck::Deck(QObject* parent) 
    : QObject(parent) {}

void Deck::queueClip(ClipPointer clip, float timeOffset) {
    Locker lock(_mutex);

    if (!clip) {
        qCWarning(recordingLog) << "Clip invalid, ignoring";
        return;
    }

    // FIXME disabling multiple clips for now
    _clips.clear();

    // if the time offset is not zero, wrap in an OffsetClip
    if (timeOffset != 0.0f) {
        clip = std::make_shared<OffsetClip>(clip, timeOffset);
    }

    _clips.push_back(clip);

    _length = std::max(_length, clip->duration());
}

void Deck::play() { 
    Locker lock(_mutex);
    if (_pause) {
        _pause = false;
        _startEpoch = Frame::epochForFrameTime(_position);
        emit playbackStateChanged();
        processFrames();
    }
}

void Deck::pause() { 
    Locker lock(_mutex);
    if (!_pause) {
        _pause = true;
        emit playbackStateChanged();
    }
}

Clip::Pointer Deck::getNextClip() {
    Clip::Pointer result;
    auto soonestFramePosition = Frame::INVALID_TIME;
    for (const auto& clip : _clips) {
        auto nextFramePosition = clip->positionFrameTime();
        if (nextFramePosition < soonestFramePosition) {
            result = clip;
            soonestFramePosition = nextFramePosition;
        }
    }
    return result;
}

void Deck::seek(float position) {
    Locker lock(_mutex);
    _position = Frame::secondsToFrameTime(position);

    // Recompute the start epoch
    _startEpoch = Frame::epochForFrameTime(_position);

    // reset the clips to the appropriate spot
    for (auto& clip : _clips) {
        clip->seekFrameTime(_position);
    }

    if (!_pause) {
        // FIXME what if the timer is already running?
        processFrames();
    }
}

float Deck::position() const {
    Locker lock(_mutex);
    auto currentPosition = _position;
    if (!_pause) {
        currentPosition = Frame::frameTimeFromEpoch(_startEpoch);
    }
    return Frame::frameTimeToSeconds(currentPosition);
}

static const Frame::Time MIN_FRAME_WAIT_INTERVAL = Frame::secondsToFrameTime(0.001f);
static const Frame::Time MAX_FRAME_PROCESSING_TIME = Frame::secondsToFrameTime(0.004f);

void Deck::processFrames() {
    if (qApp->thread() != QThread::currentThread()) {
        qWarning() << "Processing frames must only happen on the main thread.";
        return;
    }
    Locker lock(_mutex);
    if (_pause) {
        return;
    }

    auto startingPosition = Frame::frameTimeFromEpoch(_startEpoch);
    auto triggerPosition = startingPosition + MIN_FRAME_WAIT_INTERVAL;
    Clip::Pointer nextClip;
    // FIXME add code to start dropping frames if we fall behind.
    // Alternatively, add code to cache frames here and then process only the last frame of a given type
    // ... the latter will work for Avatar, but not well for audio I suspect.
    bool overLimit = false;
    for (nextClip = getNextClip(); nextClip; nextClip = getNextClip()) {
        auto currentPosition = Frame::frameTimeFromEpoch(_startEpoch);
        if ((currentPosition - startingPosition) >= MAX_FRAME_PROCESSING_TIME) {
            qCWarning(recordingLog) << "Exceeded maximum frame processing time, breaking early";
#ifdef WANT_RECORDING_DEBUG            
            qCDebug(recordingLog) << "Starting: " << currentPosition;
            qCDebug(recordingLog) << "Current:  " << startingPosition;
            qCDebug(recordingLog) << "Trigger:  " << triggerPosition;
#endif
            overLimit = true;
            break;
        }

        // If the clip is too far in the future, just break out of the handling loop
        Frame::Time framePosition = nextClip->positionFrameTime();
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
            // FIXME configure the recording scripting interface to reset the avatar basis on a loop 
            // if doing relative movement
            emit looped();
        } else {
            // otherwise pause playback
            pause();
        }
        return;
    } 

    // If we have more clip frames available, set the timer for the next one
    _position = Frame::frameTimeFromEpoch(_startEpoch);
    int nextInterval = 1;
    if (!overLimit) {
        auto nextFrameTime = nextClip->positionFrameTime();
        nextInterval = (int)Frame::frameTimeToMilliseconds(nextFrameTime - _position);
#ifdef WANT_RECORDING_DEBUG
        qCDebug(recordingLog) << "Now " << _position;
        qCDebug(recordingLog) << "Next frame time " << nextInterval;
#endif
    }
#ifdef WANT_RECORDING_DEBUG
    qCDebug(recordingLog) << "Setting timer for next processing " << nextInterval;
#endif
    _timer.singleShot(nextInterval, [this] {
        processFrames();
    });
}

void Deck::removeClip(const ClipConstPointer& clip) {
    Locker lock(_mutex);
    std::remove_if(_clips.begin(), _clips.end(), [&](const Clip::ConstPointer& testClip)->bool {
        return (clip == testClip);
    });
}

void Deck::removeClip(const QString& clipName) {
    Locker lock(_mutex);
    std::remove_if(_clips.begin(), _clips.end(), [&](const Clip::ConstPointer& clip)->bool {
        return (clip->getName() == clipName);
    });
}

void Deck::removeAllClips() {
    Locker lock(_mutex);
    _clips.clear();
}

Deck::ClipList Deck::getClips(const QString& clipName) const {
    Locker lock(_mutex);
    ClipList result = _clips;
    return result;
}


bool Deck::isPlaying() { 
    Locker lock(_mutex);
    return !_pause; 
}

bool Deck::isPaused() const { 
    Locker lock(_mutex);
    return _pause;
}

void Deck::stop() { 
    Locker lock(_mutex);
    pause();
    seek(0.0f); 
}

float Deck::length() const { 
    Locker lock(_mutex);
    return _length;
}

void Deck::loop(bool enable) { 
    Locker lock(_mutex);
    _loop = enable;
}

bool Deck::isLooping() const { 
    Locker lock(_mutex);
    return _loop;
}
