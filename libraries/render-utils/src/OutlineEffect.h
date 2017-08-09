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

/*
class PickItemsConfig : public render::Job::Config {
	Q_OBJECT
		Q_PROPERTY(bool isEnabled MEMBER isEnabled NOTIFY dirty)

public:

	bool isEnabled{ false };

signals:

	void dirty();
};
*/

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


