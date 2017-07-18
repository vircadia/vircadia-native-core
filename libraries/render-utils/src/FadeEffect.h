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
    render::ShapePipeline::ItemSetter getItemUniformSetter() const;
    render::ShapePipeline::ItemSetter getItemStoredSetter();

    int getLastCategory() const { return _lastCategory; }
    float getLastThreshold() const { return _lastThreshold; }
    const glm::vec3& getLastNoiseOffset() const { return _lastNoiseOffset; }
    const glm::vec3& getLastBaseOffset() const { return _lastBaseOffset; }
    const glm::vec3& getLastBaseInvSize() const { return _lastBaseInvSize; }

    static void packToAttributes(const int category, const float threshold, const glm::vec3& noiseOffset,
        const glm::vec3& baseOffset, const glm::vec3& baseInvSize,
        glm::vec4& packedData1, glm::vec4& packedData2, glm::vec4& packedData3);

private:

    gpu::BufferView _configurations;
    gpu::TexturePointer _maskMap;

    // The last fade set through the stored item setter
    int _lastCategory { 0 };
    float _lastThreshold { 0.f };
    glm::vec3 _lastNoiseOffset { 0.f, 0.f, 0.f };
    glm::vec3 _lastBaseOffset { 0.f, 0.f, 0.f };
    glm::vec3 _lastBaseInvSize { 1.f, 1.f, 1.f };

	explicit FadeEffect();
	virtual ~FadeEffect() { }

};
#endif // hifi_render_utils_FadeEffect_h
