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

class AmbientOcclusionEffect {
public:

    AmbientOcclusionEffect();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    typedef render::Job::Model<AmbientOcclusionEffect> JobModel;

    void setRadius(float radius);
    float getRadius() const { return _parametersBuffer.get<Parameters>()._radius_radius2_InvRadius6_s2.x; }
    
private:

    void setClipInfo(float nearZ, float farZ);

    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        glm::vec4 _clipInfo;
        glm::mat4 _projection;
        glm::vec4 _radius_radius2_InvRadius6_s2{ 0.5, 0.5 * 0.5, 1.0 / (0.25 * 0.25 * 0.25), 0.0 };

        Parameters() {}
    };
    typedef gpu::BufferView UniformBufferView;
    gpu::BufferView _parametersBuffer;

    // Class describing the uniform buffer with all the parameters common to the deferred shaders
    class DeferredTransform {
    public:
        glm::mat4 projection;
        glm::mat4 viewInverse;
        float stereoSide{ 0.f };
        float spareA, spareB, spareC;

        DeferredTransform() {}
    };
    UniformBufferView _deferredTransformBuffer[2];
    void updateDeferredTransformBuffer(const render::RenderContextPointer& renderContext);


    const gpu::PipelinePointer& getPyramidPipeline();
    const gpu::PipelinePointer& getOcclusionPipeline();
    const gpu::PipelinePointer& getHBlurPipeline(); // first
    const gpu::PipelinePointer& getVBlurPipeline(); // second

    gpu::PipelinePointer _pyramidPipeline;
    gpu::PipelinePointer _occlusionPipeline;
    gpu::PipelinePointer _hBlurPipeline;
    gpu::PipelinePointer _vBlurPipeline;
};

#endif // hifi_AmbientOcclusionEffect_h
