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

#include <gpu/Pipeline.h>
#include <render/CullTask.h>
#include "LightingModel.h"


class BeginGPURangeTimer {
public:
    using JobModel = render::Job::ModelO<BeginGPURangeTimer, gpu::RangeTimerPointer>;
    
    BeginGPURangeTimer() : _gpuTimer(std::make_shared<gpu::RangeTimer>()) {}
    
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, gpu::RangeTimerPointer& timer);
    
protected:
    gpu::RangeTimerPointer _gpuTimer;
};

using GPURangeTimerConfig = render::GPUJobConfig;

class EndGPURangeTimer {
public:
    using Config = GPURangeTimerConfig;
    using JobModel = render::Job::ModelI<EndGPURangeTimer, gpu::RangeTimerPointer, Config>;
    
    EndGPURangeTimer() {}
    
    void configure(const Config& config) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const gpu::RangeTimerPointer& timer);
    
protected:
};


class DrawConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY newStats)

    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
public:

    int getNumDrawn() { return _numDrawn; }
    void setNumDrawn(int numDrawn) { _numDrawn = numDrawn;  emit newStats(); }

    int maxDrawn{ -1 };

signals:
    void newStats();
    void dirty();

protected:
    int _numDrawn{ 0 };
};

class DrawDeferred {
public:
    using Inputs = render::VaryingSet2 <render::ItemBounds, LightingModelPointer>;
    using Config = DrawConfig;
    using JobModel = render::Job::ModelI<DrawDeferred, Inputs, Config>;

    DrawDeferred(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn; // initialized by Config
};

class DrawStateSortConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY numDrawnChanged)
        Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
        Q_PROPERTY(bool stateSort MEMBER stateSort NOTIFY dirty)
public:

    int getNumDrawn() { return numDrawn; }
    void setNumDrawn(int num) { numDrawn = num; emit numDrawnChanged(); }

    int maxDrawn{ -1 };
    bool stateSort{ true };

signals:
    void numDrawnChanged();
    void dirty();

protected:
    int numDrawn{ 0 };
};

class DrawStateSortDeferred {
public:
    using Inputs = render::VaryingSet2 <render::ItemBounds, LightingModelPointer>;

    using Config = DrawStateSortConfig;
    using JobModel = render::Job::ModelI<DrawStateSortDeferred, Inputs, Config>;

    DrawStateSortDeferred(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; _stateSort = config.stateSort; }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn; // initialized by Config
    bool _stateSort;
};

class DeferredFramebuffer;
class DrawStencilDeferred {
public:
    using JobModel = render::Job::ModelI<DrawStencilDeferred, std::shared_ptr<DeferredFramebuffer>>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const std::shared_ptr<DeferredFramebuffer>& deferredFramebuffer);

protected:
    gpu::PipelinePointer _opaquePipeline;

    gpu::PipelinePointer getOpaquePipeline();
};

using DrawBackgroundDeferredConfig = render::GPUJobConfig;

class DrawBackgroundDeferred {
public:
    using Inputs = render::VaryingSet2 <render::ItemBounds, LightingModelPointer>;

    using Config = DrawBackgroundDeferredConfig;
    using JobModel = render::Job::ModelI<DrawBackgroundDeferred, Inputs, Config>;

    void configure(const Config& config) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    gpu::RangeTimer _gpuTimer;
};

class DrawOverlay3DConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY numDrawnChanged)
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
public:
    int getNumDrawn() { return numDrawn; }
    void setNumDrawn(int num) { numDrawn = num; emit numDrawnChanged(); }

    int maxDrawn{ -1 };

signals:
    void numDrawnChanged();
    void dirty();

protected:
    int numDrawn{ 0 };
};

class DrawOverlay3D {
public:
    using Inputs = render::VaryingSet2 <render::ItemBounds, LightingModelPointer>;

    using Config = DrawOverlay3DConfig;
    using JobModel = render::Job::ModelI<DrawOverlay3D, Inputs, Config>;

    DrawOverlay3D(bool opaque);

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn; // initialized by Config
    bool _opaquePass{ true };
};

class Blit {
public:
    using JobModel = render::Job::ModelI<Blit, gpu::FramebufferPointer>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& srcFramebuffer);
};

using RenderDeferredTaskConfig = render::GPUTaskConfig;
/**
class RenderDeferredTaskConfig : public render::Task::Config {
    Q_OBJECT
    Q_PROPERTY(double gpuTime READ getGpuTime)
public:
    double getGpuTime() { return gpuTime; }

protected:
    friend class RenderDeferredTask;
    double gpuTime;
};*/

class RenderDeferredTask : public render::Task {
public:
    using Config = RenderDeferredTaskConfig;
    RenderDeferredTask(render::CullFunctor cullFunctor);

    void configure(const Config& config) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = Model<RenderDeferredTask, Config>;

protected:
    gpu::RangeTimer _gpuTimer;
};

#endif // hifi_RenderDeferredTask_h
