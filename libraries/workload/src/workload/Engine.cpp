//
//  Engine.cpp
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.02.08
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Engine.h"

#include <iostream>

namespace workload {
    WorkloadContext::WorkloadContext(const SpacePointer& space) : task::JobContext(), _space(space) {}
} // namespace workload

