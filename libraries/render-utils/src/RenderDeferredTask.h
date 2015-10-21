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

#include "gpu/Pipeline.h"

class SetupDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    typedef render::Job::Model<SetupDeferred> JobModel;
};

class PrepareDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    typedef render::Job::Model<PrepareDeferred> JobModel;
};


class RenderDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    typedef render::Job::Model<RenderDeferred> JobModel;
};

class ResolveDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    typedef render::Job::Model<ResolveDeferred> JobModel;
};

class DrawOpaqueDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemIDsBounds& inItems);

    typedef render::Job::ModelI<DrawOpaqueDeferred, render::ItemIDsBounds> JobModel;
};

class DrawTransparentDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemIDsBounds& inItems);

    typedef render::Job::ModelI<DrawTransparentDeferred, render::ItemIDsBounds> JobModel;
};

class DrawStencilDeferred {
    static gpu::PipelinePointer _opaquePipeline; //lazy evaluation hence mutable
public:
    static const gpu::PipelinePointer& getOpaquePipeline();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    typedef render::Job::Model<DrawStencilDeferred> JobModel;
};

class DrawBackgroundDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    typedef render::Job::Model<DrawBackgroundDeferred> JobModel;
};

class DrawOverlay3D {
    static gpu::PipelinePointer _opaquePipeline; //lazy evaluation hence mutable
public:
    static const gpu::PipelinePointer& getOpaquePipeline();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    typedef render::Job::Model<DrawOverlay3D> JobModel;
};

class RenderDeferredTask : public render::Task {
public:

    RenderDeferredTask();
    ~RenderDeferredTask();

    render::Jobs _jobs;

    int _drawStatusJobIndex = -1;
    int _drawHitEffectJobIndex = -1;

    void setDrawItemStatus(bool draw) { if (_drawStatusJobIndex >= 0) { _jobs[_drawStatusJobIndex].setEnabled(draw); } }
    bool doDrawItemStatus() const { if (_drawStatusJobIndex >= 0) { return _jobs[_drawStatusJobIndex].isEnabled(); } else { return false; } }
    
    void setDrawHitEffect(bool draw) { if (_drawHitEffectJobIndex >= 0) { _jobs[_drawHitEffectJobIndex].setEnabled(draw); } }
    bool doDrawHitEffect() const { if (_drawHitEffectJobIndex >=0) { return _jobs[_drawHitEffectJobIndex].isEnabled(); } else { return false; } }

    int _occlusionJobIndex = -1;

    void setOcclusionStatus(bool draw) { if (_occlusionJobIndex >= 0) { _jobs[_occlusionJobIndex].setEnabled(draw); } }
    bool doOcclusionStatus() const { if (_occlusionJobIndex >= 0) { return _jobs[_occlusionJobIndex].isEnabled(); } else { return false; } }

    int _antialiasingJobIndex = -1;

    void setAntialiasingStatus(bool draw) { if (_antialiasingJobIndex >= 0) { _jobs[_antialiasingJobIndex].setEnabled(draw); } }
    bool doAntialiasingStatus() const { if (_antialiasingJobIndex >= 0) { return _jobs[_antialiasingJobIndex].isEnabled(); } else { return false; } }

    virtual void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);


    gpu::Queries _timerQueries;
    int _currentTimerQueryIndex = 0;
};


#endif // hifi_RenderDeferredTask_h
