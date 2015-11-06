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

namespace recording {

using FrameType = uint16_t;

struct Frame;

using FramePointer = std::shared_ptr<Frame>;

// A recording of some set of state from the application, usually avatar
// data + audio for a single person
class Clip;

using ClipPointer = std::shared_ptr<Clip>;

// An interface for playing back clips
class Deck;

using DeckPointer = std::shared_ptr<Deck>;

// An interface for recording a single clip
class Recorder;

using RecorderPointer = std::shared_ptr<Recorder>;

}

#endif
