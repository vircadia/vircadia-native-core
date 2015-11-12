//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BufferClip.h"

#include <NumericalConstants.h>

#include "../Frame.h"

using namespace recording;


void BufferClip::seek(Time offset) {
    Locker lock(_mutex);
    auto itr = std::lower_bound(_frames.begin(), _frames.end(), offset,
        [](Frame::ConstPointer a, Time b)->bool {
            return a->timeOffset < b;
        }
    );
    _frameIndex = itr - _frames.begin();
}

Time BufferClip::position() const {
    Locker lock(_mutex);
    Time result = INVALID_TIME;
    if (_frameIndex < _frames.size()) {
        result = _frames[_frameIndex]->timeOffset;
    }
    return result;
}

FrameConstPointer BufferClip::peekFrame() const {
    Locker lock(_mutex);
    FrameConstPointer result;
    if (_frameIndex < _frames.size()) {
        result = _frames[_frameIndex];
    }
    return result;
}

FrameConstPointer BufferClip::nextFrame() {
    Locker lock(_mutex);
    FrameConstPointer result;
    if (_frameIndex < _frames.size()) {
        result = _frames[_frameIndex];
        ++_frameIndex;
    }
    return result;
}

void BufferClip::addFrame(FrameConstPointer newFrame) {
    if (newFrame->timeOffset < 0.0f) {
        throw std::runtime_error("Frames may not have negative time offsets");
    }
    auto currentPosition = position();
    seek(newFrame->timeOffset);
    {
        Locker lock(_mutex);

        _frames.insert(_frames.begin() + _frameIndex, newFrame);
    }
    seek(currentPosition);
}

void BufferClip::skipFrame() {
    Locker lock(_mutex);
    if (_frameIndex < _frames.size()) {
        ++_frameIndex;
    }
}

void BufferClip::reset() {
    Locker lock(_mutex);
    _frameIndex = 0;
}

Time BufferClip::duration() const {
    if (_frames.empty()) {
        return 0;
    }
    return (*_frames.rbegin())->timeOffset;
}

size_t BufferClip::frameCount() const {
    return _frames.size();
}

