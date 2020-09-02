//
//  CrashHelpers.h
//  libraries/shared/src
//
//  Created by Ryan Huffman on 11 April 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_CrashHelpers_h
#define hifi_CrashHelpers_h

#include "SharedLogging.h"

namespace crash {

void doAssert(bool value); // works for Release
void pureVirtualCall();
void doubleFree();
void nullDeref();
void doAbort();
void outOfBoundsVectorCrash();
void newFault();
void throwException();

}

#endif // hifi_CrashHelpers_h
