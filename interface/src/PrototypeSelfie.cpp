
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
    using Config = BeginSelfieFrameConfig;
    using JobModel = render::Job::Model<BeginSelfieFrame>;

    void run(const render::RenderContextPointer& renderContext) {
        auto args = renderContext->args;
        auto textureCache = DependencyManager::get<TextureCache>();
        auto destFramebuffer = textureCache->getSelfieFramebuffer();
        args->_blitFramebuffer = destFramebuffer;
        args->_viewport = glm::ivec4(0, 0, destFramebuffer->getWidth(), destFramebuffer->getHeight());

        auto srcViewFrustum = args->getViewFrustum();
        auto srcPos = srcViewFrustum.getPosition();
        srcPos.x += 2.0f;
        srcViewFrustum.setPosition(srcPos);
        args->pushViewFrustum(srcViewFrustum);
    }
};

class EndSelfieFrame {
public:
    using JobModel = render::Job::Model<EndSelfieFrame>;

    void run(const render::RenderContextPointer& renderContext) {
        auto args = renderContext->args;
        args->popViewFrustum();
    }
};

void SelfieRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor) {
    task.addJob<BeginSelfieFrame>("BeginSelfie");
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
    task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    task.addJob<EndSelfieFrame>("EndSelfie");
}