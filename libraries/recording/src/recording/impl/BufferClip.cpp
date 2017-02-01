//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BufferClip.h"

#include <QtCore/QDebug>

#include <NumericalConstants.h>
#include "../Frame.h"

using namespace recording;

QString BufferClip::getName() const {
    return _name;
}


void BufferClip::addFrame(FrameConstPointer newFrame) {
    if (newFrame->timeOffset < 0.0f) {
        throw std::runtime_error("Frames may not have negative time offsets");
    }

    Locker lock(_mutex);
    auto itr = std::lower_bound(_frames.begin(), _frames.end(), newFrame->timeOffset,
        [](const Frame& a, Frame::Time b)->bool {
            return a.timeOffset < b;
        }
    );

    auto newFrameIndex = itr - _frames.begin();
    //qDebug(recordingLog) << "Adding frame with time offset " << newFrame->timeOffset << " @ index " << newFrameIndex;
    _frames.insert(_frames.begin() + newFrameIndex, Frame(*newFrame));
}

// Internal only function, needs no locking
FrameConstPointer BufferClip::readFrame(size_t frameIndex) const {
    FramePointer result;
    if (frameIndex < _frames.size()) {
        result = std::make_shared<Frame>(_frames[frameIndex]);
    }
    return result;
}
