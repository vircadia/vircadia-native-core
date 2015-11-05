//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BufferClip.h"

#include "../Frame.h"

using namespace recording;


void BufferClip::seek(float offset) {
    Locker lock(_mutex);
    auto itr = std::lower_bound(_frames.begin(), _frames.end(), offset, 
        [](Frame::Pointer a, float b)->bool{
            return a->timeOffset < b;
        }
    );
    _frameIndex = itr - _frames.begin();
}

float BufferClip::position() const {
    Locker lock(_mutex);
    float result = std::numeric_limits<float>::max();
    if (_frameIndex < _frames.size()) {
        result = _frames[_frameIndex]->timeOffset;
    }
    return result;
}

FramePointer BufferClip::peekFrame() const {
    Locker lock(_mutex);
    FramePointer result;
    if (_frameIndex < _frames.size()) {
        result = _frames[_frameIndex];
    }
    return result;
}

FramePointer BufferClip::nextFrame() {
    Locker lock(_mutex);
    FramePointer result;
    if (_frameIndex < _frames.size()) {
        result = _frames[_frameIndex];
        ++_frameIndex;
    }
    return result;
}

void BufferClip::appendFrame(FramePointer newFrame) {
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
