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

#include <render/Scene.h>
#include <render/DrawTask.h>

class ViewFrustum;

class RenderShadowMap {
public:
    RenderShadowMap(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
             const render::ShapesIDsBounds& inShapes);

    using JobModel = render::Task::Job::ModelI<RenderShadowMap, render::ShapesIDsBounds>;
protected:
    render::ShapePlumberPointer _shapePlumber;
};

class RenderShadowTask : public render::Task {
public:
    RenderShadowTask();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
};

#endif // hifi_RenderShadowTask_h
