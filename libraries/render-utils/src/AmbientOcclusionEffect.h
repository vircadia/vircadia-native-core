//
//  AmbientOcclusionEffect.h
//  libraries/render-utils/src/
//
//  Created by Niraj Venkat on 7/15/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AmbientOcclusionEffect_h
#define hifi_AmbientOcclusionEffect_h

#include <DependencyManager.h>

#include "render/DrawTask.h"

class AmbientOcclusion {
public:

    AmbientOcclusion();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    using JobModel = render::Task::Job::Model<AmbientOcclusion>;

    const gpu::PipelinePointer& getOcclusionPipeline();
    const gpu::PipelinePointer& getHBlurPipeline();
    const gpu::PipelinePointer& getVBlurPipeline();
    const gpu::PipelinePointer& getBlendPipeline();

private:

    // Uniforms for AO
    gpu::int32 _gScaleLoc;
    gpu::int32 _gBiasLoc;
    gpu::int32 _gSampleRadiusLoc;
    gpu::int32 _gIntensityLoc;

    gpu::int32 _nearLoc;
    gpu::int32 _depthScaleLoc;
    gpu::int32 _depthTexCoordOffsetLoc;
    gpu::int32 _depthTexCoordScaleLoc;
    gpu::int32 _renderTargetResLoc;
    gpu::int32 _renderTargetResInvLoc;


    float g_scale;
    float g_bias;
    float g_sample_rad;
    float g_intensity;

    gpu::PipelinePointer _occlusionPipeline;
    gpu::PipelinePointer _hBlurPipeline;
    gpu::PipelinePointer _vBlurPipeline;
    gpu::PipelinePointer _blendPipeline;

    gpu::FramebufferPointer _occlusionBuffer;
    gpu::FramebufferPointer _hBlurBuffer;
    gpu::FramebufferPointer _vBlurBuffer;

    gpu::TexturePointer _occlusionTexture;
    gpu::TexturePointer _hBlurTexture;
    gpu::TexturePointer _vBlurTexture;

};

#endif // hifi_AmbientOcclusionEffect_h
