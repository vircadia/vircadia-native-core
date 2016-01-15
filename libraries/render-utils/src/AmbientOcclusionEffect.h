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
    
    void setRadius(float radius);
    float getRadius() const { return _parametersBuffer.get<Parameters>()._radiusInfo.x; }
    
    // Obscurance level which intensify or dim down the obscurance effect
    void setLevel(float level);
    float getLevel() const { return _parametersBuffer.get<Parameters>()._radiusInfo.w; }

    void setDithering(bool enabled);
    bool isDitheringEnabled() const { return _parametersBuffer.get<Parameters>()._ditheringInfo.x; }

    // Number of samples per pixel to evaluate the Obscurance
    void setNumSamples(int numSamples);
    int getNumSamples() const { return (int)_parametersBuffer.get<Parameters>()._sampleInfo.x; }

    // Number of spiral turns defining an angle span to distribute the samples ray directions
    void setNumSpiralTurns(float numTurns);
    float getNumSpiralTurns() const { return _parametersBuffer.get<Parameters>()._sampleInfo.z; }

    // Edge blurring setting
    void setEdgeSharpness(float sharpness);
    int getEdgeSharpness() const { return (int)_parametersBuffer.get<Parameters>()._blurInfo.x; }

    using JobModel = render::Task::Job::Model<AmbientOcclusionEffect>;

private:

    void setDepthInfo(float nearZ, float farZ);

    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        // radius info is { R, R^2, 1 / R^6, ObscuranceScale}
        glm::vec4 _radiusInfo{ 0.5, 0.5 * 0.5, 1.0 / (0.25 * 0.25 * 0.25), 1.0 };
        // Dithering info
        glm::vec4 _ditheringInfo{ 0.0, 0.0, 0.0, 0.0 };
        // Sampling info
        glm::vec4 _sampleInfo{ 11.0, 1.0/11.0, 7.0, 1.0 };
        // Blurring info
        glm::vec4 _blurInfo{ 1.0, 0.0, 0.0, 0.0 };
        // Pixel info is { viemport width height and stereo on off}
        glm::vec4 _pixelInfo;
        // Depth info is { n.f, f - n, -f}
        glm::vec4 _depthInfo;
        // Stereo info
        glm::vec4 _stereoInfo{ 0.0 };
        // Mono proj matrix or Left and Right proj matrix going from Mono Eye space to side clip space
        glm::mat4 _projection[2];
        
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
