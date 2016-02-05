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
#include <ViewFrustum.h>

namespace render {
    class DrawSceneOctreeConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(bool showVisibleCells MEMBER showVisibleCells WRITE setShowVisibleCells)
        Q_PROPERTY(bool freezeFrustum MEMBER freezeFrustum WRITE setFreezeFrustum)
    public:

        DrawSceneOctreeConfig() : Job::Config(true) {} // FIXME FOR debug
        
        bool showVisibleCells{ true }; // FIXME FOR debug
        bool freezeFrustum{ false };

    public slots:
        void setShowVisibleCells(bool enabled) { showVisibleCells = enabled; emit dirty(); }
        void setFreezeFrustum(bool enabled) { freezeFrustum = enabled; emit dirty(); }

    signals:
        void dirty();
    };

    class DrawSceneOctree {

        int _drawCellLocationLoc;
        gpu::PipelinePointer _drawCellBoundsPipeline;
        gpu::BufferPointer _cells;

        gpu::PipelinePointer _drawLODReticlePipeline;


        bool _showVisibleCells; // initialized by Config
        bool _freezeFrustum{ false }; // initialized by Config
        bool _justFrozeFrustum{ false };
        ViewFrustum _frozenFrutstum;

    public:
        using Config = DrawSceneOctreeConfig;
        using JobModel = Job::ModelI<DrawSceneOctree, ItemSpatialTree::ItemSelection, Config>;

        DrawSceneOctree() {}

        void configure(const Config& config);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemSpatialTree::ItemSelection& selection);

        const gpu::PipelinePointer getDrawCellBoundsPipeline();
        const gpu::PipelinePointer getDrawLODReticlePipeline();
    };
}

#endif // hifi_render_DrawStatus_h
