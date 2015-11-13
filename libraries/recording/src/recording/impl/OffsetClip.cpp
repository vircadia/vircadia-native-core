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

OffsetClip::OffsetClip(const Clip::Pointer& wrappedClip, Time offset) 
    : WrapperClip(wrappedClip), _offset(offset) { }

void OffsetClip::seek(Time offset) {
    _wrappedClip->seek(offset - _offset);
}

Time OffsetClip::position() const {
    return _wrappedClip->position() + _offset;
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

Time OffsetClip::duration() const {
    return _wrappedClip->duration() + _offset;
}

