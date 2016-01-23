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
    class DrawStatusConfig : public Job::Config {
        Q_OBJECT
    public:
        Q_PROPERTY(bool showDisplay MEMBER showDisplay NOTIFY dirty)
        Q_PROPERTY(bool showNetwork MEMBER showDisplay NOTIFY dirty)
        bool showDisplay{ false };
        bool showNetwork{ false };
    signals:
        void dirty();
    };

    class DrawStatus {
    public:
        using Config = DrawStatusConfig;
        using JobModel = Job::ModelI<DrawStatus, ItemIDsBounds, Config>;

        DrawStatus() {}
        DrawStatus(const gpu::TexturePointer statusIconMap) { setStatusIconMap(statusIconMap); }

        void configure(const Config& configuration);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems);

        const gpu::PipelinePointer getDrawItemBoundsPipeline();
        const gpu::PipelinePointer getDrawItemStatusPipeline();

        void setStatusIconMap(const gpu::TexturePointer& map);
        const gpu::TexturePointer getStatusIconMap() const;

    protected:
        bool _showDisplay{ false };
        bool _showNetwork{ false };

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
    };
}

#endif // hifi_render_DrawStatus_h
