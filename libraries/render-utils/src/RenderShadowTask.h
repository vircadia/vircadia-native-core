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

#include <render/CullTask.h>

class ViewFrustum;

class RenderShadowMap {
public:
    using Inputs = render::VaryingSet2<render::ShapeBounds, AABox>;
    using JobModel = render::Job::ModelI<RenderShadowMap, Inputs>;

    RenderShadowMap(render::ShapePlumberPointer shapePlumber, unsigned int cascadeIndex) : _shapePlumber{ shapePlumber }, _cascadeIndex{ cascadeIndex } {}
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    unsigned int _cascadeIndex;
};

class RenderShadowTaskConfig : public render::Task::Config::Persistent {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY dirty)
public:
    RenderShadowTaskConfig() : render::Task::Config::Persistent(QStringList() << "Render" << "Engine" << "Shadows", false) {}

signals:
    void dirty();
};

class RenderShadowTask {
public:
    using Config = RenderShadowTaskConfig;
    using JobModel = render::Task::Model<RenderShadowTask, Config>;

    RenderShadowTask() {}
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor shouldRender);

    void configure(const Config& configuration);
};

class RenderShadowSetup {
public:
    using JobModel = render::Job::Model<RenderShadowSetup>;

    RenderShadowSetup() {}
    void run(const render::RenderContextPointer& renderContext);

};

class RenderShadowCascadeSetup {
public:
    using Outputs = render::VaryingSet3<RenderArgs::RenderMode, render::ItemFilter, float>;
    using JobModel = render::Job::ModelO<RenderShadowCascadeSetup, Outputs>;

    RenderShadowCascadeSetup(unsigned int cascadeIndex) : _cascadeIndex{ cascadeIndex } {}
    void run(const render::RenderContextPointer& renderContext, Outputs& output);

private:

    unsigned int _cascadeIndex;
};

class RenderShadowCascadeTeardown {
public:
    using Input = RenderShadowCascadeSetup::Outputs;
    using JobModel = render::Job::ModelI<RenderShadowCascadeTeardown, Input>;
    void run(const render::RenderContextPointer& renderContext, const Input& input);
};

#endif // hifi_RenderShadowTask_h
