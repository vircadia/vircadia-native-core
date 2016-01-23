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

class AmbientOcclusionEffectConfig : public render::Job::Config {
    Q_OBJECT
public:
    AmbientOcclusionEffectConfig() : render::Job::Config(false) {}

    Q_PROPERTY(float radius MEMBER radius WRITE setRadius NOTIFY dirty)
    Q_PROPERTY(float obscuranceLevel MEMBER obscuranceLevel WRITE setObscuranceLevel NOTIFY dirty)
    Q_PROPERTY(float falloffBias MEMBER falloffBias WRITE setFalloffBias NOTIFY dirty)
    Q_PROPERTY(float edgeSharpness MEMBER edgeSharpness WRITE setEdgeSharpness NOTIFY dirty)
    Q_PROPERTY(float blurDeviation MEMBER blurDeviation WRITE setBlurDeviation NOTIFY dirty)
    Q_PROPERTY(float numSpiralTurns MEMBER numSpiralTurns WRITE setNumSpiralTurns NOTIFY dirty)
    Q_PROPERTY(int numSamples MEMBER numSamples WRITE setNumSamples NOTIFY dirty)
    Q_PROPERTY(int resolutionLevel MEMBER resolutionLevel WRITE setResolutionLevel NOTIFY dirty)
    Q_PROPERTY(int blurRadius MEMBER blurRadius WRITE setBlurRadius NOTIFY dirty)
    Q_PROPERTY(bool ditheringEnabled MEMBER ditheringEnabled NOTIFY dirty)
    Q_PROPERTY(bool borderingEnabled MEMBER borderingEnabled NOTIFY dirty)
    Q_PROPERTY(double gpuTime READ getGpuTime)

    void setRadius(float radius);
    void setObscuranceLevel(float obscuranceLevel);
    void setFalloffBias(float falloffBias);
    void setEdgeSharpness(float edgeSharpness);
    void setBlurDeviation(float blurDeviation);
    void setNumSpiralTurns(float numSpiralTurns);
    void setNumSamples(int numSamples);
    void setResolutionLevel(int resolutionLevel);
    void setBlurRadius(int blurRadius);
    double getGpuTime() { return gpuTime; }

    float radius{ 0.5f };
    float obscuranceLevel{ 0.5f }; // intensify or dim down the obscurance effect
    float falloffBias{ 0.01f };
    float edgeSharpness{ 1.0f };
    float blurDeviation{ 2.5f };
    float numSpiralTurns{ 7.0f }; // defining an angle span to distribute the samples ray directions
    int numSamples{ 11 };
    int resolutionLevel{ 1 };
    int blurRadius{ 4 }; // 0 means no blurring
    bool ditheringEnabled{ true }; // randomize the distribution of rays per pixel, should always be true
    bool borderingEnabled{ true }; // avoid evaluating information from non existing pixels out of the frame, should always be true
    double gpuTime{ 0.0 };

signals:
    void dirty();
};

class AmbientOcclusionEffect {
public:
    using Config = AmbientOcclusionEffectConfig;
    using JobModel = render::Job::Model<AmbientOcclusionEffect, Config>;

    AmbientOcclusionEffect();

    void configure(const Config& configuration);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    
    float getRadius() const { return _parametersBuffer.get<Parameters>().radiusInfo.x; }
    float getObscuranceLevel() const { return _parametersBuffer.get<Parameters>().radiusInfo.w; }
    float getFalloffBias() const { return (float)_parametersBuffer.get<Parameters>().ditheringInfo.z; }
    float getEdgeSharpness() const { return (float)_parametersBuffer.get<Parameters>().blurInfo.x; }
    float getBlurDeviation() const { return _parametersBuffer.get<Parameters>().blurInfo.z; }
    float getNumSpiralTurns() const { return _parametersBuffer.get<Parameters>().sampleInfo.z; }
    int getNumSamples() const { return (int)_parametersBuffer.get<Parameters>().sampleInfo.x; }
    int getResolutionLevel() const { return _parametersBuffer.get<Parameters>().resolutionInfo.x; }
    int getBlurRadius() const { return (int)_parametersBuffer.get<Parameters>().blurInfo.y; }
    bool isDitheringEnabled() const { return _parametersBuffer.get<Parameters>().ditheringInfo.x; }
    bool isBorderingEnabled() const { return _parametersBuffer.get<Parameters>().ditheringInfo.w; }
    
private:
    void updateGaussianDistribution();
    void setDepthInfo(float nearZ, float farZ);
    
    typedef gpu::BufferView UniformBufferView;

    // Class describing the uniform buffer with the transform info common to the AO shaders
    // It s changing every frame
    class FrameTransform {
    public:
        // Pixel info is { viemport width height and stereo on off}
        glm::vec4 pixelInfo;
        // Depth info is { n.f, f - n, -f}
        glm::vec4 depthInfo;
        // Stereo info
        glm::vec4 stereoInfo { 0.0 };
        // Mono proj matrix or Left and Right proj matrix going from Mono Eye space to side clip space
        glm::mat4 projection[2];
        
        FrameTransform() {}
    };
    gpu::BufferView _frameTransformBuffer;
    
    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        // Resolution info
        glm::vec4 resolutionInfo { -1.0f, 0.0f, 0.0f, 0.0f };
        // radius info is { R, R^2, 1 / R^6, ObscuranceScale}
        glm::vec4 radiusInfo{ 0.5f, 0.5f * 0.5f, 1.0f / (0.25f * 0.25f * 0.25f), 1.0f };
        // Dithering info 
        glm::vec4 ditheringInfo { 0.0f, 0.0f, 0.01f, 1.0f };
        // Sampling info
        glm::vec4 sampleInfo { 11.0f, 1.0f/11.0f, 7.0f, 1.0f };
        // Blurring info
        glm::vec4 blurInfo { 1.0f, 3.0f, 2.0f, 0.0f };
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
