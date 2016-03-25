//
//  EngineStats.h
//  render/src/render
//
//  Created by Sam Gateau on 3/27/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_EngineStats_h
#define hifi_render_EngineStats_h

#include <gpu/Context.h>

#include <QElapsedTimer>

#include "Engine.h"

namespace render {

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

        Q_PROPERTY(int frameDrawcallCount MEMBER frameDrawcallCount NOTIFY dirty)
        Q_PROPERTY(int frameDrawcallRate MEMBER frameDrawcallRate NOTIFY dirty)

        Q_PROPERTY(int frameTriangleCount MEMBER frameTriangleCount NOTIFY dirty)
        Q_PROPERTY(int frameTriangleRate MEMBER frameTriangleRate NOTIFY dirty)

        Q_PROPERTY(int frameTextureCount MEMBER frameTextureCount NOTIFY dirty)
        Q_PROPERTY(int frameTextureRate MEMBER frameTextureRate NOTIFY dirty)


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

        int frameDrawcallCount{ 0 };
        int frameDrawcallRate{ 0 };

        int frameTriangleCount{ 0 };
        int frameTriangleRate{ 0 };

        int frameTextureCount{ 0 };
        int frameTextureRate{ 0 };

        void emitDirty() { emit dirty(); }

    signals:
        void dirty();
    };

    class EngineStats {
    public:
        using Config = EngineStatsConfig;
        using JobModel = Job::Model<EngineStats, Config>;

        EngineStats() { _frameTimer.start(); }

        gpu::ContextStats _gpuStats;

        void configure(const Config& configuration) {}
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);

        QElapsedTimer _frameTimer;
    };
}

#endif