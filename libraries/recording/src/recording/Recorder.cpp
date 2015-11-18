//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Recorder.h"

#include <NumericalConstants.h>
#include <SharedUtil.h>

#include "impl/BufferClip.h"
#include "Frame.h"

using namespace recording;

Recorder::Recorder(QObject* parent) 
    : QObject(parent) {}

float Recorder::position() {
    Locker lock(_mutex);
    if (_clip) {
        return _clip->duration();
    }
    return 0.0f;
}

void Recorder::start() {
    Locker lock(_mutex);
    if (!_recording) {
        _recording = true;
        // FIXME for now just record a new clip every time
        _clip = std::make_shared<BufferClip>();
        _startEpoch = usecTimestampNow();
        _timer.start();
        emit recordingStateChanged();
    }
}

void Recorder::stop() {
    Locker lock(_mutex);
    if (_recording) {
        _recording = false;
        _elapsed = _timer.elapsed();
        emit recordingStateChanged();
    }
}

bool Recorder::isRecording() {
    Locker lock(_mutex);
    return _recording;
}

void Recorder::clear() {
    Locker lock(_mutex);
    _clip.reset();
}

void Recorder::recordFrame(FrameType type, QByteArray frameData) {
    Locker lock(_mutex);
    if (!_recording || !_clip) {
        return;
    }

    Frame::Pointer frame = std::make_shared<Frame>();
    frame->type = type;
    frame->data = frameData;
    frame->timeOffset = (usecTimestampNow() - _startEpoch) / USECS_PER_MSEC;
    _clip->addFrame(frame);
}

ClipPointer Recorder::getClip() {
    Locker lock(_mutex);
    return _clip;
}

