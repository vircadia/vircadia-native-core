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
        int _drawItemBoundPosLoc = -1;
        int _drawItemBoundDimLoc = -1;
        int _drawItemStatusPosLoc = -1;
        int _drawItemStatusDimLoc = -1;
        int _drawItemStatusValue0Loc = -1;
        int _drawItemStatusValue1Loc = -1;

        gpu::Stream::FormatPointer _drawItemFormat;
        gpu::PipelinePointer _drawItemBoundsPipeline;
        gpu::PipelinePointer _drawItemStatusPipeline;
        gpu::BufferPointer _itemBounds;
        gpu::BufferPointer _itemStatus;
        gpu::TexturePointer _statusIconMap;

    public:

        DrawStatus() {}
        DrawStatus(const gpu::TexturePointer statusIconMap) { setStatusIconMap(statusIconMap); }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems);

        typedef Job::ModelI<DrawStatus, ItemIDsBounds> JobModel;

        const gpu::PipelinePointer getDrawItemBoundsPipeline();
        const gpu::PipelinePointer getDrawItemStatusPipeline();

        void setStatusIconMap(const gpu::TexturePointer& map);
        const gpu::TexturePointer getStatusIconMap() const;
    };
}

#endif // hifi_render_DrawStatus_h
