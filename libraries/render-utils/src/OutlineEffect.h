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

class Outline {
protected:

#include "Outline_shared.slh"

public:
    enum {
        MAX_GROUP_COUNT = GROUP_COUNT
    };
};

class DrawOutlineConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(int group MEMBER group WRITE setGroup NOTIFY dirty);
        Q_PROPERTY(bool glow READ isGlow WRITE setGlow NOTIFY dirty)
        Q_PROPERTY(float width READ getWidth WRITE setWidth NOTIFY dirty)
        Q_PROPERTY(float intensity READ getIntensity WRITE setIntensity NOTIFY dirty)
        Q_PROPERTY(float colorR READ getColorR WRITE setColorR NOTIFY dirty)
        Q_PROPERTY(float colorG READ getColorG WRITE setColorG NOTIFY dirty)
        Q_PROPERTY(float colorB READ getColorB WRITE setColorB NOTIFY dirty)
        Q_PROPERTY(float unoccludedFillOpacity READ getUnoccludedFillOpacity WRITE setUnoccludedFillOpacity NOTIFY dirty)
        Q_PROPERTY(float occludedFillOpacity READ getOccludedFillOpacity WRITE setOccludedFillOpacity NOTIFY dirty)

public:

    struct GroupParameters {
        glm::vec3 color{ 1.f, 0.7f, 0.2f };
        float width{ 2.0f };
        float intensity{ 0.9f };
        float unoccludedFillOpacity{ 0.0f };
        float occludedFillOpacity{ 0.0f };
        bool glow{ false };
    };

    int getGroupCount() const;

    void setColorR(float value) { groupParameters[group].color.r = value; emit dirty(); }
    float getColorR() const { return groupParameters[group].color.r; }

    void setColorG(float value) { groupParameters[group].color.g = value; emit dirty(); }
    float getColorG() const { return groupParameters[group].color.g; }

    void setColorB(float value) { groupParameters[group].color.b = value; emit dirty(); }
    float getColorB() const { return groupParameters[group].color.b; }

    void setGlow(bool value) { groupParameters[group].glow = value; emit dirty(); }
    bool isGlow() const { return groupParameters[group].glow; }

    void setWidth(float value) { groupParameters[group].width = value; emit dirty(); }
    float getWidth() const { return groupParameters[group].width; }

    void setIntensity(float value) { groupParameters[group].intensity = value; emit dirty(); }
    float getIntensity() const { return groupParameters[group].intensity; }

    void setUnoccludedFillOpacity(float value) { groupParameters[group].unoccludedFillOpacity = value; emit dirty(); }
    float getUnoccludedFillOpacity() const { return groupParameters[group].unoccludedFillOpacity; }

    void setOccludedFillOpacity(float value) { groupParameters[group].occludedFillOpacity = value; emit dirty(); }
    float getOccludedFillOpacity() const { return groupParameters[group].occludedFillOpacity; }

    void setGroup(int value);

    int group{ 0 };
    GroupParameters groupParameters[Outline::MAX_GROUP_COUNT];

signals:
    void dirty();
};

class DrawOutline : public Outline {
public:

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

    enum Mode {
        M_ALL_UNFILLED,
        M_SOME_FILLED,
    };

    using OutlineConfigurationBuffer = gpu::StructBuffer<OutlineConfiguration>;

    const gpu::PipelinePointer& getPipeline();

    static gpu::PipelinePointer _pipeline;
    static gpu::PipelinePointer _pipelineFilled;
    OutlineConfigurationBuffer _configuration;
    glm::ivec2 _framebufferSize{ 0,0 };
    Mode _mode{ M_ALL_UNFILLED };
    float _sizes[MAX_GROUP_COUNT];
};

class DrawOutlineTask {
public:

    using Groups = render::VaryingArray<render::ItemBounds, Outline::MAX_GROUP_COUNT>;
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

    using Groups = render::VaryingArray<render::ShapeBounds, Outline::MAX_GROUP_COUNT>;
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


