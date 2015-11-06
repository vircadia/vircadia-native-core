//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Recorder.h"

#include <NumericalConstants.h>

#include "impl/BufferClip.h"
#include "Frame.h"

using namespace recording;

void Recorder::start() {
    if (!_recording) {
        _recording = true;
        if (!_clip) {
            _clip = std::make_shared<BufferClip>();
        }
        _timer.start();
        emit recordingStateChanged();
    }
}

void Recorder::stop() {
    if (!_recording) {
        _recording = false;
        _elapsed = _timer.elapsed();
        emit recordingStateChanged();
    }
}

bool Recorder::isRecording() {
    return _recording;
}

void Recorder::clear() {
    _clip.reset();
}

void Recorder::recordFrame(FrameType type, QByteArray frameData) {
    if (!_recording || !_clip) {
        return;
    }

    Frame::Pointer frame = std::make_shared<Frame>();
    frame->type = type;
    frame->data = frameData;
    frame->timeOffset = (float)(_elapsed + _timer.elapsed()) / MSECS_PER_SECOND;
    _clip->appendFrame(frame);
}

ClipPointer Recorder::getClip() {
    auto result = _clip;
    _clip.reset();
    return result;
}

