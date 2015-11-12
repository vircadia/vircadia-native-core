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

    virtual ~BufferClip() {}

    virtual Time duration() const override;
    virtual size_t frameCount() const override;

    virtual void seek(Time offset) override;
    virtual Time position() const override;

    virtual FramePointer peekFrame() const override;
    virtual FramePointer nextFrame() override;
    virtual void skipFrame() override;
    virtual void addFrame(FramePointer) override;

private:
    virtual void reset() override;

    std::vector<FramePointer> _frames;
    mutable size_t _frameIndex { 0 };
};

}

#endif
