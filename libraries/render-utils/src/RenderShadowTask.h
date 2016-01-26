//
//  RenderShadowTask.h
//  render-utils/src/
//
//  Created by Zach Pomerantz on 1/7/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderShadowTask_h
#define hifi_RenderShadowTask_h

#include <gpu/Framebuffer.h>
#include <gpu/Pipeline.h>

#include <render/DrawTask.h>

class ViewFrustum;

class RenderShadowMap {
public:
    using JobModel = render::Job::ModelI<RenderShadowMap, render::ShapesIDsBounds>;

    RenderShadowMap(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
             const render::ShapesIDsBounds& inShapes);

protected:
    render::ShapePlumberPointer _shapePlumber;
};

class RenderShadowTaskConfig : public render::Task::Config {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY dirty)
public:
    RenderShadowTaskConfig() : render::Task::Config(false) {}

signals:
    void dirty();
};

class RenderShadowTask : public render::Task {
public:
    using Config = RenderShadowTaskConfig;
    using JobModel = Model<RenderShadowTask, Config>;

    RenderShadowTask(render::CullFunctor shouldRender);

    void configure(const Config& configuration);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
};

#endif // hifi_RenderShadowTask_h
