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
#include <gpu/Context.h>

#include "Context.h"
#include "Task.h"
namespace render {

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
        void registerScene(const ScenePointer& scene) { _sceneContext->_scene = scene; }

        // Push a RenderContext
        void setRenderContext(const RenderContext& renderContext) { (*_renderContext) = renderContext; }
        RenderContextPointer getRenderContext() const { return _renderContext; }

        // Render a frame
        // A frame must have a scene registered and a context set to render
        void run();

    protected:
        SceneContextPointer _sceneContext;
        RenderContextPointer _renderContext;
    };
    using EnginePointer = std::shared_ptr<Engine>;


    // A simple job collecting global stats on the Engine / Scene / GPU
    class EngineStatsConfig : public Job::Config{
        Q_OBJECT

        Q_PROPERTY(int numBuffers MEMBER numBuffers NOTIFY dirty)
        Q_PROPERTY(int numGPUBuffers MEMBER numGPUBuffers NOTIFY dirty)
        Q_PROPERTY(qint64 bufferSysmemUsage MEMBER bufferSysmemUsage NOTIFY dirty)
        Q_PROPERTY(qint64 bufferVidmemUsage MEMBER bufferVidmemUsage NOTIFY dirty)

        Q_PROPERTY(int numTextures MEMBER numTextures NOTIFY dirty)
        Q_PROPERTY(int numGPUTextures MEMBER numGPUTextures NOTIFY dirty)
        Q_PROPERTY(qint64 textureSysmemUsage MEMBER textureSysmemUsage NOTIFY dirty)
        Q_PROPERTY(qint64 textureVidmemUsage MEMBER textureVidmemUsage NOTIFY dirty)
        Q_PROPERTY(int numFrameTextures MEMBER numFrameTextures NOTIFY dirty)
    public:
        EngineStatsConfig() : Job::Config(true) {}

        int numBuffers{ 0 };
        int numGPUBuffers{ 0 };
        qint64 bufferSysmemUsage{ 0 };
        qint64 bufferVidmemUsage{ 0 };

        int numTextures{ 0 };
        int numGPUTextures{ 0 };
        qint64 textureSysmemUsage{ 0 };
        qint64 textureVidmemUsage{ 0 };

        int numFrameTriangles{ 0 };
        int numFrameTextures{ 0 };

        void emitDirty() { emit dirty(); }

    signals:
        void dirty();
    };

    class EngineStats {
    public:
        using Config = EngineStatsConfig;
        using JobModel = Job::Model<EngineStats, Config>;

        EngineStats() {}

        gpu::ContextStats _gpuStats;

        void configure(const Config& configuration) {}
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);
    };


}

#endif // hifi_render_Engine_h
