//
//  SortTask.h
//  render/src/render
//
//  Created by Zach Pomerantz on 2/26/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_SortTask_h
#define hifi_render_SortTask_h

#include "Engine.h"

namespace render {
    void depthSortItems(const RenderContextPointer& renderContext, bool frontToBack, const ItemBounds& inItems, ItemBounds& outItems, AABox* bounds = nullptr);

    class PipelineSortShapes {
    public:
        using JobModel = Job::ModelIO<PipelineSortShapes, ItemBounds, ShapeBounds>;
        void run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ShapeBounds& outShapes);
    };

    class DepthSortShapes {
    public:
        using JobModel = Job::ModelIO<DepthSortShapes, ShapeBounds, ShapeBounds>;

        bool _frontToBack;
        DepthSortShapes(bool frontToBack = true) : _frontToBack(frontToBack) {}

        void run(const RenderContextPointer& renderContext, const ShapeBounds& inShapes, ShapeBounds& outShapes);
    };

    class DepthSortShapesAndComputeBounds {
    public:
        using Outputs = VaryingSet2<ShapeBounds, AABox>;
        using JobModel = Job::ModelIO<DepthSortShapesAndComputeBounds, ShapeBounds, Outputs>;

        bool _frontToBack;
        DepthSortShapesAndComputeBounds(bool frontToBack = true) : _frontToBack(frontToBack) {}

        void run(const RenderContextPointer& renderContext, const ShapeBounds& inShapes, Outputs& outputs);
    };

    class DepthSortItems {
    public:
        using JobModel = Job::ModelIO<DepthSortItems, ItemBounds, ItemBounds>;

        bool _frontToBack;
        DepthSortItems(bool frontToBack = true) : _frontToBack(frontToBack) {}

        void run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems);
    };
}

#endif // hifi_render_SortTask_h;