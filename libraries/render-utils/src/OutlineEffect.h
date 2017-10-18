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

    gpu::FramebufferPointer getDepthFramebuffer();
    gpu::TexturePointer getDepthTexture();

    gpu::FramebufferPointer getColorFramebuffer() { return _colorFrameBuffer; }

    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void update(const gpu::FramebufferPointer& primaryFrameBuffer);
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }

protected:

    void clear();
    void allocate();

    gpu::FramebufferPointer _depthFrameBuffer;
    gpu::FramebufferPointer _colorFrameBuffer;

    glm::ivec2 _frameSize;
};

using OutlineRessourcesPointer = std::shared_ptr<OutlineRessources>;

class PrepareDrawOutline {
public:
    using Inputs = gpu::FramebufferPointer;
    using Outputs = OutlineRessourcesPointer;
    using JobModel = render::Job::ModelIO<PrepareDrawOutline, Inputs, Outputs>;

    PrepareDrawOutline();

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    OutlineRessourcesPointer _ressources;

};

class DrawOutlineMask {
public:

    using Inputs = render::VaryingSet2<render::ShapeBounds, OutlineRessourcesPointer>;
    using Outputs = glm::ivec4;
    using JobModel = render::Job::ModelIO<DrawOutlineMask, Inputs, Outputs>;

    DrawOutlineMask(render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber } {}

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

protected:

    render::ShapePlumberPointer _shapePlumber;

    static glm::ivec4 computeOutlineRect(const render::ShapeBounds& shapes, const ViewFrustum& viewFrustum, glm::ivec2 frameSize);
};

class DrawOutlineConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(bool glow MEMBER glow NOTIFY dirty)
        Q_PROPERTY(float width MEMBER width NOTIFY dirty)
        Q_PROPERTY(float intensity MEMBER intensity NOTIFY dirty)
        Q_PROPERTY(float colorR READ getColorR WRITE setColorR NOTIFY dirty)
        Q_PROPERTY(float colorG READ getColorG WRITE setColorG NOTIFY dirty)
        Q_PROPERTY(float colorB READ getColorB WRITE setColorB NOTIFY dirty)
        Q_PROPERTY(float unoccludedFillOpacity MEMBER unoccludedFillOpacity NOTIFY dirty)
        Q_PROPERTY(float occludedFillOpacity MEMBER occludedFillOpacity NOTIFY dirty)

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
    float unoccludedFillOpacity{ 0.0f };
    float occludedFillOpacity{ 0.0f };
    bool glow{ false };

signals:
    void dirty();
};

class DrawOutline {
public:

    using Inputs = render::VaryingSet4<DeferredFrameTransformPointer, OutlineRessourcesPointer, DeferredFramebufferPointer, glm::ivec4>;
    using Config = DrawOutlineConfig;
    using JobModel = render::Job::ModelI<DrawOutline, Inputs, Config>;

    DrawOutline();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

#include "Outline_shared.slh"

    enum {
        SCENE_DEPTH_SLOT = 0,
        OUTLINED_DEPTH_SLOT,

        OUTLINE_PARAMS_SLOT = 0,
        FRAME_TRANSFORM_SLOT,
    };

    using OutlineConfigurationBuffer = gpu::StructBuffer<OutlineParameters>;

    const gpu::PipelinePointer& getPipeline();

    static gpu::PipelinePointer _pipeline;
    static gpu::PipelinePointer _pipelineFilled;

    OutlineConfigurationBuffer _configuration;
    glm::ivec2 _framebufferSize{ 0,0 };
    bool _isFilled{ false };
    float _size;
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
    using Inputs = render::VaryingSet2<OutlineRessourcesPointer, glm::ivec4>;
    using Config = DebugOutlineConfig;
    using JobModel = render::Job::ModelI<DebugOutline, Inputs, Config>;

    DebugOutline();
    ~DebugOutline();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    gpu::PipelinePointer _depthPipeline;
    int _geometryDepthId{ 0 };
    bool _isDisplayEnabled{ false };

    const gpu::PipelinePointer& getDepthPipeline();
    void initializePipelines();
};

class DrawOutlineTask {
public:

    using Groups = render::VaryingArray<render::ItemBounds, render::Scene::MAX_OUTLINE_COUNT>;
    using Inputs = render::VaryingSet4<Groups, DeferredFramebufferPointer, gpu::FramebufferPointer, DeferredFrameTransformPointer>;
    using Config = render::Task::Config;
    using JobModel = render::Task::ModelI<DrawOutlineTask, Inputs, Config>;

    DrawOutlineTask();

    void configure(const Config& config);
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

private:

    static void initMaskPipelines(render::ShapePlumber& plumber, gpu::StatePointer state);
};

#endif // hifi_render_utils_OutlineEffect_h


