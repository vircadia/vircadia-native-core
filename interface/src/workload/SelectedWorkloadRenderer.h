//
//  GameWorkloadRender.h
//
//  Created by Sam Gateau on 2/20/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_SelectedWorkloadRenderer_h
#define hifi_SelectedWorkloadRenderer_h

#include "GameWorkload.h"

#include "GameWorkloadRenderer.h"

class SelectedWorkloadRenderer {
public:
    using Config = GameSpaceToRenderConfig;
    using Outputs = render::Transaction;
    using JobModel = workload::Job::ModelO<SelectedWorkloadRenderer, Outputs, Config>;

    SelectedWorkloadRenderer() {}

    void configure(const Config& config) {}
    void run(const workload::WorkloadContextPointer& renderContext, Outputs& outputs);

protected:
    render::ItemID _spaceRenderItemID{ render::Item::INVALID_ITEM_ID };
};

#endif
