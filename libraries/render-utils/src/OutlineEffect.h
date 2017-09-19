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

class OutlineFramebuffer {
public:
    OutlineFramebuffer();

    gpu::FramebufferPointer getDepthFramebuffer();
    gpu::TexturePointer getDepthTexture();

    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void update(const gpu::TexturePointer& colorBuffer);
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }

protected:

    void clear();
    void allocate();

    gpu::FramebufferPointer _depthFramebuffer;
    gpu::TexturePointer _depthTexture;

    glm::ivec2 _frameSize;
};

using OutlineFramebufferPointer = std::shared_ptr<OutlineFramebuffer>;

class DrawOutlineDepth {
public:

    using Inputs = render::VaryingSet2<render::ShapeBounds, DeferredFramebufferPointer>;
    // Output will contain outlined objects only z-depth texture and the input primary buffer but without the primary depth buffer
    using Outputs = OutlineFramebufferPointer;
    using JobModel = render::Job::ModelIO<DrawOutlineDepth, Inputs, Outputs>;

    DrawOutlineDepth(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& output);

protected:

    render::ShapePlumberPointer _shapePlumber;
    OutlineFramebufferPointer _outlineFramebuffer;
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
    float width{ 3.f };
    float intensity{ 1.f };
    float fillOpacityUnoccluded{ 0.35f };
    float fillOpacityOccluded{ 0.1f };
    bool glow{ false };

signals:
    void dirty();
};

class DrawOutline {
public:
    using Inputs = render::VaryingSet4<DeferredFrameTransformPointer, OutlineFramebufferPointer, DeferredFramebufferPointer, gpu::FramebufferPointer>;
    using Config = DrawOutlineConfig;
    using JobModel = render::Job::ModelI<DrawOutline, Inputs, Config>;

    DrawOutline();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    enum {
        SCENE_DEPTH_SLOT = 0,
        OUTLINED_DEPTH_SLOT,

        OUTLINE_PARAMS_SLOT = 0,
        FRAME_TRANSFORM_SLOT
    };

#include "Outline_shared.slh"

    using OutlineConfigurationBuffer = gpu::StructBuffer<OutlineParameters>;

    const gpu::PipelinePointer& getPipeline(bool isFilled);

    gpu::FramebufferPointer _primaryWithoutDepthBuffer;
    glm::ivec2 _frameBufferSize {0, 0};
    gpu::PipelinePointer _pipeline;
    gpu::PipelinePointer _pipelineFilled;
    OutlineConfigurationBuffer _configuration;
    glm::vec3 _color;
    float _size;
    int _blurKernelSize;
    float _intensity;
    float _fillOpacityUnoccluded;
    float _fillOpacityOccluded;
    float _threshold;
};

class DebugOutlineConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(bool viewOutlinedDepth MEMBER viewOutlinedDepth NOTIFY dirty)

public:

    bool viewOutlinedDepth{ false };

signals:
    void dirty();
};

class DebugOutline {
public:
    using Inputs = OutlineFramebufferPointer;
    using Config = DebugOutlineConfig;
    using JobModel = render::Job::ModelI<DebugOutline, Inputs, Config>;

    DebugOutline();
    ~DebugOutline();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    const gpu::PipelinePointer& getDebugPipeline();

    gpu::PipelinePointer _debugPipeline;
    int _geometryId{ 0 };
    bool _isDisplayDepthEnabled{ false };
};

class DrawOutlineTask {
public:
    using Inputs = render::VaryingSet5<render::ItemBounds, render::ShapePlumberPointer, DeferredFramebufferPointer, gpu::FramebufferPointer, DeferredFrameTransformPointer>;
    using Config = render::Task::Config;
    using JobModel = render::Task::ModelI<DrawOutlineTask, Inputs, Config>;

    DrawOutlineTask();

    void configure(const Config& config);
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

};

#endif // hifi_render_utils_OutlineEffect_h


