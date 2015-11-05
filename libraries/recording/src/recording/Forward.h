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

// A recording of some set of state from the application, usually avatar
// data + audio for a single person
class Clip;

// An interface for interacting with clips, creating them by recording or
// playing them back.  Also serialization to and from files / network sources
class Deck;

}

#endif
