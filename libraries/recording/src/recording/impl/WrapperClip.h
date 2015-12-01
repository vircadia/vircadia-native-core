//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Impl_WrapperClip_h
#define hifi_Recording_Impl_WrapperClip_h

#include "../Clip.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>

#include <mutex>

namespace recording {

class WrapperClip : public Clip {
public:
    using Pointer = std::shared_ptr<WrapperClip>;

    WrapperClip(const Clip::Pointer& wrappedClip);

    virtual float duration() const override;
    virtual size_t frameCount() const override;

    virtual void seekFrameTime(Frame::Time offset) override;
    virtual Frame::Time positionFrameTime() const override;

    virtual FrameConstPointer peekFrame() const override;
    virtual FrameConstPointer nextFrame() override;
    virtual void skipFrame() override;
    virtual void addFrame(FrameConstPointer) override;

protected:
    virtual void reset() override;

    const Clip::Pointer _wrappedClip;
};

}

#endif
