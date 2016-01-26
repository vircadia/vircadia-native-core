//
//  DrawSceneOctree.h
//  render/src/render
//
//  Created by Sam Gateau on 1/25/16.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_DrawSceneOctree_h
#define hifi_render_DrawSceneOctree_h

#include "DrawTask.h"
#include "gpu/Batch.h"

namespace render {
    class DrawSceneOctree {

        int _drawBoundPosLoc;
        int _drawBoundDimLoc;
        gpu::PipelinePointer _drawCellBoundsPipeline;
        gpu::BufferPointer _cells;

    public:

        DrawSceneOctree() {}

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);

        using JobModel = Task::Job::Model<DrawSceneOctree>;

        const gpu::PipelinePointer getDrawCellBoundsPipeline();
    };
}

#endif // hifi_render_DrawStatus_h
