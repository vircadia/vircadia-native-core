//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OffsetClip.h"

#include <algorithm>

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <Finally.h>

#include "../Frame.h"
#include "../Logging.h"


using namespace recording;

OffsetClip::OffsetClip(const Clip::Pointer& wrappedClip, float offset)
    : WrapperClip(wrappedClip), _offset(Frame::secondsToFrameTime(offset)) { }

void OffsetClip::seekFrameTime(Frame::Time offset) {
    _wrappedClip->seekFrameTime(offset - _offset);
}

Frame::Time OffsetClip::positionFrameTime() const {
    return _wrappedClip->positionFrameTime() + _offset;
}

FrameConstPointer OffsetClip::peekFrame() const {
    auto result = std::make_shared<Frame>(*_wrappedClip->peekFrame());
    result->timeOffset += _offset;
    return result;
}

FrameConstPointer OffsetClip::nextFrame() {
    auto result = std::make_shared<Frame>(*_wrappedClip->nextFrame());
    result->timeOffset += _offset;
    return result;
}

float OffsetClip::duration() const {
    return _wrappedClip->duration() + _offset;
}

QString OffsetClip::getName() const {
    return _wrappedClip->getName();
}

Clip::Pointer OffsetClip::duplicate() const {
    return std::make_shared<OffsetClip>(
        _wrappedClip->duplicate(), Frame::frameTimeToSeconds(_offset));
}



