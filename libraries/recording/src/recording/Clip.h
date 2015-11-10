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

class QIODevice;

namespace recording {

class Clip {
public:
    using Pointer = std::shared_ptr<Clip>;

    virtual ~Clip() {}

    Pointer duplicate();

    virtual Time duration() const = 0;
    virtual size_t frameCount() const = 0;

    virtual void seek(Time offset) = 0;
    virtual Time position() const = 0;

    virtual FramePointer peekFrame() const = 0;
    virtual FramePointer nextFrame() = 0;
    virtual void skipFrame() = 0;
    virtual void addFrame(FramePointer) = 0;

    static Pointer fromFile(const QString& filePath);
    static void toFile(const QString& filePath, Pointer clip);
    static Pointer newClip();
    
protected:
    using Mutex = std::recursive_mutex;
    using Locker = std::unique_lock<Mutex>;

    virtual void reset() = 0;

    mutable Mutex _mutex;
};

}

#endif
