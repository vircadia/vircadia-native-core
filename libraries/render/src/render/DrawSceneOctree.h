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
        Q_PROPERTY(bool enabled MEMBER enabled NOTIFY dirty())
        Q_PROPERTY(bool showVisibleCells MEMBER showVisibleCells WRITE setShowVisibleCells)
        Q_PROPERTY(bool showEmptyCells MEMBER showEmptyCells WRITE setShowEmptyCells)
        Q_PROPERTY(int numAllocatedCells READ getNumAllocatedCells)
        Q_PROPERTY(int numFreeCells READ getNumFreeCells)

    public:

        DrawSceneOctreeConfig() : Job::Config(false) {}
        
        bool showVisibleCells{ true };
        bool showEmptyCells{ false };

        int numAllocatedCells{ 0 };
        int numFreeCells{ 0 };

        int getNumAllocatedCells() const { return numAllocatedCells; }
        int getNumFreeCells() const { return numFreeCells; }

    public slots:
        void setShowVisibleCells(bool show) { showVisibleCells = show; emit dirty(); }
        void setShowEmptyCells(bool show) { showEmptyCells = show; emit dirty(); }

    signals:
        void dirty();
    };

    class DrawSceneOctree {

        int _drawCellLocationLoc;
        gpu::PipelinePointer _drawCellBoundsPipeline;
        gpu::BufferPointer _cells;

        gpu::PipelinePointer _drawLODReticlePipeline;

        int _drawItemBoundPosLoc = -1;
        int _drawItemBoundDimLoc = -1;
        gpu::PipelinePointer _drawItemBoundPipeline;

        bool _showVisibleCells; // initialized by Config
        bool _showEmptyCells; // initialized by Config

    public:
        using Config = DrawSceneOctreeConfig;
        using JobModel = Job::ModelI<DrawSceneOctree, ItemSpatialTree::ItemSelection, Config>;

        DrawSceneOctree() {}

        void configure(const Config& config);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemSpatialTree::ItemSelection& selection);

        const gpu::PipelinePointer getDrawCellBoundsPipeline();
        const gpu::PipelinePointer getDrawLODReticlePipeline();
        const gpu::PipelinePointer getDrawItemBoundPipeline();
    };


    class DrawItemSelectionConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(bool enabled MEMBER enabled NOTIFY dirty())
        Q_PROPERTY(bool showInsideItems MEMBER showInsideItems WRITE setShowInsideItems)
        Q_PROPERTY(bool showInsideSubcellItems MEMBER showInsideSubcellItems WRITE setShowInsideSubcellItems)
        Q_PROPERTY(bool showPartialItems MEMBER showPartialItems WRITE setShowPartialItems)
        Q_PROPERTY(bool showPartialSubcellItems MEMBER showPartialSubcellItems WRITE setShowPartialSubcellItems)
    public:

        DrawItemSelectionConfig() : Job::Config(false) {}

        bool showInsideItems{ true };
        bool showInsideSubcellItems{ true };
        bool showPartialItems{ true };
        bool showPartialSubcellItems{ true };

        public slots:
        void setShowInsideItems(bool show) { showInsideItems = show; emit dirty(); }
        void setShowInsideSubcellItems(bool show) { showInsideSubcellItems = show; emit dirty(); }
        void setShowPartialItems(bool show) { showPartialItems = show; emit dirty(); }
        void setShowPartialSubcellItems(bool show) { showPartialSubcellItems = show; emit dirty(); }

    signals:
        void dirty();
    };

    class DrawItemSelection {

        int _drawItemBoundPosLoc = -1;
        int _drawItemBoundDimLoc = -1;
        int _drawCellLocationLoc = -1;
        gpu::PipelinePointer _drawItemBoundPipeline;

        bool _showInsideItems; // initialized by Config
        bool _showInsideSubcellItems; // initialized by Config
        bool _showPartialItems; // initialized by Config
        bool _showPartialSubcellItems; // initialized by Config

    public:
        using Config = DrawItemSelectionConfig;
        using JobModel = Job::ModelI<DrawItemSelection, ItemSpatialTree::ItemSelection, Config>;

        DrawItemSelection() {}

        void configure(const Config& config);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemSpatialTree::ItemSelection& selection);

        const gpu::PipelinePointer getDrawItemBoundPipeline();
    };
}

#endif // hifi_render_DrawStatus_h
