
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
    glm::vec3 _position{};
    glm::quat _orientation{};
public:
    using Config = BeginSelfieFrameConfig;
    using JobModel = render::Job::Model<BeginSelfieFrame, Config>;

    void configure(const Config& config) {
        qDebug() << "FIXME pos" << config.position << "orient" << config.orientation;
        _position = config.position;
        _orientation = config.orientation;
    }

    void run(const render::RenderContextPointer& renderContext) {
        auto args = renderContext->args;
        auto textureCache = DependencyManager::get<TextureCache>();
        auto destFramebuffer = textureCache->getSelfieFramebuffer();
        // Why don't we have to reset these values? Is it because we happen to be last in the pipeline (which would be a bug waiting to happen)?
        args->_blitFramebuffer = destFramebuffer;
        args->_viewport = glm::ivec4(0, 0, destFramebuffer->getWidth(), destFramebuffer->getHeight());

        auto srcViewFrustum = args->getViewFrustum();
        srcViewFrustum.setPosition(_position);
        srcViewFrustum.setOrientation(_orientation);
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