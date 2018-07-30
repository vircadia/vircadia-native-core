//
//  SpaceClassifier.cpp
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.02.21
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SpaceClassifier.h"

#include "ViewTask.h"
#include "RegionState.h"

using namespace workload;

void PerformSpaceTransaction::configure(const Config& config) {

}
void PerformSpaceTransaction::run(const WorkloadContextPointer& context) {
    context->_space->enqueueFrame();
    context->_space->processTransactionQueue();
}

void SpaceClassifierTask::build(JobModel& model, const Varying& in, Varying& out) {
    model.addJob<AssignSpaceViews >("assignSpaceViews", in);
    model.addJob<PerformSpaceTransaction>("updateSpace");
    const auto regionTrackerOut = model.addJob<RegionTracker>("regionTracker");
    const auto regionChanges = regionTrackerOut.getN<RegionTracker::Outputs>(1);
    model.addJob<RegionState>("regionState", regionChanges);
    out = regionTrackerOut;
}

