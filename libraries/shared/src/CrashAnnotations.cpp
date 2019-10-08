//
//  CrashAnnotations.cpp
//  libraries/shared/src
//
//  Created by Clement Brisset on 9/26/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SharedUtil.h"

// Global variables specifically tuned to work with ptrace module for crash collection
// Do not move/rename unless you update the ptrace module with it.
bool crash_annotation_isShuttingDown = false;

namespace crash {
namespace annotations {

void setShutdownState(bool isShuttingDown) {
    crash_annotation_isShuttingDown = isShuttingDown;
}


};
};
