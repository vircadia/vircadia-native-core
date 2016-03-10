//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Impl_ArrayClip_h
#define hifi_Recording_Impl_ArrayClip_h

#include "../Clip.h"

#include <mutex>

namespace recording {

template <typename T>
class ArrayClip : public Clip {
public:
    virtual float duration() const override {
        Locker lock(_mutex);
        if (_frames.empty()) {
            return 0;
        }
        return Frame::frameTimeToSeconds((*_frames.rbegin()).timeOffset);
    }

    virtual size_t frameCount() const override {
        Locker lock(_mutex);
        return _frames.size();
    }

    virtual Clip::Pointer duplicate() const override {
        auto result = newClip();
        Locker lock(_mutex);
        for (size_t i = 0; i < _frames.size(); ++i) {
            result->addFrame(readFrame(i));
        }
        return result;
    }

    virtual void seekFrameTime(Frame::Time offset) override {
        Locker lock(_mutex);
        auto itr = std::lower_bound(_frames.begin(), _frames.end(), offset,
                [](const T& a, Frame::Time b)->bool {
                return a.timeOffset < b;
            }
        );
        _frameIndex = itr - _frames.begin();
    }

    virtual Frame::Time positionFrameTime() const override {
        Locker lock(_mutex);
        Frame::Time result = Frame::INVALID_TIME;
        if (_frameIndex < _frames.size()) {
            result = _frames[_frameIndex].timeOffset;
        }
        return result;
    }

    virtual FrameConstPointer peekFrame() const override {
        Locker lock(_mutex);
        FrameConstPointer result;
        if (_frameIndex < _frames.size()) {
            result = readFrame(_frameIndex);
        }
        return result;
    }

    virtual FrameConstPointer nextFrame() override {
        Locker lock(_mutex);
        FrameConstPointer result;
        if (_frameIndex < _frames.size()) {
            result = readFrame(_frameIndex++);
        }
        return result;
    }

    virtual void skipFrame() override {
        Locker lock(_mutex);
        if (_frameIndex < _frames.size()) {
            ++_frameIndex;
        }
    }

protected:
    virtual void reset() override {
        _frameIndex = 0;
    }

    virtual FrameConstPointer readFrame(size_t index) const = 0;
    std::vector<T> _frames;
    mutable size_t _frameIndex { 0 };
};

}

#endif
