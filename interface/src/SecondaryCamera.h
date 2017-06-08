//
//  SecondaryCamera.h
//  interface/src
//
//  Created by Samuel Gateau, Howard Stearns, and Zach Fox on 2017-06-08.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_SecondaryCamera_h
#define hifi_SecondaryCamera_h
    
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

class BeginSecondaryCameraFrameConfig : public render::Task::Config { // Exposes view frustum position/orientation to javascript.
    Q_OBJECT
    Q_PROPERTY(glm::vec3 position MEMBER position NOTIFY dirty)  // of viewpoint to render from
    Q_PROPERTY(glm::quat orientation MEMBER orientation NOTIFY dirty)  // of viewpoint to render from
public:
    glm::vec3 position{};
    glm::quat orientation{};
    BeginSecondaryCameraFrameConfig() : render::Task::Config(false) {}
signals:
    void dirty();
};

class SecondaryCameraRenderTaskConfig : public render::Task::Config {
    Q_OBJECT
public:
    SecondaryCameraRenderTaskConfig() : render::Task::Config(false) {}
private:
    void resetSize(int width, int height);
signals:
    void dirty();
public slots:
    void resetSizeSpectatorCamera(int width, int height);
};

class SecondaryCameraRenderTask {
public:
    using Config = SecondaryCameraRenderTaskConfig;
    using JobModel = render::Task::Model<SecondaryCameraRenderTask, Config>;
    SecondaryCameraRenderTask() {}
    void configure(const Config& config) {}
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor);
};

#endif
