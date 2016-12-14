//
//  RenderForwardTask.h
//  render-utils/src/
//
//  Created by Zach Pomerantz on 12/13/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderForwardTask_h
#define hifi_RenderForwardTask_h

#include <gpu/Pipeline.h>
#include <render/CullTask.h>
#include "LightingModel.h"

using RenderForwardTaskConfig = render::GPUTaskConfig;

class RenderForwardTask : public render::Task {
public:
    using Config = RenderForwardTaskConfig;
    RenderForwardTask(render::CullFunctor cullFunctor);

    void configure(const Config& config) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = Model<RenderForwardTask, Config>;

protected:
    gpu::RangeTimerPointer _gpuTimer;
};

#endif // hifi_RenderForwardTask_h
