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

class OutlineFramebuffer {
public:
    OutlineFramebuffer();

    gpu::FramebufferPointer getDepthFramebuffer();
    gpu::TexturePointer getDepthTexture();

    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void update(const gpu::TexturePointer& linearDepthBuffer);
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }

protected:

    void clear();
    void allocate();

    gpu::FramebufferPointer _depthFramebuffer;
    gpu::TexturePointer _depthTexture;

    glm::ivec2 _frameSize;
};

using OutlineFramebufferPointer = std::shared_ptr<OutlineFramebuffer>;

class PrepareOutline {

public:

    using Inputs = render::VaryingSet2<DeferredFramebufferPointer, render::ItemBounds>;
	// Output will contain outlined objects only z-depth texture
	using Output = OutlineFramebufferPointer;
	using JobModel = render::Job::ModelIO<PrepareOutline, Inputs, Output>;

	PrepareOutline() {}

	void run(const render::RenderContextPointer& renderContext, const PrepareOutline::Inputs& input, PrepareOutline::Output& output);

private:

    OutlineFramebufferPointer _outlineFramebuffer;
    gpu::PipelinePointer _copyDepthPipeline;
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
    float width{ 5.f };
    float intensity{ 1.f };
    float fillOpacityUnoccluded{ 0.35f };
    float fillOpacityOccluded{ 0.1f };
    bool glow{ false };

signals:
    void dirty();
};

class DrawOutline {
public:
    using Inputs = render::VaryingSet2<DeferredFramebufferPointer, OutlineFramebufferPointer>;
    using Config = DrawOutlineConfig;
    using JobModel = render::Job::ModelI<DrawOutline, Inputs, Config>;

    DrawOutline();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    enum {
        SCENE_DEPTH_SLOT = 0,
        OUTLINED_DEPTH_SLOT
    };

#include "Outline_shared.slh"

    using OutlineConfigurationBuffer = gpu::StructBuffer<OutlineParameters>;

    const gpu::PipelinePointer& getPipeline();

    gpu::PipelinePointer _pipeline;
    OutlineConfigurationBuffer _configuration;
    glm::vec3 _color;
    float _size;
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

#endif // hifi_render_utils_OutlineEffect_h


