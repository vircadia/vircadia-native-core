//
//  ZoneRenderer.h
//  render/src/render-utils
//
//  Created by Sam Gateau on 4/4/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ZoneRenderer_h
#define hifi_ZoneRenderer_h

#include "render/Engine.h"

#include "DeferredFrameTransform.h"

class ZoneRendererConfig : public render::Task::Config {
    Q_OBJECT
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
public:

    ZoneRendererConfig() : render::Task::Config(
    ) {}

    int maxDrawn { -1 };

signals:
    void dirty();

protected:
};

class ZoneRendererTask {
public:

    static const render::Selection::Name ZONES_SELECTION;


    using Inputs = render::ItemBounds;
    using Config = ZoneRendererConfig;
    using JobModel = render::Task::ModelI<ZoneRendererTask, Inputs, Config>;

    ZoneRendererTask() {}

    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }

protected:
    int _maxDrawn; // initialized by Config
};

class DebugZoneLighting {
public:
    class Config : public render::JobConfig {
    public:
        Config(bool enabled = false) : JobConfig(enabled) {}
    };

    using Inputs = DeferredFrameTransformPointer;
    using JobModel = render::Job::ModelI<DebugZoneLighting, Inputs, Config>;

    DebugZoneLighting() {}

    void configure(const Config& configuration) {}
    void run(const render::RenderContextPointer& context, const Inputs& inputs);

protected:

    enum Slots {
        ZONE_DEFERRED_TRANSFORM_BUFFER = 0,
        ZONE_KEYLIGHT_BUFFER,
        ZONE_AMBIENT_BUFFER,
        ZONE_AMBIENT_MAP,
        ZONE_SKYBOX_BUFFER,
        ZONE_SKYBOX_MAP,
    };

    gpu::PipelinePointer _keyLightPipeline;
    gpu::PipelinePointer _ambientPipeline;
    gpu::PipelinePointer _backgroundPipeline;

    const gpu::PipelinePointer& getKeyLightPipeline();
    const gpu::PipelinePointer& getAmbientPipeline();
    const gpu::PipelinePointer& getBackgroundPipeline();
};


#endif