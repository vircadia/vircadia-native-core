//
//  RenderOutline.h
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_RenderOutline_h
#define hifi_render_utils_RenderOutline_h

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
class PrepareOutline {

public:

    using Input = DeferredFramebufferPointer;
	// Output will contain outlined objects only z-depth texture
	using Output = gpu::TexturePointer;
	using JobModel = render::Job::ModelIO<PrepareOutline, Input, Output>;

	PrepareOutline() {}

	void run(const render::RenderContextPointer& renderContext, const PrepareOutline::Input& input, PrepareOutline::Output& output);

private:

    gpu::FramebufferPointer _outlineFramebuffer;
    gpu::PipelinePointer _copyDepthPipeline;
};

#endif // hifi_render_utils_RenderOutline_h


