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

#include "DeferredFrameTransform.h"
#include "DeferredFramebuffer.h"
#include "SurfaceGeometryPass.h"

class AmbientOcclusionFramebuffer {
public:
    AmbientOcclusionFramebuffer();

    gpu::FramebufferPointer getOcclusionFramebuffer();
    gpu::TexturePointer getOcclusionTexture();
    
    gpu::FramebufferPointer getOcclusionBlurredFramebuffer();
    gpu::TexturePointer getOcclusionBlurredTexture();
    
    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void updateLinearDepth(const gpu::TexturePointer& linearDepthBuffer);
    gpu::TexturePointer getLinearDepthTexture();
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }
    
    void setResolutionLevel(int level);
    int getResolutionLevel() const { return _resolutionLevel; }
    
protected:
    void clear();
    void allocate();
    
    gpu::TexturePointer _linearDepthTexture;
    
    gpu::FramebufferPointer _occlusionFramebuffer;
    gpu::TexturePointer _occlusionTexture;
    
    gpu::FramebufferPointer _occlusionBlurredFramebuffer;
    gpu::TexturePointer _occlusionBlurredTexture;
    
    
    glm::ivec2 _frameSize;
    int _resolutionLevel{ 0 };
};

using AmbientOcclusionFramebufferPointer = std::shared_ptr<AmbientOcclusionFramebuffer>;

class AmbientOcclusionEffectConfig : public render::Job::Config::Persistent {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY dirty)
    Q_PROPERTY(bool ditheringEnabled MEMBER ditheringEnabled NOTIFY dirty)
    Q_PROPERTY(bool borderingEnabled MEMBER borderingEnabled NOTIFY dirty)
    Q_PROPERTY(float radius MEMBER radius WRITE setRadius)
    Q_PROPERTY(float perspectiveScale MEMBER perspectiveScale WRITE setPerspectiveScale)
    Q_PROPERTY(float obscuranceLevel MEMBER obscuranceLevel WRITE setObscuranceLevel)
    Q_PROPERTY(float falloffBias MEMBER falloffBias WRITE setFalloffBias)
    Q_PROPERTY(float edgeSharpness MEMBER edgeSharpness WRITE setEdgeSharpness)
    Q_PROPERTY(float blurDeviation MEMBER blurDeviation WRITE setBlurDeviation)
    Q_PROPERTY(float numSpiralTurns MEMBER numSpiralTurns WRITE setNumSpiralTurns)
    Q_PROPERTY(int numSamples MEMBER numSamples WRITE setNumSamples)
    Q_PROPERTY(int resolutionLevel MEMBER resolutionLevel WRITE setResolutionLevel)
    Q_PROPERTY(int blurRadius MEMBER blurRadius WRITE setBlurRadius)
    Q_PROPERTY(double gpuTime READ getGpuTime)
public:
    AmbientOcclusionEffectConfig() : render::Job::Config::Persistent("Ambient Occlusion", false) {}

    const int MAX_RESOLUTION_LEVEL = 4;
    const int MAX_BLUR_RADIUS = 6;

    void setRadius(float newRadius) { radius = std::max(0.01f, newRadius); emit dirty(); }
    void setPerspectiveScale(float scale) { perspectiveScale = scale; emit dirty(); }
    void setObscuranceLevel(float level) { obscuranceLevel = std::max(0.01f, level); emit dirty(); }
    void setFalloffBias(float bias) { falloffBias = std::max(0.0f, std::min(bias, 0.2f)); emit dirty(); }
    void setEdgeSharpness(float sharpness) { edgeSharpness = std::max(0.0f, (float)sharpness); emit dirty(); }
    void setBlurDeviation(float deviation) { blurDeviation = std::max(0.0f, deviation); emit dirty(); }
    void setNumSpiralTurns(float turns) { numSpiralTurns = std::max(0.0f, (float)turns); emit dirty(); }
    void setNumSamples(int samples) { numSamples = std::max(1.0f, (float)samples); emit dirty(); }
    void setResolutionLevel(int level) { resolutionLevel = std::max(0, std::min(level, MAX_RESOLUTION_LEVEL)); emit dirty(); }
    void setBlurRadius(int radius) { blurRadius = std::max(0, std::min(MAX_BLUR_RADIUS, radius)); emit dirty(); }
    double getGpuTime() { return gpuTime; }

    float radius{ 0.5f };
    float perspectiveScale{ 1.0f };
    float obscuranceLevel{ 0.5f }; // intensify or dim down the obscurance effect
    float falloffBias{ 0.01f };
    float edgeSharpness{ 1.0f };
    float blurDeviation{ 2.5f };
    float numSpiralTurns{ 7.0f }; // defining an angle span to distribute the samples ray directions
    int numSamples{ 11 };
    int resolutionLevel{ 0 };
    int blurRadius{ 0 }; // 0 means no blurring
    bool ditheringEnabled{ true }; // randomize the distribution of rays per pixel, should always be true
    bool borderingEnabled{ true }; // avoid evaluating information from non existing pixels out of the frame, should always be true
    double gpuTime{ 0.0 };

signals:
    void dirty();
};


namespace gpu {
template <class T> class UniformBuffer : public gpu::BufferView {
  public:
    ~UniformBuffer<T>() {};
    UniformBuffer<T>() : gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(T), (const gpu::Byte*) &T())) {}

    const T* operator ->() const { return &get<T>(); }  
    T* operator ->() { return &edit<T>(); }

};


}

class AmbientOcclusionEffect {
public:
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, DeferredFramebufferPointer, LinearDepthFramebufferPointer>;
    using Outputs = render::VaryingSet2<AmbientOcclusionFramebufferPointer, gpu::BufferView>;
    using Config = AmbientOcclusionEffectConfig;
    using JobModel = render::Job::ModelIO<AmbientOcclusionEffect, Inputs, Outputs, Config>;

    AmbientOcclusionEffect();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);
    

    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        // Resolution info
        glm::vec4 resolutionInfo { -1.0f, 0.0f, 1.0f, 0.0f };
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

        int getResolutionLevel() const { return resolutionInfo.x; }
        float getRadius() const { return radiusInfo.x; }
        float getPerspectiveScale() const { return resolutionInfo.z; }
        float getObscuranceLevel() const { return radiusInfo.w; }
        float getFalloffBias() const { return (float)ditheringInfo.z; }
        float getEdgeSharpness() const { return (float)blurInfo.x; }
        float getBlurDeviation() const { return blurInfo.z; }
        float getNumSpiralTurns() const { return sampleInfo.z; }
        int getNumSamples() const { return (int)sampleInfo.x; }
        int getBlurRadius() const { return (int)blurInfo.y; }
        bool isDitheringEnabled() const { return ditheringInfo.x; }
        bool isBorderingEnabled() const { return ditheringInfo.w; }
    };
    using ParametersBuffer = gpu::UniformBuffer<Parameters>;

private:
    void updateGaussianDistribution();
   
    ParametersBuffer _parametersBuffer;

    const gpu::PipelinePointer& getOcclusionPipeline();
    const gpu::PipelinePointer& getHBlurPipeline(); // first
    const gpu::PipelinePointer& getVBlurPipeline(); // second

    gpu::PipelinePointer _occlusionPipeline;
    gpu::PipelinePointer _hBlurPipeline;
    gpu::PipelinePointer _vBlurPipeline;

    AmbientOcclusionFramebufferPointer _framebuffer;
    
    gpu::RangeTimer _gpuTimer;

    friend class DebugAmbientOcclusion;
};


class DebugAmbientOcclusionConfig : public render::Job::Config {
    Q_OBJECT

    Q_PROPERTY(bool showCursorPixel MEMBER showCursorPixel NOTIFY dirty)
    Q_PROPERTY(glm::vec2 debugCursorTexcoord MEMBER debugCursorTexcoord NOTIFY dirty)
public:
    DebugAmbientOcclusionConfig() : render::Job::Config(true) {}

    bool showCursorPixel{ false };
    glm::vec2 debugCursorTexcoord{ 0.5, 0.5 };

signals:
    void dirty();
};


class DebugAmbientOcclusion {
public:
    using Inputs = render::VaryingSet4<DeferredFrameTransformPointer, DeferredFramebufferPointer, LinearDepthFramebufferPointer, AmbientOcclusionEffect::ParametersBuffer>;
    using Config = DebugAmbientOcclusionConfig;
    using JobModel = render::Job::ModelI<DebugAmbientOcclusion, Inputs, Config>;

    DebugAmbientOcclusion();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);
    
private:
       
    // Class describing the uniform buffer with all the parameters common to the debug AO shaders
    class Parameters {
    public:
        // Pixel info
        glm::vec4 pixelInfo { 0.0f, 0.0f, 0.0f, 0.0f };
        
        Parameters() {}
    };
    gpu::UniformBuffer<Parameters> _parametersBuffer;

    const gpu::PipelinePointer& getDebugPipeline();

    gpu::PipelinePointer _debugPipeline;

    bool _showCursorPixel{ false };
};

#endif // hifi_AmbientOcclusionEffect_h
