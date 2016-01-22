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

class RenderShadowMapConfig : public render::Job::Config {
    Q_OBJECT
public:
    RenderShadowMapConfig() : render::Job::Config(false) {}
};

class RenderShadowMap {
public:
    using Config = RenderShadowMapConfig;
    using JobModel = render::Job::ModelI<RenderShadowMap, render::ShapesIDsBounds, Config>;

    RenderShadowMap(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}
    void configure(const Config&) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
             const render::ShapesIDsBounds& inShapes);

protected:
    render::ShapePlumberPointer _shapePlumber;
};

class RenderShadowTask : public render::Task {
public:
    RenderShadowTask(render::CullFunctor shouldRender);

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = Model<RenderShadowTask>;
};

#endif // hifi_RenderShadowTask_h
