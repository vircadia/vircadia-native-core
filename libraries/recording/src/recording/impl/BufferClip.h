//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Impl_BufferClip_h
#define hifi_Recording_Impl_BufferClip_h

#include "../Clip.h"

#include <mutex>

namespace recording {

class BufferClip : public Clip {
public:
    using Pointer = std::shared_ptr<BufferClip>;

    BufferClip(QObject* parent = nullptr) : Clip(parent) {}
    virtual ~BufferClip() {}

    virtual void seek(float offset) override;
    virtual float position() const override;

    virtual FramePointer peekFrame() const override;
    virtual FramePointer nextFrame() override;
    virtual void skipFrame() override;
    virtual void appendFrame(FramePointer) override;

private:
    using Mutex = std::mutex;
    using Locker = std::unique_lock<Mutex>;

    virtual void reset() override;

    std::vector<FramePointer> _frames;
    mutable Mutex _mutex;
    mutable size_t _frameIndex { 0 };
};

}

#endif
