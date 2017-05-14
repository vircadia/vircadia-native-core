
#include "PrototypeSelfie.h"


void MainRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor, bool isDeferred) {

    task.addJob<RenderShadowTask>("RenderShadowTask", cullFunctor);
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
    if (!isDeferred) {
        task.addJob<RenderForwardTask>("Forward", items);
    } else {
        task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    }
}


#include <TextureCache.h>

using RenderArgsPointer = std::shared_ptr<RenderArgs>;

void SelfieRenderTaskConfig::resetSize(int width, int height) {
    bool wasEnabled = isEnabled();
    setEnabled(false);
    auto textureCache = DependencyManager::get<TextureCache>();
    textureCache->resetSelfieFramebuffer(width, height);
    setEnabled(wasEnabled);
}

class BeginSelfieFrame {
public:
    using JobModel = render::Job::ModelO<BeginSelfieFrame, RenderArgsPointer>;


    void run(const render::RenderContextPointer& renderContext, RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;

        auto textureCache = DependencyManager::get<TextureCache>();

        auto destFramebuffer = textureCache->getSelfieFramebuffer();
        _cachedArgs._blitFramebuffer = args->_blitFramebuffer;
        args->_blitFramebuffer = destFramebuffer;
        args->_viewport = glm::ivec4(0, 0, destFramebuffer->getWidth(), destFramebuffer->getHeight());


        auto srcViewFrustum = args->getViewFrustum();
        auto srcPos = srcViewFrustum.getPosition();
        srcPos.x += 2.0f;
        srcViewFrustum.setPosition(srcPos);
        args->pushViewFrustum(srcViewFrustum);

       // cachedArgs = _cachedArgs;
    }

protected:
    RenderArgs _cachedArgs;
};

class EndSelfieFrame {
public:
    using JobModel = render::Job::ModelI<EndSelfieFrame, RenderArgsPointer>;


    void run(const render::RenderContextPointer& renderContext, const RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;
        args->popViewFrustum();
        
    }

protected:
};

void SelfieRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor) {


    const auto cachedArg = task.addJob<BeginSelfieFrame>("BeginSelfie");

    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
 
    task.addJob<RenderDeferredTask>("RenderDeferredTask", items);

    task.addJob<EndSelfieFrame>("EndSelfie", cachedArg);

}