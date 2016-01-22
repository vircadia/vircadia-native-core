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

    using JobModel = render::Job::Model<SetupDeferred>;
};

class PrepareDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Job::Model<PrepareDeferred>;
};


class RenderDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Job::Model<RenderDeferred>;
};


class ToneMappingConfig : public render::Job::Config {
    Q_OBJECT
public:
    ToneMappingConfig() : render::Job::Config(true) {}
    Q_PROPERTY(float exposure MEMBER exposure NOTIFY dirty);
    Q_PROPERTY(int curve MEMBER curve NOTIFY dirty);
    float exposure{ 0.0 };
    int curve{ 3 };
signals:
    void dirty();
};

class ToneMappingDeferred {
public:
    using Config = ToneMappingConfig;
    using JobModel = render::Job::Model<ToneMappingDeferred, Config>;

    void configure(const Config&);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    ToneMappingEffect _toneMappingEffect;
};

class DrawConfig : public render::Job::Config {
    Q_OBJECT
public:
    Q_PROPERTY(int numDrawn READ getNumDrawn)
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
    int getNumDrawn() { return numDrawn; }

    int numDrawn{ 0 };
    int maxDrawn{ -1 };
signals:
    void dirty();
};

class DrawDeferred {
public:
    using Config = DrawConfig;
    using JobModel = render::Job::ModelI<DrawDeferred, render::ItemIDsBounds, Config>;

    DrawDeferred(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemIDsBounds& inItems);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn{ -1 };
};

class DrawStencilDeferred {
public:
    static const gpu::PipelinePointer& getOpaquePipeline();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Job::Model<DrawStencilDeferred>;

protected:
    static gpu::PipelinePointer _opaquePipeline; //lazy evaluation hence mutable
};

class DrawBackgroundDeferred {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Job::Model<DrawBackgroundDeferred>;
};

class DrawOverlay3DConfig : public render::Job::Config {
    Q_OBJECT
public:
    Q_PROPERTY(int numItems READ getNumItems)
    Q_PROPERTY(int numDrawn READ getNumDrawn)
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
    int getNumItems() { return numItems; }
    int getNumDrawn() { return numDrawn; }

    int numItems{ 0 };
    int numDrawn{ 0 };
    int maxDrawn{ -1 };
signals:
    void dirty();
};

class DrawOverlay3D {
public:
    using Config = DrawOverlay3DConfig;
    using JobModel = render::Job::Model<DrawOverlay3D, Config>;

    DrawOverlay3D(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    static const gpu::PipelinePointer& getOpaquePipeline();

protected:
    static gpu::PipelinePointer _opaquePipeline; //lazy evaluation hence mutable
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn{ -1 };
};

class Blit {
public:
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = render::Job::Model<Blit>;
};

class RenderDeferredTask : public render::Task {
public:
    RenderDeferredTask(render::CullFunctor cullFunctor);

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = Model<RenderDeferredTask>;

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

    void setToneMappingToneCurve(int toneCurve);


protected:
    int _drawDebugDeferredBufferIndex;
    int _drawStatusJobIndex;
    int _drawHitEffectJobIndex;
    int _occlusionJobIndex;
    int _antialiasingJobIndex;
    int _toneMappingJobIndex;
};

#endif // hifi_RenderDeferredTask_h
