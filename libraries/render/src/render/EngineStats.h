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

        Q_PROPERTY(quint32 bufferCPUCount MEMBER bufferCPUCount NOTIFY dirty)
        Q_PROPERTY(quint32 bufferGPUCount MEMBER bufferGPUCount NOTIFY dirty)
        Q_PROPERTY(qint64 bufferCPUMemoryUsage MEMBER bufferCPUMemoryUsage NOTIFY dirty)
        Q_PROPERTY(qint64 bufferGPUMemoryUsage MEMBER bufferGPUMemoryUsage NOTIFY dirty)

        Q_PROPERTY(quint32 textureCPUCount MEMBER textureCPUCount NOTIFY dirty)
        Q_PROPERTY(quint32 textureGPUCount MEMBER textureGPUCount NOTIFY dirty)
        Q_PROPERTY(qint64 textureCPUMemoryUsage MEMBER textureCPUMemoryUsage NOTIFY dirty)
        Q_PROPERTY(qint64 textureGPUMemoryUsage MEMBER textureGPUMemoryUsage NOTIFY dirty)
        Q_PROPERTY(qint64 textureGPUVirtualMemoryUsage MEMBER textureGPUVirtualMemoryUsage NOTIFY dirty)
        Q_PROPERTY(quint32 textureGPUTransferCount MEMBER textureGPUTransferCount NOTIFY dirty)

        Q_PROPERTY(quint32 frameAPIDrawcallCount MEMBER frameAPIDrawcallCount NOTIFY dirty)
        Q_PROPERTY(quint32 frameDrawcallCount MEMBER frameDrawcallCount NOTIFY dirty)
        Q_PROPERTY(quint32 frameDrawcallRate MEMBER frameDrawcallRate NOTIFY dirty)

        Q_PROPERTY(quint32 frameTriangleCount MEMBER frameTriangleCount NOTIFY dirty)
        Q_PROPERTY(quint32 frameTriangleRate MEMBER frameTriangleRate NOTIFY dirty)

        Q_PROPERTY(quint32 frameTextureCount MEMBER frameTextureCount NOTIFY dirty)
        Q_PROPERTY(quint32 frameTextureRate MEMBER frameTextureRate NOTIFY dirty)
        Q_PROPERTY(quint32 frameTextureMemoryUsage MEMBER frameTextureMemoryUsage NOTIFY dirty)

        Q_PROPERTY(quint32 frameSetPipelineCount MEMBER frameSetPipelineCount NOTIFY dirty)
        Q_PROPERTY(quint32 frameSetInputFormatCount MEMBER frameSetInputFormatCount NOTIFY dirty)


    public:
        EngineStatsConfig() : Job::Config(true) {}

        quint32 bufferCPUCount{ 0 };
        quint32 bufferGPUCount{ 0 };
        qint64 bufferCPUMemoryUsage{ 0 };
        qint64 bufferGPUMemoryUsage{ 0 };

        quint32 textureCPUCount{ 0 };
        quint32 textureGPUCount{ 0 };
        qint64 textureCPUMemoryUsage{ 0 };
        qint64 textureGPUMemoryUsage{ 0 };
        qint64 textureGPUVirtualMemoryUsage{ 0 };
        quint32 textureGPUTransferCount{ 0 };

        quint32 frameAPIDrawcallCount{ 0 };
        quint32 frameDrawcallCount{ 0 };
        quint32 frameDrawcallRate{ 0 };

        quint32 frameTriangleCount{ 0 };
        quint32 frameTriangleRate{ 0 };

        quint32 frameTextureCount{ 0 };
        quint32 frameTextureRate{ 0 };
        qint64 frameTextureMemoryUsage{ 0 };

        quint32 frameSetPipelineCount{ 0 };

        quint32 frameSetInputFormatCount{ 0 };



        void emitDirty() { emit dirty(); }

    signals:
        void dirty();
    };

    class EngineStats {
        gpu::ContextStats _gpuStats;
        QElapsedTimer _frameTimer;
    public:
        using Config = EngineStatsConfig;
        using JobModel = Job::Model<EngineStats, Config>;

        EngineStats() { _frameTimer.start(); }

        void configure(const Config& configuration) {}
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);
    };
}

#endif