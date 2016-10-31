//
//  hitEffect.h
//  hifi
//
//  Created by eric levin on 7/17/15.
//
//

#ifndef hifi_hitEffect_h
#define hifi_hitEffect_h

#include <render/DrawTask.h>

class HitEffectConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled)
public:
    HitEffectConfig() : render::Job::Config(false) {}
};

class HitEffect {
public:
    using Config = HitEffectConfig;
    using JobModel = render::Job::Model<HitEffect, Config>;
    
    HitEffect();
    ~HitEffect();
    void configure(const Config& config) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    
    const gpu::PipelinePointer& getHitEffectPipeline();
    
private:
    int _geometryId { 0 };
    gpu::PipelinePointer _hitEffectPipeline;
};

#endif
