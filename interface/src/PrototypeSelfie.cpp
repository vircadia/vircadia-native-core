
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
    using JobModel = render::Job::ModelO<BeginSelfieFrame, RenderArgsPointer, Config>;

    void configure(const Config& config) {
        //qDebug() << "FIXME pos" << config.position << "orient" << config.orientation;
        _position = config.position;
        _orientation = config.orientation;
    }

    void run(const render::RenderContextPointer& renderContext, RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;
        auto textureCache = DependencyManager::get<TextureCache>();
        auto destFramebuffer = textureCache->getSelfieFramebuffer();
        // Caching/restoring the old values doesn't seem to be needed. Is it because we happen to be last in the pipeline (which would be a bug waiting to happen)?
        _cachedArgs._blitFramebuffer = args->_blitFramebuffer;
        _cachedArgs._viewport = args->_viewport;
        args->_blitFramebuffer = destFramebuffer;
        args->_viewport = glm::ivec4(0, 0, destFramebuffer->getWidth(), destFramebuffer->getHeight());
        // FIXME: We're also going to need to clear/restore the stereo setup!

        auto srcViewFrustum = args->getViewFrustum();
        srcViewFrustum.setPosition(_position);
        srcViewFrustum.setOrientation(_orientation);
        //srcViewFrustum.calculate(); // do we need this? I don't think so
        //qDebug() << "FIXME pos" << _position << "orient" << _orientation << "frust pos" << srcViewFrustum.getPosition() << "orient" << srcViewFrustum.getOrientation() << "direct" << srcViewFrustum.getDirection();
        args->pushViewFrustum(srcViewFrustum);
        cachedArgs = std::make_shared<RenderArgs>(_cachedArgs);
    }

protected:
    RenderArgs _cachedArgs;
};

class EndSelfieFrame {
public:
    using JobModel = render::Job::ModelI<EndSelfieFrame, RenderArgsPointer>;

    void run(const render::RenderContextPointer& renderContext, const RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;
        args->_blitFramebuffer = cachedArgs->_blitFramebuffer;
        args->_viewport = cachedArgs->_viewport;
        args->popViewFrustum();
    }
};

void SelfieRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor) {
    const auto cachedArg = task.addJob<BeginSelfieFrame>("BeginSelfie");
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
    task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    task.addJob<EndSelfieFrame>("EndSelfie", cachedArg);
}