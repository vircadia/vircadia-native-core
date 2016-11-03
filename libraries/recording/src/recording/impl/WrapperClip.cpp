//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WrapperClip.h"


#include <QtCore/QDebug>

#include <Finally.h>

#include "../Frame.h"
#include "../Logging.h"


using namespace recording;

WrapperClip::WrapperClip(const Clip::Pointer& wrappedClip)
    : _wrappedClip(wrappedClip) { }

void WrapperClip::seekFrameTime(Frame::Time offset) {
    _wrappedClip->seekFrameTime(offset);
}

Frame::Time WrapperClip::positionFrameTime() const {
    return _wrappedClip->position();
}

FrameConstPointer WrapperClip::peekFrame() const {
    return _wrappedClip->peekFrame();
}

FrameConstPointer WrapperClip::nextFrame() {
    return _wrappedClip->nextFrame();
}

void WrapperClip::skipFrame() {
    _wrappedClip->skipFrame();
}

void WrapperClip::reset() {
    _wrappedClip->reset();
}

void WrapperClip::addFrame(FrameConstPointer) {
    throw std::runtime_error("Wrapper clips are read only");
}

float WrapperClip::duration() const {
    return _wrappedClip->duration();
}

size_t WrapperClip::frameCount() const {
    return _wrappedClip->frameCount();
}

