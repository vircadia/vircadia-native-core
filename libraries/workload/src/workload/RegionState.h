//
//  RegionState.h
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.03.07
//  Copyright 2018 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple plane class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_workload_RegionState_h
#define hifi_workload_RegionState_h

#include "Space.h"
#include "Engine.h"

namespace workload {

    class RegionState {
    public:
        using Inputs = IndexVectors;
        using JobModel = workload::Job::ModelI<RegionState, Inputs>;

        RegionState() {
            _state.resize(Region::UNKNOWN);
        }

        void run(const workload::WorkloadContextPointer& renderContext, const Inputs& inputs);

    protected:
        IndexVectors _state;
    };
}
#endif // hifi_workload_RegionState_h
