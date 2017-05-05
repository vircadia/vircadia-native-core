//
//  Engine.h
//  render/src/render
//
//  Created by Sam Gateau on 3/3/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Engine_h
#define hifi_render_Engine_h

#include <SettingHandle.h>

#include "Scene.h"
#include "../task/Task.h"
#include "gpu/Batch.h"

namespace render {



    class RenderContext : public task::JobContext {
    public:
        virtual ~RenderContext() {}

        RenderArgs* args;
        ScenePointer _scene;
    };
    using RenderContextPointer = std::shared_ptr<RenderContext>;

    Task_DeclareTypeAliases(RenderContext)

    // Versions of the COnfig integrating a gpu & batch timer
    class GPUJobConfig : public JobConfig {
        Q_OBJECT
            Q_PROPERTY(double gpuRunTime READ getGPURunTime)
            Q_PROPERTY(double batchRunTime READ getBatchRunTime)

            double _msGPURunTime { 0.0 };
        double _msBatchRunTime { 0.0 };
    public:
        using Persistent = PersistentConfig<GPUJobConfig>;

        GPUJobConfig() = default;
        GPUJobConfig(bool enabled) : JobConfig(enabled) {}

        // Running Time measurement on GPU and for Batch execution
        void setGPUBatchRunTime(double msGpuTime, double msBatchTime) { _msGPURunTime = msGpuTime; _msBatchRunTime = msBatchTime; }
        double getGPURunTime() const { return _msGPURunTime; }
        double getBatchRunTime() const { return _msBatchRunTime; }
    };

    class GPUTaskConfig : public TaskConfig {
        Q_OBJECT
            Q_PROPERTY(double gpuRunTime READ getGPURunTime)
            Q_PROPERTY(double batchRunTime READ getBatchRunTime)

            double _msGPURunTime { 0.0 };
        double _msBatchRunTime { 0.0 };
    public:

        using Persistent = PersistentConfig<GPUTaskConfig>;


        GPUTaskConfig() = default;
        GPUTaskConfig(bool enabled) : TaskConfig(enabled) {}

        // Running Time measurement on GPU and for Batch execution
        void setGPUBatchRunTime(double msGpuTime, double msBatchTime) { _msGPURunTime = msGpuTime; _msBatchRunTime = msBatchTime; }
        double getGPURunTime() const { return _msGPURunTime; }
        double getBatchRunTime() const { return _msBatchRunTime; }
    };


    // The render engine holds all render tasks, and is itself a render task.
    // State flows through tasks to jobs via the render and scene contexts -
    // the engine should not be known from its jobs.
    class Engine : public Task {
    public:

        Engine();
        ~Engine() = default;

        // Load any persisted settings, and set up the presets
        // This should be run after adding all jobs, and before building ui
        void load();

        // Register the scene
        void registerScene(const ScenePointer& scene) { _renderContext->_scene = scene; }

        // acces the RenderContext
        RenderContextPointer getRenderContext() const { return _renderContext; }

        // Render a frame
        // Must have a scene registered and a context set
        void run() { assert(_renderContext);  Task::run(_renderContext); }

    protected:
        RenderContextPointer _renderContext;
    
        void run(const RenderContextPointer& context) override { assert(_renderContext);  Task::run(_renderContext); }
    };
    using EnginePointer = std::shared_ptr<Engine>;

}

#endif // hifi_render_Engine_h
