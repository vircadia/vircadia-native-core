//
//  FadeEffect.h

//  Created by Olivier Prat on 17/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_FadeEffect_h
#define hifi_render_utils_FadeEffect_h

#include <DependencyManager.h>
#include <render/Engine.h>

class FadeEffect : public Dependency {
    SINGLETON_DEPENDENCY;

public:

    void build(render::Task::TaskConcept& task, const task::Varying& editableItems);

    render::ShapePipeline::BatchSetter getBatchSetter() const;
    render::ShapePipeline::ItemSetter getItemSetter() const;

private:

    gpu::BufferView _configurations;
    gpu::TexturePointer _maskMap;

	explicit FadeEffect();
	virtual ~FadeEffect() { }

};
#endif // hifi_render_utils_FadeEffect_h
