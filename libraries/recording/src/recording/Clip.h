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

#include <QtCore/QObject>

class QIODevice;

namespace recording {

class Clip : public QObject {
public:
    using Pointer = std::shared_ptr<Clip>;

    Clip(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~Clip() {}

    Pointer duplicate();

    virtual void seek(float offset) = 0;
    virtual float position() const = 0;

    virtual FramePointer peekFrame() const = 0;
    virtual FramePointer nextFrame() = 0;
    virtual void skipFrame() = 0;
    virtual void appendFrame(FramePointer) = 0;


    static Pointer fromFile(const QString& filePath);
    static void toFile(Pointer clip, const QString& filePath);
    
protected:
    virtual void reset() = 0;
};

}

#endif
