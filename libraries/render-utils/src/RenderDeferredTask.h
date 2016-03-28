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

class DrawConfig : public render::Job::Config {
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

class DrawDeferred {
public:
    using Config = DrawConfig;
    using JobModel = render::Job::ModelI<DrawDeferred, render::ItemBounds, Config>;

    DrawDeferred(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemBounds& inItems);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn; // initialized by Config
};

class DrawStencilDeferred {
public:
    using JobModel = render::Job::Model<DrawStencilDeferred>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    static const gpu::PipelinePointer& getOpaquePipeline();

protected:
    static gpu::PipelinePointer _opaquePipeline; //lazy evaluation hence mutable
};

class DrawBackgroundDeferredConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(double gpuTime READ getGpuTime)
public:
    double getGpuTime() { return gpuTime; }

protected:
    friend class DrawBackgroundDeferred;
    double gpuTime;
};

class DrawBackgroundDeferred {
public:
    using Config = DrawBackgroundDeferredConfig;
    using JobModel = render::Job::ModelI<DrawBackgroundDeferred, render::ItemBounds, Config>;

    void configure(const Config& config) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemBounds& inItems);

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
    using Config = DrawOverlay3DConfig;
    using JobModel = render::Job::ModelI<DrawOverlay3D, render::ItemBounds, Config>;

    DrawOverlay3D(bool opaque);

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const render::ItemBounds& inItems);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn; // initialized by Config
    bool _opaquePass{ true };
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
};

#endif // hifi_RenderDeferredTask_h
