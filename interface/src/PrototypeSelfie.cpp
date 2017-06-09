
#include "PrototypeSelfie.h"

#include <gpu/Context.h>

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

void SelfieRenderTaskConfig::resetSize(int width, int height) { // Carefully adjust the framebuffer / texture.
    bool wasEnabled = isEnabled();
    setEnabled(false);
    auto textureCache = DependencyManager::get<TextureCache>();
    textureCache->resetSelfieFramebuffer(width, height);
    setEnabled(wasEnabled);
}

class BeginSelfieFrame {  // Changes renderContext for our framebuffer and and view.
    glm::vec3 _position{};
    glm::quat _orientation{};
public:
    using Config = BeginSelfieFrameConfig;
    using JobModel = render::Job::ModelO<BeginSelfieFrame, RenderArgsPointer, Config>;
    BeginSelfieFrame() {
        _cachedArgsPointer = std::make_shared<RenderArgs>(_cachedArgs);
    }

    void configure(const Config& config) {
        // Why does this run all the time, even when not enabled? Should we check and bail?
        //qDebug() << "FIXME pos" << config.position << "orient" << config.orientation;
        _position = config.position;
        _orientation = config.orientation;
    }

    void run(const render::RenderContextPointer& renderContext, RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;
        auto textureCache = DependencyManager::get<TextureCache>();
        auto destFramebuffer = textureCache->getSelfieFramebuffer();
        // Caching/restoring the old values doesn't seem to be needed. Is it because we happen to be last in the pipeline (which would be a bug waiting to happen)?
        _cachedArgsPointer->_blitFramebuffer = args->_blitFramebuffer;
        _cachedArgsPointer->_viewport = args->_viewport;
        _cachedArgsPointer->_displayMode = args->_displayMode;
        args->_blitFramebuffer = destFramebuffer;
        args->_viewport = glm::ivec4(0, 0, destFramebuffer->getWidth(), destFramebuffer->getHeight());
        args->_viewport = glm::ivec4(0, 0, destFramebuffer->getWidth(), destFramebuffer->getHeight());
        args->_displayMode = RenderArgs::MONO;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.disableContextStereo();
        });

        auto srcViewFrustum = args->getViewFrustum();
        srcViewFrustum.setPosition(_position);
        srcViewFrustum.setOrientation(_orientation);
        //srcViewFrustum.calculate(); // do we need this? I don't think so
        //qDebug() << "FIXME pos" << _position << "orient" << _orientation << "frust pos" << srcViewFrustum.getPosition() << "orient" << srcViewFrustum.getOrientation() << "direct" << srcViewFrustum.getDirection();
        args->pushViewFrustum(srcViewFrustum);
        cachedArgs = _cachedArgsPointer;
    }

protected:
    RenderArgs _cachedArgs;
    RenderArgsPointer _cachedArgsPointer;
};

class EndSelfieFrame {  // Restores renderContext.
public:
    using JobModel = render::Job::ModelI<EndSelfieFrame, RenderArgsPointer>;

    void run(const render::RenderContextPointer& renderContext, const RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;
        args->_blitFramebuffer = cachedArgs->_blitFramebuffer;
        args->_viewport = cachedArgs->_viewport;
        args->popViewFrustum();
        args->_displayMode = cachedArgs->_displayMode;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.restoreContextStereo();
        });
    }
};

void SelfieRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor) {
    const auto cachedArg = task.addJob<BeginSelfieFrame>("BeginSelfie");
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
    task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    task.addJob<EndSelfieFrame>("EndSelfie", cachedArg);
}