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
    
    void setResolutionLevel(int level);
    int getResolutionLevel() const { return _parametersBuffer.get<Parameters>()._resolutionInfo.x; }

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

    // Blurring Radius
    // 0 means no blurring
    const int MAX_BLUR_RADIUS = 6;
    void setBlurRadius(int radius);
    int getBlurRadius() const { return (int)_parametersBuffer.get<Parameters>()._blurInfo.y; }

    void setBlurDeviation(float deviation);
    float getBlurDeviation() const { return _parametersBuffer.get<Parameters>()._blurInfo.z; }

    
    double getGPUTime() const { return _gpuTimer.getAverage(); }
    
    using JobModel = render::Task::Job::Model<AmbientOcclusionEffect>;

private:

    void updateGaussianDistribution();
    void setDepthInfo(float nearZ, float farZ);
    
    typedef gpu::BufferView UniformBufferView;

    // Class describing the uniform buffer with the transform info common to the AO shaders
    // It s changing every frame
    class FrameTransform {
    public:
        // Pixel info is { viemport width height and stereo on off}
        glm::vec4 _pixelInfo;
        // Depth info is { n.f, f - n, -f}
        glm::vec4 _depthInfo;
        // Stereo info
        glm::vec4 _stereoInfo{ 0.0 };
        // Mono proj matrix or Left and Right proj matrix going from Mono Eye space to side clip space
        glm::mat4 _projection[2];
        
        FrameTransform() {}
    };
    gpu::BufferView _frameTransformBuffer;
    
    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        // Resolution info
        glm::vec4 _resolutionInfo{ 1.0, 1.0, 1.0, 1.0 };
        // radius info is { R, R^2, 1 / R^6, ObscuranceScale}
        glm::vec4 _radiusInfo{ 0.5, 0.5 * 0.5, 1.0 / (0.25 * 0.25 * 0.25), 1.0 };
        // Dithering info
        glm::vec4 _ditheringInfo{ 0.0, 0.0, 0.0, 0.0 };
        // Sampling info
        glm::vec4 _sampleInfo{ 11.0, 1.0/11.0, 7.0, 1.0 };
        // Blurring info
        glm::vec4 _blurInfo{ 1.0, 3.0, 2.0, 0.0 };
         // gaussian distribution coefficients first is the sampling radius (max is 6)
        const static int GAUSSIAN_COEFS_LENGTH = 8;
        float _gaussianCoefs[GAUSSIAN_COEFS_LENGTH];
        
        Parameters() {}
    };
    gpu::BufferView _parametersBuffer;

    const gpu::PipelinePointer& getPyramidPipeline();
    const gpu::PipelinePointer& getOcclusionPipeline();
    const gpu::PipelinePointer& getHBlurPipeline(); // first
    const gpu::PipelinePointer& getVBlurPipeline(); // second

    gpu::PipelinePointer _pyramidPipeline;
    gpu::PipelinePointer _occlusionPipeline;
    gpu::PipelinePointer _hBlurPipeline;
    gpu::PipelinePointer _vBlurPipeline;

    gpu::RangeTimer _gpuTimer;
};

#endif // hifi_AmbientOcclusionEffect_h
