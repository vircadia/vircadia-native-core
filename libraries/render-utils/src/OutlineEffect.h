//
//  OutlineEffect.h
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_OutlineEffect_h
#define hifi_render_utils_OutlineEffect_h

#include <render/Engine.h>
#include "DeferredFramebuffer.h"
#include "DeferredFrameTransform.h"

class OutlineRessources {
public:
    OutlineRessources();

    gpu::FramebufferPointer getFramebuffer();
    gpu::TexturePointer getIdTexture();
    gpu::TexturePointer getDepthTexture();

    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void update(const gpu::TexturePointer& colorBuffer);
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }

protected:

    void clear();
    void allocate();

    gpu::FramebufferPointer _frameBuffer;
    gpu::TexturePointer _depthTexture;
    gpu::TexturePointer _idTexture;

    glm::ivec2 _frameSize;
};

using OutlineRessourcesPointer = std::shared_ptr<OutlineRessources>;

class PrepareDrawOutline {
public:
    using Inputs = gpu::FramebufferPointer;
    using Outputs = gpu::FramebufferPointer;
    using Config = render::Job::Config;
    using JobModel = render::Job::ModelIO<PrepareDrawOutline, Inputs, Outputs, Config>;

    PrepareDrawOutline();

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    gpu::FramebufferPointer _primaryWithoutDepthBuffer;
    gpu::Vec2u _frameBufferSize{ 0, 0 };

};

class DrawOutlineConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(bool glow MEMBER glow NOTIFY dirty)
        Q_PROPERTY(float width MEMBER width NOTIFY dirty)
        Q_PROPERTY(float intensity MEMBER intensity NOTIFY dirty)
        Q_PROPERTY(float colorR READ getColorR WRITE setColorR NOTIFY dirty)
        Q_PROPERTY(float colorG READ getColorG WRITE setColorG NOTIFY dirty)
        Q_PROPERTY(float colorB READ getColorB WRITE setColorB NOTIFY dirty)
        Q_PROPERTY(float fillOpacityUnoccluded MEMBER fillOpacityUnoccluded NOTIFY dirty)
        Q_PROPERTY(float fillOpacityOccluded MEMBER fillOpacityOccluded NOTIFY dirty)

public:

    void setColorR(float value) { color.r = value; emit dirty(); }
    float getColorR() const { return color.r; }

    void setColorG(float value) { color.g = value; emit dirty(); }
    float getColorG() const { return color.g; }

    void setColorB(float value) { color.b = value; emit dirty(); }
    float getColorB() const { return color.b; }

    glm::vec3 color{ 1.f, 0.7f, 0.2f };
    float width{ 2.0f };
    float intensity{ 0.9f };
    float fillOpacityUnoccluded{ 0.0f };
    float fillOpacityOccluded{ 0.0f };
    bool glow{ false };

signals:
    void dirty();
};

class DrawOutline {
private:

#include "Outline_shared.slh"

public:
    enum {
        MAX_GROUP_COUNT = GROUP_COUNT
    };

    using Inputs = render::VaryingSet4<DeferredFrameTransformPointer, OutlineRessourcesPointer, DeferredFramebufferPointer, gpu::FramebufferPointer>;
    using Config = DrawOutlineConfig;
    using JobModel = render::Job::ModelI<DrawOutline, Inputs, Config>;

    DrawOutline();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    enum {
        SCENE_DEPTH_SLOT = 0,
        OUTLINED_DEPTH_SLOT,
        OUTLINED_ID_SLOT,

        OUTLINE_PARAMS_SLOT = 0,
        FRAME_TRANSFORM_SLOT,
    };

    struct OutlineConfiguration {
        OutlineParameters _groups[MAX_GROUP_COUNT];
    };

    using OutlineConfigurationBuffer = gpu::StructBuffer<OutlineConfiguration>;

    static const gpu::PipelinePointer& getPipeline(bool isFilled);

    static gpu::PipelinePointer _pipeline;
    static gpu::PipelinePointer _pipelineFilled;
    OutlineConfigurationBuffer _configuration;
    glm::vec3 _color;
    float _size;
    int _blurKernelSize;
    float _intensity;
    float _fillOpacityUnoccluded;
    float _fillOpacityOccluded;
    float _threshold;
    bool _hasConfigurationChanged{ true };
};

class DrawOutlineTask {
public:

    using Groups = render::VaryingArray<render::ItemBounds, DrawOutline::MAX_GROUP_COUNT>;
    using Inputs = render::VaryingSet4<Groups, DeferredFramebufferPointer, gpu::FramebufferPointer, DeferredFrameTransformPointer>;
    using Config = render::Task::Config;
    using JobModel = render::Task::ModelI<DrawOutlineTask, Inputs, Config>;

    DrawOutlineTask();

    void configure(const Config& config);
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

private:

    static void initMaskPipelines(render::ShapePlumber& plumber, gpu::StatePointer state);
};

class DrawOutlineMask {
public:

    using Groups = render::VaryingArray<render::ShapeBounds, DrawOutline::MAX_GROUP_COUNT>;
    using Inputs = render::VaryingSet2<Groups, DeferredFramebufferPointer>;
    // Output will contain outlined objects only z-depth texture and the input primary buffer but without the primary depth buffer
    using Outputs = OutlineRessourcesPointer;
    using JobModel = render::Job::ModelIO<DrawOutlineMask, Inputs, Outputs>;

    DrawOutlineMask(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& output);

protected:

    render::ShapePlumberPointer _shapePlumber;
    OutlineRessourcesPointer _outlineRessources;
};

class DebugOutlineConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(bool viewMask MEMBER viewMask NOTIFY dirty)

public:

    bool viewMask{ false };

signals:
    void dirty();
};

class DebugOutline {
public:
    using Inputs = OutlineRessourcesPointer;
    using Config = DebugOutlineConfig;
    using JobModel = render::Job::ModelI<DebugOutline, Inputs, Config>;

    DebugOutline();
    ~DebugOutline();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    gpu::PipelinePointer _depthPipeline;
    gpu::PipelinePointer _idPipeline;
    int _geometryDepthId{ 0 };
    int _geometryColorId{ 0 };
    bool _isDisplayEnabled{ false };

    const gpu::PipelinePointer& getDepthPipeline();
    const gpu::PipelinePointer& getIdPipeline();
    void initializePipelines();
};


#endif // hifi_render_utils_OutlineEffect_h


