//
//  DrawStatus.h
//  render/src/render
//
//  Created by Niraj Venkat on 6/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_DrawStatus_h
#define hifi_render_DrawStatus_h

#include "DrawTask.h"
#include "gpu/Batch.h"

namespace render {
    class DrawStatus {
        gpu::PipelinePointer _drawItemBoundsPipeline;
        gpu::PipelinePointer _drawItemStatusPipeline;

    public:

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems);

        typedef Job::ModelI<DrawStatus, ItemIDsBounds> JobModel;

        const gpu::PipelinePointer& getDrawItemBoundsPipeline();
        const gpu::PipelinePointer& getDrawItemStatusPipeline();
    };
}

#endif // hifi_render_DrawStatus_h
