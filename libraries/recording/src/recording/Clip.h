//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Clip_h
#define hifi_Recording_Clip_h

#include "Forward.h"

#include <mutex>

#include <QtCore/QObject>

#include "Frame.h"

class QIODevice;

namespace recording {

class Clip {
public:
    using Pointer = std::shared_ptr<Clip>;
    using ConstPointer = std::shared_ptr<const Clip>;

    virtual ~Clip() {}

    virtual Pointer duplicate() const = 0;

    virtual QString getName() const = 0;

    virtual float duration() const = 0;
    virtual size_t frameCount() const = 0;

    virtual void seek(float offset) final;
    virtual float position() const final;

    virtual void seekFrameTime(Frame::Time offset) = 0;
    virtual Frame::Time positionFrameTime() const = 0;

    virtual FrameConstPointer peekFrame() const = 0;
    virtual FrameConstPointer nextFrame() = 0;
    virtual void skipFrame() = 0;
    virtual void addFrame(FrameConstPointer) = 0;

    bool write(QIODevice& output);

    static Pointer fromFile(const QString& filePath);
    static void toFile(const QString& filePath, const ConstPointer& clip);
    static QByteArray toBuffer(const ConstPointer& clip);
    static Pointer newClip();
    
    static const QString FRAME_TYPE_MAP;
    static const QString FRAME_COMREPSSION_FLAG;

protected:
    friend class WrapperClip;
    using Mutex = std::recursive_mutex;
    using Locker = std::unique_lock<Mutex>;

    virtual void reset() = 0;

    mutable Mutex _mutex;
};

}

#endif
