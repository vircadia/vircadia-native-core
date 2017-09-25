//
//  BloomEffect.h
//  render-utils/src/
//
//  Created by Olivier Prat on 09/25/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_BloomEffect_h
#define hifi_render_utils_BloomEffect_h

#include <render/Engine.h>

class BloomConfig : public render::Task::Config {
	Q_OBJECT
		Q_PROPERTY(float mix MEMBER mix WRITE setMix NOTIFY dirty)
		Q_PROPERTY(float size MEMBER size WRITE setSize NOTIFY dirty)

public:

	float mix{ 0.2f };
	float size{ 0.1f };

    void setMix(float value);
    void setSize(float value);

signals:
	void dirty();
};

class Bloom {
public:
	using Inputs = gpu::FramebufferPointer;
	using Config = BloomConfig;
	using JobModel = render::Task::ModelI<Bloom, Inputs, Config>;

	Bloom();

	void configure(const Config& config);
	void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

};

#endif // hifi_render_utils_BloomEffect_h
