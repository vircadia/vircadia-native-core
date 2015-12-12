//
//  Created by Bradley Austin Davis on 2015/12/10
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NsightHelpers.h"

#if defined(NSIGHT_FOUND)
#include "nvToolsExt.h"

ProfileRange::ProfileRange(const char *name) {
    nvtxRangePush(name);
}

ProfileRange::~ProfileRange() {
    nvtxRangePop();
}

#endif
