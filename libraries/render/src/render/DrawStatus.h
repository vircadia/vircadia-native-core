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
        Q_PROPERTY(bool showDisplay MEMBER showDisplay WRITE setShowDisplay)
        Q_PROPERTY(bool showNetwork MEMBER showNetwork WRITE setShowNetwork)
    public:
        DrawStatusConfig() : Job::Config(false) {} // FIXME FOR debug

        void dirtyHelper();

        bool showDisplay{ false };
        bool showNetwork{ false };

    public slots:
        void setShowDisplay(bool enabled) { showDisplay = enabled; dirtyHelper(); }
        void setShowNetwork(bool enabled) { showNetwork = enabled; dirtyHelper(); }

    signals:
        void dirty();
    };

    class DrawStatus {
    public:
        using Config = DrawStatusConfig;
        using JobModel = Job::ModelI<DrawStatus, ItemBounds, Config>;

        DrawStatus() {}
        DrawStatus(const gpu::TexturePointer statusIconMap) { setStatusIconMap(statusIconMap); }

        void configure(const Config& config);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems);

        const gpu::PipelinePointer getDrawItemBoundsPipeline();
        const gpu::PipelinePointer getDrawItemStatusPipeline();

        void setStatusIconMap(const gpu::TexturePointer& map);
        const gpu::TexturePointer getStatusIconMap() const;

    protected:
        bool _showDisplay; // initialized by Config
        bool _showNetwork; // initialized by Config

        int _drawItemBoundPosLoc = -1;
        int _drawItemBoundDimLoc = -1;
        int _drawItemCellLocLoc = -1;
        int _drawItemStatusPosLoc = -1;
        int _drawItemStatusDimLoc = -1;
        int _drawItemStatusValue0Loc = -1;
        int _drawItemStatusValue1Loc = -1;

        gpu::Stream::FormatPointer _drawItemFormat;
        gpu::PipelinePointer _drawItemBoundsPipeline;
        gpu::PipelinePointer _drawItemStatusPipeline;

        std::vector<AABox> _itemBounds;
        std::vector<std::pair<glm::ivec4, glm::ivec4>> _itemStatus;
        std::vector<Octree::Location> _itemCells;
        //gpu::BufferPointer _itemBounds;
        //gpu::BufferPointer _itemCells;
        //gpu::BufferPointer _itemStatus;
        gpu::TexturePointer _statusIconMap;
    };
}

#endif // hifi_render_DrawStatus_h
