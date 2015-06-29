//
//  Stats.h
//  render/src/render
//
//  Created by Niraj Venkat on 6/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Stats_h
#define hifi_render_Stats_h

#include "DrawTask.h"
#include "gpu/Batch.h"
#include <PerfStat.h>


namespace render {
    class DrawStats {
    public:
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems);

        typedef Job::ModelI<DrawStats, ItemIDsBounds> JobModel;
    };
}

#endif // hifi_render_Stats_h
