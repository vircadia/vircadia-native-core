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

#include "Shadows_shared.slh"

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
    RenderShadowTaskConfig() : render::Task::Config::Persistent(QStringList() << "Render" << "Engine" << "Shadows", true) {}

signals:
    void dirty();
};

class RenderShadowTask {
public:

    // There is one AABox per shadow cascade
    using Output = render::VaryingArray<AABox, SHADOW_CASCADE_MAX_COUNT>;
    using Config = RenderShadowTaskConfig;
    using JobModel = render::Task::ModelO<RenderShadowTask, Output, Config>;

    RenderShadowTask() {}
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cameraCullFunctor, uint8_t tagBits = 0x00, uint8_t tagMask = 0x00);

    void configure(const Config& configuration);

    struct CullFunctor {
        float _minSquareSize{ 0.0f };

        bool operator()(const RenderArgs* args, const AABox& bounds) const {
            // Cull only objects that are too small relatively to shadow frustum
            const auto boundsSquareRadius = glm::dot(bounds.getDimensions(), bounds.getDimensions());
            return boundsSquareRadius > _minSquareSize;
        }
    };

    CullFunctor _cullFunctor;

};

class RenderShadowSetupConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float constantBias0 MEMBER constantBias0 NOTIFY dirty)
        Q_PROPERTY(float constantBias1 MEMBER constantBias1 NOTIFY dirty)
        Q_PROPERTY(float constantBias2 MEMBER constantBias2 NOTIFY dirty)
        Q_PROPERTY(float constantBias3 MEMBER constantBias3 NOTIFY dirty)
        Q_PROPERTY(float slopeBias0 MEMBER slopeBias0 NOTIFY dirty)
        Q_PROPERTY(float slopeBias1 MEMBER slopeBias1 NOTIFY dirty)
        Q_PROPERTY(float slopeBias2 MEMBER slopeBias2 NOTIFY dirty)
        Q_PROPERTY(float slopeBias3 MEMBER slopeBias3 NOTIFY dirty)
public:

    float constantBias0{ 0.15f };
    float constantBias1{ 0.15f };
    float constantBias2{ 0.175f };
    float constantBias3{ 0.2f };
    float slopeBias0{ 0.6f };
    float slopeBias1{ 0.6f };
    float slopeBias2{ 0.7f };
    float slopeBias3{ 0.82f };

signals:
    void dirty();
};

class RenderShadowSetup {
public:
    using Outputs = render::VaryingSet3<RenderArgs::RenderMode, glm::ivec2, ViewFrustumPointer>;
    using Config = RenderShadowSetupConfig;
    using JobModel = render::Job::ModelO<RenderShadowSetup, Outputs, Config>;

    RenderShadowSetup();
    void configure(const Config& configuration);
    void run(const render::RenderContextPointer& renderContext, Outputs& output);

private:

    ViewFrustumPointer _cameraFrustum;
    ViewFrustumPointer _coarseShadowFrustum;
    struct {
        float _constant;
        float _slope;
    } _bias[SHADOW_CASCADE_MAX_COUNT];

    void setConstantBias(int cascadeIndex, float value);
    void setSlopeBias(int cascadeIndex, float value);
};

class RenderShadowCascadeSetup {
public:
    using Outputs = render::VaryingSet2<render::ItemFilter, ViewFrustumPointer>;
    using JobModel = render::Job::ModelO<RenderShadowCascadeSetup, Outputs>;

    RenderShadowCascadeSetup(unsigned int cascadeIndex, RenderShadowTask::CullFunctor& cullFunctor, uint8_t tagBits = 0x00, uint8_t tagMask = 0x00) : 
    _cascadeIndex{ cascadeIndex }, _cullFunctor{ cullFunctor }, _tagBits(tagBits), _tagMask(tagMask) {}
    void run(const render::RenderContextPointer& renderContext, Outputs& output);

private:

    unsigned int _cascadeIndex;
    RenderShadowTask::CullFunctor& _cullFunctor;
    uint8_t _tagBits{ 0x00 };
    uint8_t _tagMask{ 0x00 };
};

class RenderShadowCascadeTeardown {
public:
    using Input = render::ItemFilter;
    using JobModel = render::Job::ModelI<RenderShadowCascadeTeardown, Input>;
    void run(const render::RenderContextPointer& renderContext, const Input& input);
};

class RenderShadowTeardown {
public:
    using Input = RenderShadowSetup::Outputs;
    using JobModel = render::Job::ModelI<RenderShadowTeardown, Input>;
    void run(const render::RenderContextPointer& renderContext, const Input& input);
};

class CullShadowBounds {
public:
    using Inputs = render::VaryingSet3<render::ShapeBounds, render::ItemFilter, ViewFrustumPointer>;
    using Outputs = render::VaryingSet2<render::ShapeBounds, AABox>;
    using JobModel = render::Job::ModelIO<CullShadowBounds, Inputs, Outputs>;

    CullShadowBounds(render::CullFunctor cullFunctor) :
        _cullFunctor{ cullFunctor } {
    }

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    render::CullFunctor _cullFunctor;

};

#endif // hifi_RenderShadowTask_h
