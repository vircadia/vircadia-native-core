//
//  PickItemsJob.h
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_PickItemsJob_h
#define hifi_render_utils_PickItemsJob_h

#include <render/Engine.h>

class PickItemsConfig : public render::Job::Config {

public:

    PickItemsConfig() : render::Job::Config(false) {}
};

class PickItemsJob {

public:

	using Config = PickItemsConfig;
	using Input = render::ItemBounds;
	using Output = render::ItemBounds;
	using JobModel = render::Job::ModelIO<PickItemsJob, Input, Output, Config>;

    PickItemsJob();

	void configure(const Config& config);
	void run(const render::RenderContextPointer& renderContext, const PickItemsJob::Input& input, PickItemsJob::Output& output);

private:

	bool _isEnabled{ false };

	render::ItemID findNearestItem(const render::RenderContextPointer& renderContext, const render::ItemBounds& inputs, float& minIsectDistance) const;
};

#endif // hifi_render_utils_PickItemsJob_h


