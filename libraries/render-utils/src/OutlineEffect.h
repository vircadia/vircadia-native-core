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
#include <render/OutlineStyleStage.h>
#include <render/RenderFetchCullSortTask.h>

#include "DeferredFramebuffer.h"
#include "DeferredFrameTransform.h"

class OutlineRessources {
public:
    OutlineRessources();

    gpu::FramebufferPointer getDepthFramebuffer();
    gpu::TexturePointer getDepthTexture();

    gpu::FramebufferPointer getColorFramebuffer();

    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void update(const gpu::FramebufferPointer& primaryFrameBuffer);
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }

protected:

    gpu::FramebufferPointer _depthFrameBuffer;
    gpu::FramebufferPointer _colorFrameBuffer;
    gpu::TexturePointer _depthStencilTexture;

    glm::ivec2 _frameSize;

    void allocateColorBuffer(const gpu::FramebufferPointer& primaryFrameBuffer);
    void allocateDepthBuffer(const gpu::FramebufferPointer& primaryFrameBuffer);
};

using OutlineRessourcesPointer = std::shared_ptr<OutlineRessources>;

class OutlineSharedParameters {
public:

    enum {
        MAX_OUTLINE_COUNT = 8
    };

    OutlineSharedParameters();

    std::array<render::OutlineStyleStage::Index, MAX_OUTLINE_COUNT> _outlineIds;

    static float getBlurPixelWidth(const render::OutlineStyle& style, int frameBufferHeight);
};

using OutlineSharedParametersPointer = std::shared_ptr<OutlineSharedParameters>;

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

class SelectionToOutline {
public:

    using Outputs = std::vector<std::string>;
    using JobModel = render::Job::ModelO<SelectionToOutline, Outputs>;

    SelectionToOutline(OutlineSharedParametersPointer parameters) : _sharedParameters{ parameters } {}

    void run(const render::RenderContextPointer& renderContext, Outputs& outputs);

private:

    OutlineSharedParametersPointer _sharedParameters;
};

class ExtractSelectionName {
public:

    using Inputs = SelectionToOutline::Outputs;
    using Outputs = std::string;
    using JobModel = render::Job::ModelIO<ExtractSelectionName, Inputs, Outputs>;

    ExtractSelectionName(unsigned int outlineIndex) : _outlineIndex{ outlineIndex } {}

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    unsigned int _outlineIndex;

};

class DrawOutlineMask {
public:

    using Inputs = render::VaryingSet2<render::ShapeBounds, OutlineRessourcesPointer>;
    using Outputs = glm::ivec4;
    using JobModel = render::Job::ModelIO<DrawOutlineMask, Inputs, Outputs>;

    DrawOutlineMask(unsigned int outlineIndex, render::ShapePlumberPointer shapePlumber, OutlineSharedParametersPointer parameters);

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

protected:

    unsigned int _outlineIndex;
    render::ShapePlumberPointer _shapePlumber;
    OutlineSharedParametersPointer _sharedParameters;
    
    static gpu::BufferPointer _boundsBuffer;
    static gpu::PipelinePointer _stencilMaskPipeline;
    static gpu::PipelinePointer _stencilMaskFillPipeline;
};

class DrawOutline {
public:

    using Inputs = render::VaryingSet4<DeferredFrameTransformPointer, OutlineRessourcesPointer, DeferredFramebufferPointer, glm::ivec4>;
    using Config = render::Job::Config;
    using JobModel = render::Job::ModelI<DrawOutline, Inputs, Config>;

    DrawOutline(unsigned int outlineIndex, OutlineSharedParametersPointer parameters);

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

    static const gpu::PipelinePointer& getPipeline(const render::OutlineStyle& style);

    static gpu::PipelinePointer _pipeline;
    static gpu::PipelinePointer _pipelineFilled;

    unsigned int _outlineIndex;
    OutlineParameters _parameters;
    OutlineSharedParametersPointer _sharedParameters;
    OutlineConfigurationBuffer _configuration;
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

    using Inputs = render::VaryingSet4<RenderFetchCullSortTask::BucketList, DeferredFramebufferPointer, gpu::FramebufferPointer, DeferredFrameTransformPointer>;
    using Config = render::Task::Config;
    using JobModel = render::Task::ModelI<DrawOutlineTask, Inputs, Config>;

    DrawOutlineTask();

    void configure(const Config& config);
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

private:

    static void initMaskPipelines(render::ShapePlumber& plumber, gpu::StatePointer state);
    static const render::Varying addSelectItemJobs(JobModel& task, const render::Varying& selectionName, const RenderFetchCullSortTask::BucketList& items);

};

#endif // hifi_render_utils_OutlineEffect_h


