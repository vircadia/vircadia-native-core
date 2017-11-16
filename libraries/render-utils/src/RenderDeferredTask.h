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
#include <render/RenderFetchCullSortTask.h>
#include "LightingModel.h"


class BeginGPURangeTimer {
public:
    using JobModel = render::Job::ModelO<BeginGPURangeTimer, gpu::RangeTimerPointer>;

    BeginGPURangeTimer(const std::string& name) : _gpuTimer(std::make_shared<gpu::RangeTimer>(name)) {}

    void run(const render::RenderContextPointer& renderContext, gpu::RangeTimerPointer& timer);

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
    void run(const render::RenderContextPointer& renderContext, const gpu::RangeTimerPointer& timer);

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
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

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
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn; // initialized by Config
    bool _stateSort;
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
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn; // initialized by Config
    bool _opaquePass { true };
};

class CompositeHUD {
public:
    using JobModel = render::Job::Model<CompositeHUD>;

    CompositeHUD() {}
    void run(const render::RenderContextPointer& renderContext);
};

class Blit {
public:
    using JobModel = render::Job::ModelI<Blit, gpu::FramebufferPointer>;

    void run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& srcFramebuffer);
};

class ExtractFrustums {
public:

    enum Frustum {
        SHADOW_CASCADE0_FRUSTUM = 0,
        SHADOW_CASCADE1_FRUSTUM,
        SHADOW_CASCADE2_FRUSTUM,
        SHADOW_CASCADE3_FRUSTUM,

        SHADOW_CASCADE_FRUSTUM_COUNT,

        VIEW_FRUSTUM = SHADOW_CASCADE_FRUSTUM_COUNT,

        FRUSTUM_COUNT
    };

    using Output = render::VaryingArray<ViewFrustumPointer, FRUSTUM_COUNT>;
    using JobModel = render::Job::ModelO<ExtractFrustums, Output>;

    void run(const render::RenderContextPointer& renderContext, Output& output);
};

class RenderDeferredTaskConfig : public render::Task::Config {
    Q_OBJECT
        Q_PROPERTY(float fadeScale MEMBER fadeScale NOTIFY dirty)
        Q_PROPERTY(float fadeDuration MEMBER fadeDuration NOTIFY dirty)
        Q_PROPERTY(bool debugFade MEMBER debugFade NOTIFY dirty)
        Q_PROPERTY(float debugFadePercent MEMBER debugFadePercent NOTIFY dirty)
public:
    float fadeScale{ 0.5f };
    float fadeDuration{ 3.0f };
    float debugFadePercent{ 0.f };
    bool debugFade{ false };

signals:
    void dirty();

};

class RenderDeferredTask {
public:
    using Input = RenderFetchCullSortTask::Output;
    using Config = RenderDeferredTaskConfig;
    using JobModel = render::Task::ModelI<RenderDeferredTask, Input, Config>;

    RenderDeferredTask();

    void configure(const Config& config);
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

private:

    static const render::Varying addSelectItemJobs(JobModel& task, const char* selectionName, 
                                                   const render::Varying& metas, const render::Varying& opaques, const render::Varying& transparents);
};

#endif // hifi_RenderDeferredTask_h
