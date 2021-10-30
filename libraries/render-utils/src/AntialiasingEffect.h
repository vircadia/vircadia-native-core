//
//  AntialiasingEffect.h
//  libraries/render-utils/src/
//
//  Created by Raffi Bedikian on 8/30/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AntialiasingEffect_h
#define hifi_AntialiasingEffect_h

#include <DependencyManager.h>

#include "render/DrawTask.h"
#include "DeferredFrameTransform.h"
#include "VelocityBufferPass.h"


class JitterSampleConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float scale MEMBER scale NOTIFY dirty)
        Q_PROPERTY(bool freeze MEMBER freeze NOTIFY dirty)
        Q_PROPERTY(bool stop MEMBER stop NOTIFY dirty)
        Q_PROPERTY(int index READ getIndex NOTIFY dirty)
        Q_PROPERTY(int state READ getState WRITE setState NOTIFY dirty)
public:
    JitterSampleConfig() : render::Job::Config(true) {}

    float scale{ 0.5f };
    bool stop{ false };
    bool freeze{ false };

    void setIndex(int current);
    void setState(int state);

public slots:
    int cycleStopPauseRun();
    int prev();
    int next();
    int none();
    int pause();
    int play();

    int getIndex() const { return _index; }
    int getState() const { return _state; }
signals:
    void dirty();

private:
    int _state{ 0 };
    int _index{ 0 };

};


class JitterSample {
public:

    enum {
        SEQUENCE_LENGTH = 64 
    };

    using Config = JitterSampleConfig;
    using Output = glm::vec2;
    using JobModel = render::Job::ModelO<JitterSample, Output, Config>;

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, Output& jitter);

private:

    struct SampleSequence {
        SampleSequence();

        glm::vec2 offsets[SEQUENCE_LENGTH + 1];
        int sequenceLength{ SEQUENCE_LENGTH };
        int currentIndex{ 0 };
    };

    SampleSequence _sampleSequence;
    float _scale{ 1.0 };
    bool _freeze{ false };
};


class AntialiasingConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(int mode READ getAAMode WRITE setAAMode NOTIFY dirty)
    Q_PROPERTY(float blend MEMBER blend NOTIFY dirty)
    Q_PROPERTY(float sharpen MEMBER sharpen NOTIFY dirty)
    Q_PROPERTY(float covarianceGamma MEMBER covarianceGamma NOTIFY dirty)

    Q_PROPERTY(bool constrainColor MEMBER constrainColor NOTIFY dirty)
    Q_PROPERTY(bool feedbackColor MEMBER feedbackColor NOTIFY dirty)

    Q_PROPERTY(bool debug MEMBER debug NOTIFY dirty)
    Q_PROPERTY(float debugX MEMBER debugX NOTIFY dirty)
    Q_PROPERTY(bool fxaaOnOff READ debugFXAA WRITE setDebugFXAA NOTIFY dirty)
    Q_PROPERTY(float debugShowVelocityThreshold MEMBER debugShowVelocityThreshold NOTIFY dirty)
    Q_PROPERTY(bool showCursorPixel MEMBER showCursorPixel NOTIFY dirty)
    Q_PROPERTY(glm::vec2 debugCursorTexcoord MEMBER debugCursorTexcoord NOTIFY dirty)
    Q_PROPERTY(float debugOrbZoom MEMBER debugOrbZoom NOTIFY dirty)

    Q_PROPERTY(bool showClosestFragment MEMBER showClosestFragment NOTIFY dirty)

public:
    AntialiasingConfig() : render::Job::Config(true) {}

    enum Mode {
        NONE = 0,
        TAA,
        FXAA,
        MODE_COUNT
    };
    Q_ENUM(Mode) // Stored as signed int.

    void setAAMode(int mode);
    int getAAMode() const { return _mode; }

    void setDebugFXAA(bool debug) { debugFXAAX = (debug ? 0.0f : 1.0f); emit dirty();}
    bool debugFXAA() const { return (debugFXAAX == 0.0f ? true : false); }

    int _mode{ TAA }; // '_' prefix but not private?

    float blend{ 0.25f };
    float sharpen{ 0.05f };

    bool constrainColor{ true };
    float covarianceGamma{ 0.65f };
    bool feedbackColor{ false };

    float debugX{ 0.0f };
    float debugFXAAX{ 1.0f };
    float debugShowVelocityThreshold{ 1.0f };
    glm::vec2 debugCursorTexcoord{ 0.5f, 0.5f };
    float debugOrbZoom{ 2.0f };

    bool debug { false };
    bool showCursorPixel { false };
    bool showClosestFragment{ false };

signals:
    void dirty();
};

#define SET_BIT(bitfield, bitIndex, value) bitfield = ((bitfield) & ~(1 << (bitIndex))) | ((value) << (bitIndex))
#define GET_BIT(bitfield, bitIndex) ((bitfield) & (1 << (bitIndex)))

#define ANTIALIASING_USE_TAA    1

#if ANTIALIASING_USE_TAA

struct TAAParams {
    float nope{ 0.0f };
    float blend{ 0.15f };
    float covarianceGamma{ 1.0f };
    float debugShowVelocityThreshold{ 1.0f };

    glm::ivec4 flags{ 0 };
    glm::vec4 pixelInfo{ 0.5f, 0.5f, 2.0f, 0.0f };
    glm::vec4 regionInfo{ 0.0f, 0.0f, 1.0f, 0.0f };

    void setConstrainColor(bool enabled) { SET_BIT(flags.y, 1, enabled); }
    bool isConstrainColor() const { return (bool)GET_BIT(flags.y, 1); }

    void setFeedbackColor(bool enabled) { SET_BIT(flags.y, 4, enabled); }
    bool isFeedbackColor() const { return (bool)GET_BIT(flags.y, 4); }

    void setDebug(bool enabled) { SET_BIT(flags.x, 0, enabled); }
    bool isDebug() const { return (bool) GET_BIT(flags.x, 0); }

    void setShowDebugCursor(bool enabled) { SET_BIT(flags.x, 1, enabled); }
    bool showDebugCursor() const { return (bool)GET_BIT(flags.x, 1); }

    void setDebugCursor(glm::vec2 debugCursor) { pixelInfo.x = debugCursor.x; pixelInfo.y = debugCursor.y; }
    glm::vec2 getDebugCursor() const { return glm::vec2(pixelInfo.x, pixelInfo.y); }
    
    void setDebugOrbZoom(float orbZoom) { pixelInfo.z = orbZoom; }
    float getDebugOrbZoom() const { return pixelInfo.z; }

    void setShowClosestFragment(bool enabled) { SET_BIT(flags.x, 3, enabled); }

};
using TAAParamsBuffer = gpu::StructBuffer<TAAParams>;

class Antialiasing {
public:
    using Inputs = render::VaryingSet4 < DeferredFrameTransformPointer, gpu::FramebufferPointer, LinearDepthFramebufferPointer, VelocityFramebufferPointer > ;
    using Config = AntialiasingConfig;
    using JobModel = render::Job::ModelI<Antialiasing, Inputs, Config>;

    Antialiasing(bool isSharpenEnabled = true);
    ~Antialiasing();
    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

    const gpu::PipelinePointer& getAntialiasingPipeline(const render::RenderContextPointer& renderContext);
    const gpu::PipelinePointer& getBlendPipeline();
    const gpu::PipelinePointer& getDebugBlendPipeline();

private:

    gpu::FramebufferSwapChainPointer _antialiasingBuffers;
    gpu::TexturePointer _antialiasingTextures[2];
    gpu::BufferPointer _blendParamsBuffer;
    gpu::PipelinePointer _antialiasingPipeline;
    gpu::PipelinePointer _blendPipeline;
    gpu::PipelinePointer _debugBlendPipeline;

    TAAParamsBuffer _params;
    AntialiasingConfig::Mode _mode{ AntialiasingConfig::TAA };
    float _sharpen{ 0.15f };
    bool _isSharpenEnabled{ true };
};


#else // User setting for antialias mode will probably be broken.
class AntiAliasingConfig : public render::Job::Config { // Not to be confused with AntialiasingConfig...
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled)
public:
    AntiAliasingConfig() : render::Job::Config(true) {}
};

class Antialiasing {
public:
    using Config = AntiAliasingConfig;
    using JobModel = render::Job::ModelI<Antialiasing, gpu::FramebufferPointer, Config>;
    
    Antialiasing();
    ~Antialiasing();
    void configure(const Config& config) {}
    void run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer);
    
    const gpu::PipelinePointer& getAntialiasingPipeline();
    const gpu::PipelinePointer& getBlendPipeline();

private:
    gpu::FramebufferPointer _antialiasingBuffer;
    
    gpu::TexturePointer _antialiasingTexture;
    gpu::BufferPointer _paramsBuffer;
    
    gpu::PipelinePointer _antialiasingPipeline;
    gpu::PipelinePointer _blendPipeline;
    int _geometryId { 0 };
};
#endif

#endif // hifi_AntialiasingEffect_h
