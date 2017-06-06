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

class BeginSelfieFrameConfig : public render::Task::Config { // Exposes view frustum position/orientation to javascript.
    Q_OBJECT
    Q_PROPERTY(glm::vec3 position MEMBER position NOTIFY dirty)  // of viewpoint to render from
    Q_PROPERTY(glm::quat orientation MEMBER orientation NOTIFY dirty)  // of viewpoint to render from
public:
    glm::vec3 position{};
    glm::quat orientation{};
    BeginSelfieFrameConfig() : render::Task::Config(false) {}
signals:
    void dirty();
};

class SelfieRenderTaskConfig : public render::Task::Config {
    Q_OBJECT
public:
    SelfieRenderTaskConfig() : render::Task::Config(false) {}
signals:
    void dirty();
public slots:
    void resetSize(int width, int height);
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
