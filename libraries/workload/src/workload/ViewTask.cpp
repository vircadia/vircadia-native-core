//
//  ViewTask.cpp
//  libraries/workload/src/workload
//
//  Created by Sam Gateau 2018.03.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ViewTask.h"

using namespace workload;


void SetupViews::configure(const Config& config) {
    data = config.data;
}

void SetupViews::run(const WorkloadContextPointer& renderContext, const Input& inputs) {
    
    Views views = inputs;
    for (auto& v : views) {
        View::updateRegions(v, (float*) &data);
    }

    renderContext->_space->setViews(views);
}

