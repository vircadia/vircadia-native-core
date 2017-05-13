#pragma once
#ifndef hifi_PrototypeSelfie_h
#define hifi_PrototypeSelfie_h
    
#include <RenderShadowTask.h>
#include <render/RenderFetchCullSortTask.h>
#include <RenderDeferredTask.h>
#include <RenderForwardTask.h>
#include "OctreeRenderer.h"


class MainRenderTask {
public:
    using JobModel = render::Task::Model<MainRenderTask>;

    MainRenderTask() {}

    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor, bool isDeferred = true);
};

class SelfieRenderTaskConfig : public render::Task::Config {
    Q_OBJECT
public:
    SelfieRenderTaskConfig() : render::Task::Config(false) {}
signals:
    void dirty();

protected:
};

class SelfieRenderTask {
public:
    using Config = SelfieRenderTaskConfig;

    using JobModel = render::Task::Model<SelfieRenderTask, Config>;

    SelfieRenderTask() {}

    void configure(const Config& config) {}

    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor);
};

#endif
