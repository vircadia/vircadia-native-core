//
//  RenderDeferredTask.h
//  render-utils/src/
//
//  Created by Sam Gateau on 5/29/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderDeferredTask_h
#define hifi_RenderDeferredTask_h

#include "render/DrawTask.h"

#include "DeferredLightingEffect.h"

class PrepareDeferred {
public:
};
namespace render {
template <> void jobRun(const PrepareDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);
}

class ResolveDeferred {
public:
};
namespace render {
template <> void jobRun(const ResolveDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);
}

class RenderDeferredTask : public render::Task {
public:

    RenderDeferredTask();
    ~RenderDeferredTask();

    render::Jobs _jobs;

    virtual void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

};


#endif // hifi_RenderDeferredTask_h
