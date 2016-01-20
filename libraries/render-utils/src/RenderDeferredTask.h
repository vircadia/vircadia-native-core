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

#include "gpu/Pipeline.h"

#include "render/DrawTask.h"

#include "ToneMappingEffect.h"

class SetupDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Task::Job::Model<SetupDeferred>;
};

class PrepareDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Task::Job::Model<PrepareDeferred>;
};


class RenderDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Task::Job::Model<RenderDeferred>;
};

class ToneMappingDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    ToneMappingEffect _toneMappingEffect;

    using JobModel = render::Task::Job::Model<ToneMappingDeferred>;
};

class DrawOpaqueDeferred {
public:
    DrawOpaqueDeferred(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemIDsBounds& inItems);

    using JobModel = render::Task::Job::ModelI<DrawOpaqueDeferred, render::ItemIDsBounds>;

protected:
    render::ShapePlumberPointer _shapePlumber;
};

class DrawTransparentDeferred {
public:
    DrawTransparentDeferred(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemIDsBounds& inItems);

    using JobModel = render::Task::Job::ModelI<DrawTransparentDeferred, render::ItemIDsBounds>;

protected:
    render::ShapePlumberPointer _shapePlumber;
};

class DrawStencilDeferred {
public:
    static const gpu::PipelinePointer& getOpaquePipeline();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Task::Job::Model<DrawStencilDeferred>;

protected:
    static gpu::PipelinePointer _opaquePipeline; //lazy evaluation hence mutable
};

class DrawBackgroundDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Task::Job::Model<DrawBackgroundDeferred>;
};

class DrawOverlay3D {
public:
    DrawOverlay3D(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}
    static const gpu::PipelinePointer& getOpaquePipeline();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Task::Job::Model<DrawOverlay3D>;

protected:
    static gpu::PipelinePointer _opaquePipeline; //lazy evaluation hence mutable
    render::ShapePlumberPointer _shapePlumber;
};

class Blit {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Task::Job::Model<Blit>;
};

class RenderDeferredTask : public render::Task {
public:
    RenderDeferredTask(render::CullFunctor cullFunctor);

    void setDrawDebugDeferredBuffer(int draw) { enableJob(_drawDebugDeferredBufferIndex, draw >= 0); }
    bool doDrawDebugDeferredBuffer() const { return getEnableJob(_drawDebugDeferredBufferIndex); }
    
    void setDrawItemStatus(int draw) { enableJob(_drawStatusJobIndex, draw > 0); }
    bool doDrawItemStatus() const { return getEnableJob(_drawStatusJobIndex); }
    
    void setDrawHitEffect(bool draw) { enableJob(_drawHitEffectJobIndex, draw); }
    bool doDrawHitEffect() const { return getEnableJob(_drawHitEffectJobIndex); }

    void setOcclusionStatus(bool draw) { enableJob(_occlusionJobIndex, draw); }
    bool doOcclusionStatus() const { return getEnableJob(_occlusionJobIndex); }

    void setAntialiasingStatus(bool draw) { enableJob(_antialiasingJobIndex, draw); }
    bool doAntialiasingStatus() const { return getEnableJob(_antialiasingJobIndex); }

    void setToneMappingExposure(float exposure);
    float getToneMappingExposure() const;

    void setToneMappingToneCurve(int toneCurve);
    int getToneMappingToneCurve() const;

    virtual void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

protected:
    int _drawDebugDeferredBufferIndex;
    int _drawStatusJobIndex;
    int _drawHitEffectJobIndex;
    int _occlusionJobIndex;
    int _antialiasingJobIndex;
    int _toneMappingJobIndex;
};

#endif // hifi_RenderDeferredTask_h
