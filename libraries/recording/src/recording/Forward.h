//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Forward_h
#define hifi_Recording_Forward_h

#include <memory>
#include <list>
#include <limits>

namespace recording {

using FrameType = uint16_t;

using FrameSize = uint16_t;

struct Frame;

using FramePointer = std::shared_ptr<Frame>;

using FrameConstPointer = std::shared_ptr<const Frame>;

// A recording of some set of state from the application, usually avatar
// data + audio for a single person
class Clip;

using ClipPointer = std::shared_ptr<Clip>;

using ClipConstPointer = std::shared_ptr<const Clip>;

// An interface for playing back clips
class Deck;

// An interface for recording a single clip
class Recorder;

}

#endif
