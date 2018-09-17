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

#include <string>
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
    bool updateLinearDepth(const gpu::TexturePointer& linearDepthBuffer);
    gpu::TexturePointer getLinearDepthTexture();
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }
        
protected:
    void clear();
    void allocate();
    
    gpu::TexturePointer _linearDepthTexture;
    
    gpu::FramebufferPointer _occlusionFramebuffer;
    gpu::TexturePointer _occlusionTexture;
    
    gpu::FramebufferPointer _occlusionBlurredFramebuffer;
    gpu::TexturePointer _occlusionBlurredTexture;
    
    
    glm::ivec2 _frameSize;
};

using AmbientOcclusionFramebufferPointer = std::shared_ptr<AmbientOcclusionFramebuffer>;

class AmbientOcclusionEffectConfig : public render::GPUJobConfig::Persistent {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY dirty)
    Q_PROPERTY(bool ditheringEnabled MEMBER ditheringEnabled NOTIFY dirty)
    Q_PROPERTY(bool borderingEnabled MEMBER borderingEnabled NOTIFY dirty)
    Q_PROPERTY(bool fetchMipsEnabled MEMBER fetchMipsEnabled NOTIFY dirty)
    Q_PROPERTY(float radius MEMBER radius WRITE setRadius)
    Q_PROPERTY(float obscuranceLevel MEMBER obscuranceLevel WRITE setObscuranceLevel)
    Q_PROPERTY(float falloffAngle MEMBER falloffAngle WRITE setFalloffAngle)
    Q_PROPERTY(float falloffDistance MEMBER falloffDistance WRITE setFalloffDistance)
    Q_PROPERTY(float edgeSharpness MEMBER edgeSharpness WRITE setEdgeSharpness)
    Q_PROPERTY(float blurDeviation MEMBER blurDeviation WRITE setBlurDeviation)
    Q_PROPERTY(float numSpiralTurns MEMBER numSpiralTurns WRITE setNumSpiralTurns)
    Q_PROPERTY(int numSamples MEMBER numSamples WRITE setNumSamples)
    Q_PROPERTY(int resolutionLevel MEMBER resolutionLevel WRITE setResolutionLevel)
    Q_PROPERTY(int blurRadius MEMBER blurRadius WRITE setBlurRadius)

public:
    AmbientOcclusionEffectConfig();

    const int MAX_RESOLUTION_LEVEL = 4;
    const int MAX_BLUR_RADIUS = 15;

    void setRadius(float newRadius) { radius = std::max(0.01f, newRadius); emit dirty(); }
    void setObscuranceLevel(float level) { obscuranceLevel = std::max(0.01f, level); emit dirty(); }
    void setFalloffAngle(float bias) { falloffAngle = std::max(0.0f, std::min(bias, 1.0f)); emit dirty(); }
    void setFalloffDistance(float value) { falloffDistance = std::max(0.0f, value); emit dirty(); }
    void setEdgeSharpness(float sharpness) { edgeSharpness = std::max(0.0f, (float)sharpness); emit dirty(); }
    void setBlurDeviation(float deviation) { blurDeviation = std::max(0.0f, deviation); emit dirty(); }
    void setNumSpiralTurns(float turns) { numSpiralTurns = std::max(0.0f, (float)turns); emit dirty(); }
    void setNumSamples(int samples) { numSamples = std::max(1.0f, (float)samples); emit dirty(); }
    void setResolutionLevel(int level) { resolutionLevel = std::max(0, std::min(level, MAX_RESOLUTION_LEVEL)); emit dirty(); }
    void setBlurRadius(int radius) { blurRadius = std::max(0, std::min(MAX_BLUR_RADIUS, radius)); emit dirty(); }

    float radius;
    float perspectiveScale;
    float obscuranceLevel; // intensify or dim down the obscurance effect
    float falloffAngle;
    float falloffDistance;
    float edgeSharpness;
    float blurDeviation;
    float numSpiralTurns; // defining an angle span to distribute the samples ray directions
    int numSamples;
    int resolutionLevel;
    int blurRadius; // 0 means no blurring
    bool ditheringEnabled; // randomize the distribution of taps per pixel, should always be true
    bool borderingEnabled; // avoid evaluating information from non existing pixels out of the frame, should always be true
    bool fetchMipsEnabled; // fetch taps in sub mips to otpimize cache, should always be true

signals:
    void dirty();
};

class AmbientOcclusionEffect {
public:
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, DeferredFramebufferPointer, LinearDepthFramebufferPointer>;
    using Outputs = render::VaryingSet2<AmbientOcclusionFramebufferPointer, gpu::BufferView>;
    using Config = AmbientOcclusionEffectConfig;
    using JobModel = render::Job::ModelIO<AmbientOcclusionEffect, Inputs, Outputs, Config>;

    AmbientOcclusionEffect();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);
    

    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class AOParameters {
    public:
        // Resolution info
        glm::vec4 resolutionInfo;
        // radius info is { R, R^2, 1 / R^6, ObscuranceScale}
        glm::vec4 radiusInfo;
        // Dithering info 
        glm::vec4 ditheringInfo;
        // Sampling info
        glm::vec4 sampleInfo;
        // Blurring info
        glm::vec4 blurInfo;
         // gaussian distribution coefficients first is the sampling radius (max is 6)
        const static int GAUSSIAN_COEFS_LENGTH = 16;
        float _gaussianCoefs[GAUSSIAN_COEFS_LENGTH];
         
        AOParameters();

        int getResolutionLevel() const { return resolutionInfo.x; }
        float getRadius() const { return radiusInfo.x; }
        float getPerspectiveScale() const { return resolutionInfo.z; }
        float getObscuranceLevel() const { return radiusInfo.w; }
        float getFalloffAngle() const { return (float)ditheringInfo.z; }
        float getFalloffDistance() const { return ditheringInfo.y; }
        float getEdgeSharpness() const { return (float)blurInfo.x; }
        float getBlurDeviation() const { return blurInfo.z; }
        
        float getNumSpiralTurns() const { return sampleInfo.z; }
        int getNumSamples() const { return (int)sampleInfo.x; }
        bool isFetchMipsEnabled() const { return sampleInfo.w; }

        int getBlurRadius() const { return (int)blurInfo.y; }
        bool isDitheringEnabled() const { return ditheringInfo.x; }
        bool isBorderingEnabled() const { return ditheringInfo.w; }
    };
    using AOParametersBuffer = gpu::StructBuffer<AOParameters>;

private:

    // Class describing the uniform buffer with all the parameters common to the bilateral blur shaders
    class BlurParameters {
    public:
        glm::vec4 scaleHeight{ 0.0f };

        BlurParameters() {}
    };
    using BlurParametersBuffer = gpu::StructBuffer<BlurParameters>;


    void updateGaussianDistribution();
    void updateBlurParameters();
   
    AOParametersBuffer _aoParametersBuffer;
    BlurParametersBuffer _vblurParametersBuffer;
    BlurParametersBuffer _hblurParametersBuffer;

    static const gpu::PipelinePointer& getOcclusionPipeline();
	static const gpu::PipelinePointer& getHBlurPipeline(); // first
	static const gpu::PipelinePointer& getVBlurPipeline(); // second
	static const gpu::PipelinePointer& getMipCreationPipeline();

	static gpu::PipelinePointer _occlusionPipeline;
	static gpu::PipelinePointer _hBlurPipeline;
	static gpu::PipelinePointer _vBlurPipeline;
	static gpu::PipelinePointer _mipCreationPipeline;

    AmbientOcclusionFramebufferPointer _framebuffer;
    
    gpu::RangeTimerPointer _gpuTimer;

    friend class DebugAmbientOcclusion;
};


class DebugAmbientOcclusionConfig : public render::Job::Config {
    Q_OBJECT

    Q_PROPERTY(bool showCursorPixel MEMBER showCursorPixel NOTIFY dirty)
    Q_PROPERTY(glm::vec2 debugCursorTexcoord MEMBER debugCursorTexcoord NOTIFY dirty)
public:
    DebugAmbientOcclusionConfig() : render::Job::Config(false) {}

    bool showCursorPixel{ false };
    glm::vec2 debugCursorTexcoord{ 0.5f, 0.5f };

signals:
    void dirty();
};


class DebugAmbientOcclusion {
public:
    using Inputs = render::VaryingSet4<DeferredFrameTransformPointer, DeferredFramebufferPointer, LinearDepthFramebufferPointer, AmbientOcclusionEffect::AOParametersBuffer>;
    using Config = DebugAmbientOcclusionConfig;
    using JobModel = render::Job::ModelI<DebugAmbientOcclusion, Inputs, Config>;

    DebugAmbientOcclusion();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);
    
private:
       
    // Class describing the uniform buffer with all the parameters common to the debug AO shaders
    class Parameters {
    public:
        // Pixel info
        glm::vec4 pixelInfo { 0.0f, 0.0f, 0.0f, 0.0f };
        
        Parameters() {}
    };
    gpu::StructBuffer<Parameters> _parametersBuffer;

    const gpu::PipelinePointer& getDebugPipeline();

    gpu::PipelinePointer _debugPipeline;

    bool _showCursorPixel{ false };
};

#endif // hifi_AmbientOcclusionEffect_h
