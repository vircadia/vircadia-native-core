//
//  Application_render.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include <MainWindow.h>

#include <display-plugins/CompositorHelper.h>
#include <FramebufferCache.h>
#include "ui/Stats.h"
#include "FrameTimingsScriptingInterface.h"

// Statically provided display and input plugins
extern DisplayPluginList getDisplayPlugins();

void Application::editRenderArgs(RenderArgsEditor editor) {
    QMutexLocker renderLocker(&_renderArgsMutex);
    editor(_appRenderArgs);

}

void Application::paintGL() {
    // Some plugins process message events, allowing paintGL to be called reentrantly.
    if (_aboutToQuit || _window->isMinimized()) {
        return;
    }

    _frameCount++;
    _lastTimeRendered.start();

    auto lastPaintBegin = usecTimestampNow();
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff0000ff, (uint64_t)_frameCount);
    PerformanceTimer perfTimer("paintGL");

    if (nullptr == _displayPlugin) {
        return;
    }

    DisplayPluginPointer displayPlugin;
    {
        PROFILE_RANGE(render, "/getActiveDisplayPlugin");
        displayPlugin = getActiveDisplayPlugin();
    }

    {
        PROFILE_RANGE(render, "/pluginBeginFrameRender");
        // If a display plugin loses it's underlying support, it
        // needs to be able to signal us to not use it
        if (!displayPlugin->beginFrameRender(_frameCount)) {
            updateDisplayMode();
            return;
        }
    }

    // update the avatar with a fresh HMD pose
    //   {
    //      PROFILE_RANGE(render, "/updateAvatar");
    //       getMyAvatar()->updateFromHMDSensorMatrix(getHMDSensorPose());
    //   }

    //  auto lodManager = DependencyManager::get<LODManager>();

    RenderArgs renderArgs;
    float sensorToWorldScale;
    glm::mat4  HMDSensorPose;
    glm::mat4  eyeToWorld;
    glm::mat4  sensorToWorld;

    bool isStereo;
    glm::mat4  stereoEyeOffsets[2];
    glm::mat4  stereoEyeProjections[2];

    {
        QMutexLocker viewLocker(&_renderArgsMutex);
        renderArgs = _appRenderArgs._renderArgs;
        HMDSensorPose = _appRenderArgs._headPose;
        eyeToWorld = _appRenderArgs._eyeToWorld;
        sensorToWorld = _appRenderArgs._sensorToWorld;
        sensorToWorldScale = _appRenderArgs._sensorToWorldScale;
        isStereo = _appRenderArgs._isStereo;
        for_each_eye([&](Eye eye) {
            stereoEyeOffsets[eye] = _appRenderArgs._eyeOffsets[eye];
            stereoEyeProjections[eye] = _appRenderArgs._eyeProjections[eye];
        });
    }

    //float sensorToWorldScale = getMyAvatar()->getSensorToWorldScale();
    //{
    //    PROFILE_RANGE(render, "/buildFrustrumAndArgs");
    //    {
    //        QMutexLocker viewLocker(&_viewMutex);
    //        // adjust near clip plane to account for sensor scaling.
    //        auto adjustedProjection = glm::perspective(_viewFrustum.getFieldOfView(),
    //                                                   _viewFrustum.getAspectRatio(),
    //                                                   DEFAULT_NEAR_CLIP * sensorToWorldScale,
    //                                                   _viewFrustum.getFarClip());
    //        _viewFrustum.setProjection(adjustedProjection);
    //        _viewFrustum.calculate();
    //    }
    //    renderArgs = RenderArgs(_gpuContext, lodManager->getOctreeSizeScale(),
    //        lodManager->getBoundaryLevelAdjust(), RenderArgs::DEFAULT_RENDER_MODE,
    //        RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);
    //    {
    //        QMutexLocker viewLocker(&_viewMutex);
    //        renderArgs.setViewFrustum(_viewFrustum);
    //    }
    //}

    //{
    //    PROFILE_RANGE(render, "/resizeGL");
    //    PerformanceWarning::setSuppressShortTimings(Menu::getInstance()->isOptionChecked(MenuOption::SuppressShortTimings));
    //    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    //    PerformanceWarning warn(showWarnings, "Application::paintGL()");
    //    resizeGL();
    //}

    {
        PROFILE_RANGE(render, "/gpuContextReset");
        //   _gpuContext->beginFrame(getHMDSensorPose());
        _gpuContext->beginFrame(HMDSensorPose);
        // Reset the gpu::Context Stages
        // Back to the default framebuffer;
        gpu::doInBatch(_gpuContext, [&](gpu::Batch& batch) {
            batch.resetStages();
        });
    }


    {
        PROFILE_RANGE(render, "/renderOverlay");
        PerformanceTimer perfTimer("renderOverlay");
        // NOTE: There is no batch associated with this renderArgs
        // the ApplicationOverlay class assumes it's viewport is setup to be the device size
        QSize size = getDeviceSize();
        renderArgs._viewport = glm::ivec4(0, 0, size.width(), size.height());
        _applicationOverlay.renderOverlay(&renderArgs);
    }

    //   updateCamera(renderArgs);
    {
        PROFILE_RANGE(render, "/updateCompositor");
        //    getApplicationCompositor().setFrameInfo(_frameCount, _myCamera.getTransform(), getMyAvatar()->getSensorToWorldMatrix());
        getApplicationCompositor().setFrameInfo(_frameCount, eyeToWorld, sensorToWorld);
    }

    gpu::FramebufferPointer finalFramebuffer;
    QSize finalFramebufferSize;
    {
        PROFILE_RANGE(render, "/getOutputFramebuffer");
        // Primary rendering pass
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        finalFramebufferSize = framebufferCache->getFrameBufferSize();
        // Final framebuffer that will be handled to the display-plugin
        finalFramebuffer = framebufferCache->getFramebuffer();
    }

    //auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
    //float ipdScale = hmdInterface->getIPDScale();

    //// scale IPD by sensorToWorldScale, to make the world seem larger or smaller accordingly.
    //ipdScale *= sensorToWorldScale;

    {
        //PROFILE_RANGE(render, "/mainRender");
        //PerformanceTimer perfTimer("mainRender");
        //// FIXME is this ever going to be different from the size previously set in the render args
        //// in the overlay render?
        //// Viewport is assigned to the size of the framebuffer
        //renderArgs._viewport = ivec4(0, 0, finalFramebufferSize.width(), finalFramebufferSize.height());
        //auto baseProjection = renderArgs.getViewFrustum().getProjection();
        //if (displayPlugin->isStereo()) {
        //    // Stereo modes will typically have a larger projection matrix overall,
        //    // so we ask for the 'mono' projection matrix, which for stereo and HMD
        //    // plugins will imply the combined projection for both eyes.
        //    //
        //    // This is properly implemented for the Oculus plugins, but for OpenVR
        //    // and Stereo displays I'm not sure how to get / calculate it, so we're
        //    // just relying on the left FOV in each case and hoping that the
        //    // overall culling margin of error doesn't cause popping in the
        //    // right eye.  There are FIXMEs in the relevant plugins
        //    _myCamera.setProjection(displayPlugin->getCullingProjection(baseProjection));
        //    renderArgs._context->enableStereo(true);
        //    mat4 eyeOffsets[2];
        //    mat4 eyeProjections[2];

        //    // FIXME we probably don't need to set the projection matrix every frame,
        //    // only when the display plugin changes (or in non-HMD modes when the user
        //    // changes the FOV manually, which right now I don't think they can.
        //    for_each_eye([&](Eye eye) {
        //        // For providing the stereo eye views, the HMD head pose has already been
        //        // applied to the avatar, so we need to get the difference between the head
        //        // pose applied to the avatar and the per eye pose, and use THAT as
        //        // the per-eye stereo matrix adjustment.
        //        mat4 eyeToHead = displayPlugin->getEyeToHeadTransform(eye);
        //        // Grab the translation
        //        vec3 eyeOffset = glm::vec3(eyeToHead[3]);
        //        // Apply IPD scaling
        //        mat4 eyeOffsetTransform = glm::translate(mat4(), eyeOffset * -1.0f * ipdScale);
        //        eyeOffsets[eye] = eyeOffsetTransform;
        //        eyeProjections[eye] = displayPlugin->getEyeProjection(eye, baseProjection);
        //    });
        //    renderArgs._context->setStereoProjections(eyeProjections);
        //    renderArgs._context->setStereoViews(eyeOffsets);

        //    // Configure the type of display / stereo
        //    renderArgs._displayMode = (isHMDMode() ? RenderArgs::STEREO_HMD : RenderArgs::STEREO_MONITOR);
        //}

        if (isStereo) {
            renderArgs._context->enableStereo(true);
            renderArgs._context->setStereoProjections(stereoEyeProjections);
            renderArgs._context->setStereoViews(stereoEyeOffsets);
            //         renderArgs._displayMode
        }

        renderArgs._blitFramebuffer = finalFramebuffer;
        // displaySide(&renderArgs, _myCamera);
        runRenderFrame(&renderArgs);
    }

    gpu::Batch postCompositeBatch;
    {
        PROFILE_RANGE(render, "/postComposite");
        PerformanceTimer perfTimer("postComposite");
        renderArgs._batch = &postCompositeBatch;
        renderArgs._batch->setViewportTransform(ivec4(0, 0, finalFramebufferSize.width(), finalFramebufferSize.height()));
        renderArgs._batch->setViewTransform(renderArgs.getViewFrustum().getView());
        _overlays.render3DHUDOverlays(&renderArgs);
    }

    auto frame = _gpuContext->endFrame();
    frame->frameIndex = _frameCount;
    frame->framebuffer = finalFramebuffer;
    frame->framebufferRecycler = [](const gpu::FramebufferPointer& framebuffer) {
        DependencyManager::get<FramebufferCache>()->releaseFramebuffer(framebuffer);
    };
    frame->overlay = _applicationOverlay.getOverlayTexture();
    frame->postCompositeBatch = postCompositeBatch;
    // deliver final scene rendering commands to the display plugin
    {
        PROFILE_RANGE(render, "/pluginOutput");
        PerformanceTimer perfTimer("pluginOutput");
        _frameCounter.increment();
        displayPlugin->submitFrame(frame);
    }

    // Reset the framebuffer and stereo state
    renderArgs._blitFramebuffer.reset();
    renderArgs._context->enableStereo(false);

    {
        Stats::getInstance()->setRenderDetails(renderArgs._details);
    }

    uint64_t lastPaintDuration = usecTimestampNow() - lastPaintBegin;
    _frameTimingsScriptingInterface.addValue(lastPaintDuration);
}


// WorldBox Render Data & rendering functions

class WorldBoxRenderData {
public:
    typedef render::Payload<WorldBoxRenderData> Payload;
    typedef Payload::DataPointer Pointer;

    int _val = 0;
    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID WorldBoxRenderData::_item{ render::Item::INVALID_ITEM_ID };

namespace render {
    template <> const ItemKey payloadGetKey(const WorldBoxRenderData::Pointer& stuff) { return ItemKey::Builder::opaqueShape(); }
    template <> const Item::Bound payloadGetBound(const WorldBoxRenderData::Pointer& stuff) { return Item::Bound(); }
    template <> void payloadRender(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args) {
        if (Menu::getInstance()->isOptionChecked(MenuOption::WorldAxes)) {
            PerformanceTimer perfTimer("worldBox");

            auto& batch = *args->_batch;
            DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch);
            renderWorldBox(args, batch);
        }
    }
}

void Application::runRenderFrame(RenderArgs* renderArgs) {

    // FIXME: This preDisplayRender call is temporary until we create a separate render::scene for the mirror rendering.
    // Then we can move this logic into the Avatar::simulate call.
    //   auto myAvatar = getMyAvatar();
    //   myAvatar->preDisplaySide(renderArgs);

    PROFILE_RANGE(render, __FUNCTION__);
    PerformanceTimer perfTimer("display");
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::runRenderFrame()");

    // load the view frustum
    //  {
    //     QMutexLocker viewLocker(&_viewMutex);
    //     theCamera.loadViewFrustum(_displayViewFrustum);
    // }

    // The pending changes collecting the changes here
    render::Transaction transaction;

    // Assuming nothing gets rendered through that
    //if (!selfAvatarOnly) {
    {
        if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
            // render models...
            PerformanceTimer perfTimer("entities");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::runRenderFrame() ... entities...");

            RenderArgs::DebugFlags renderDebugFlags = RenderArgs::RENDER_DEBUG_NONE;

            if (Menu::getInstance()->isOptionChecked(MenuOption::PhysicsShowHulls)) {
                renderDebugFlags = static_cast<RenderArgs::DebugFlags>(renderDebugFlags |
                    static_cast<int>(RenderArgs::RENDER_DEBUG_HULLS));
            }
            renderArgs->_debugFlags = renderDebugFlags;
        }
    }

    // FIXME: Move this out of here!, WorldBox should be driven by the entity content just like the other entities
    // Make sure the WorldBox is in the scene
    if (!render::Item::isValidID(WorldBoxRenderData::_item)) {
        auto worldBoxRenderData = make_shared<WorldBoxRenderData>();
        auto worldBoxRenderPayload = make_shared<WorldBoxRenderData::Payload>(worldBoxRenderData);

        WorldBoxRenderData::_item = _main3DScene->allocateID();

        transaction.resetItem(WorldBoxRenderData::_item, worldBoxRenderPayload);
        _main3DScene->enqueueTransaction(transaction);
    }


    // For now every frame pass the renderContext
    {
        PerformanceTimer perfTimer("EngineRun");

        /*   {
        QMutexLocker viewLocker(&_viewMutex);
        renderArgs->setViewFrustum(_displayViewFrustum);
        }*/
        //   renderArgs->_cameraMode = (int8_t)theCamera.getMode(); // HACK
        renderArgs->_scene = getMain3DScene();
        _renderEngine->getRenderContext()->args = renderArgs;

        // Before the deferred pass, let's try to use the render engine
        _renderEngine->run();
    }
}
