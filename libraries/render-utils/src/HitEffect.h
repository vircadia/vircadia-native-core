//
//  hitEffect.h
//  hifi
//
//  Created by eric levin on 7/17/15.
//
//

#ifndef hifi_hitEffect_h
#define hifi_hitEffect_h

#include <DependencyManager.h>
#include "render/DrawTask.h"

class AbstractViewStateInterface;
class ProgramObject;

class HitEffect {
public:
    
    HitEffect();
    
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    typedef render::Job::Model<HitEffect> JobModel;
    
    const gpu::PipelinePointer&     getHitEffectPipeline();
    
private:
    
    gpu::PipelinePointer _hitEffectPipeline;
};

#endif
