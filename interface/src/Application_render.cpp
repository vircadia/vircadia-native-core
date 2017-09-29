//
//  Application_render.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef tryingSOmething

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
    {
        QMutexLocker viewLocker(&_renderArgsMutex);
        renderArgs = _appRenderArgs._renderArgs;
        HMDSensorPose = _appRenderArgs._eyeToWorld;
        sensorToWorldScale = _appRenderArgs._sensorToWorldScale;
    }
    /*
    float sensorToWorldScale = getMyAvatar()->getSensorToWorldScale();
    {
    PROFILE_RANGE(render, "/buildFrustrumAndArgs");
    {
    QMutexLocker viewLocker(&_viewMutex);
    // adjust near clip plane to account for sensor scaling.
    auto adjustedProjection = glm::perspective(_viewFrustum.getFieldOfView(),
    _viewFrustum.getAspectRatio(),
    DEFAULT_NEAR_CLIP * sensorToWorldScale,
    _viewFrustum.getFarClip());
    _viewFrustum.setProjection(adjustedProjection);
    _viewFrustum.calculate();
    }
    renderArgs = RenderArgs(_gpuContext, lodManager->getOctreeSizeScale(),
    lodManager->getBoundaryLevelAdjust(), RenderArgs::DEFAULT_RENDER_MODE,
    RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);
    {
    QMutexLocker viewLocker(&_viewMutex);
    renderArgs.setViewFrustum(_viewFrustum);
    }
    }
    */
    {
        PROFILE_RANGE(render, "/resizeGL");
        PerformanceWarning::setSuppressShortTimings(Menu::getInstance()->isOptionChecked(MenuOption::SuppressShortTimings));
        bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
        PerformanceWarning warn(showWarnings, "Application::paintGL()");
        resizeGL();
    }

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

    /* glm::vec3 boomOffset;
    {
    PROFILE_RANGE(render, "/updateCamera");
    {
    PerformanceTimer perfTimer("CameraUpdates");

    auto myAvatar = getMyAvatar();
    boomOffset = myAvatar->getModelScale() * myAvatar->getBoomLength() * -IDENTITY_FORWARD;

    // The render mode is default or mirror if the camera is in mirror mode, assigned further below
    renderArgs._renderMode = RenderArgs::DEFAULT_RENDER_MODE;

    // Always use the default eye position, not the actual head eye position.
    // Using the latter will cause the camera to wobble with idle animations,
    // or with changes from the face tracker
    if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
    if (isHMDMode()) {
    mat4 camMat = myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
    _myCamera.setPosition(extractTranslation(camMat));
    _myCamera.setOrientation(glmExtractRotation(camMat));
    } else {
    _myCamera.setPosition(myAvatar->getDefaultEyePosition());
    _myCamera.setOrientation(myAvatar->getMyHead()->getHeadOrientation());
    }
    } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
    if (isHMDMode()) {
    auto hmdWorldMat = myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
    _myCamera.setOrientation(glm::normalize(glmExtractRotation(hmdWorldMat)));
    _myCamera.setPosition(extractTranslation(hmdWorldMat) +
    myAvatar->getOrientation() * boomOffset);
    } else {
    _myCamera.setOrientation(myAvatar->getHead()->getOrientation());
    if (Menu::getInstance()->isOptionChecked(MenuOption::CenterPlayerInView)) {
    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
    + _myCamera.getOrientation() * boomOffset);
    } else {
    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
    + myAvatar->getOrientation() * boomOffset);
    }
    }
    } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
    if (isHMDMode()) {
    auto mirrorBodyOrientation = myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f));

    glm::quat hmdRotation = extractRotation(myAvatar->getHMDSensorMatrix());
    // Mirror HMD yaw and roll
    glm::vec3 mirrorHmdEulers = glm::eulerAngles(hmdRotation);
    mirrorHmdEulers.y = -mirrorHmdEulers.y;
    mirrorHmdEulers.z = -mirrorHmdEulers.z;
    glm::quat mirrorHmdRotation = glm::quat(mirrorHmdEulers);

    glm::quat worldMirrorRotation = mirrorBodyOrientation * mirrorHmdRotation;

    _myCamera.setOrientation(worldMirrorRotation);

    glm::vec3 hmdOffset = extractTranslation(myAvatar->getHMDSensorMatrix());
    // Mirror HMD lateral offsets
    hmdOffset.x = -hmdOffset.x;

    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
    + glm::vec3(0, _raiseMirror * myAvatar->getModelScale(), 0)
    + mirrorBodyOrientation * glm::vec3(0.0f, 0.0f, 1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror
    + mirrorBodyOrientation * hmdOffset);
    } else {
    _myCamera.setOrientation(myAvatar->getOrientation()
    * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)));
    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
    + glm::vec3(0, _raiseMirror * myAvatar->getModelScale(), 0)
    + (myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, _rotateMirror, 0.0f))) *
    glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror);
    }
    renderArgs._renderMode = RenderArgs::MIRROR_RENDER_MODE;
    } else if (_myCamera.getMode() == CAMERA_MODE_ENTITY) {
    EntityItemPointer cameraEntity = _myCamera.getCameraEntityPointer();
    if (cameraEntity != nullptr) {
    if (isHMDMode()) {
    glm::quat hmdRotation = extractRotation(myAvatar->getHMDSensorMatrix());
    _myCamera.setOrientation(cameraEntity->getRotation() * hmdRotation);
    glm::vec3 hmdOffset = extractTranslation(myAvatar->getHMDSensorMatrix());
    _myCamera.setPosition(cameraEntity->getPosition() + (hmdRotation * hmdOffset));
    } else {
    _myCamera.setOrientation(cameraEntity->getRotation());
    _myCamera.setPosition(cameraEntity->getPosition());
    }
    }
    }
    // Update camera position
    if (!isHMDMode()) {
    _myCamera.update(1.0f / _frameCounter.rate());
    }
    }
    }
    */
    {
        PROFILE_RANGE(render, "/updateCompositor");
        getApplicationCompositor().setFrameInfo(_frameCount, _myCamera.getTransform(), getMyAvatar()->getSensorToWorldMatrix());
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

    auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
    float ipdScale = hmdInterface->getIPDScale();

    // scale IPD by sensorToWorldScale, to make the world seem larger or smaller accordingly.
    ipdScale *= sensorToWorldScale;

    {
        PROFILE_RANGE(render, "/mainRender");
        PerformanceTimer perfTimer("mainRender");
        // FIXME is this ever going to be different from the size previously set in the render args
        // in the overlay render?
        // Viewport is assigned to the size of the framebuffer
        renderArgs._viewport = ivec4(0, 0, finalFramebufferSize.width(), finalFramebufferSize.height());
        auto baseProjection = renderArgs.getViewFrustum().getProjection();
        if (displayPlugin->isStereo()) {
            // Stereo modes will typically have a larger projection matrix overall,
            // so we ask for the 'mono' projection matrix, which for stereo and HMD
            // plugins will imply the combined projection for both eyes.
            //
            // This is properly implemented for the Oculus plugins, but for OpenVR
            // and Stereo displays I'm not sure how to get / calculate it, so we're
            // just relying on the left FOV in each case and hoping that the
            // overall culling margin of error doesn't cause popping in the
            // right eye.  There are FIXMEs in the relevant plugins
            _myCamera.setProjection(displayPlugin->getCullingProjection(baseProjection));
            renderArgs._context->enableStereo(true);
            mat4 eyeOffsets[2];
            mat4 eyeProjections[2];

            // FIXME we probably don't need to set the projection matrix every frame,
            // only when the display plugin changes (or in non-HMD modes when the user
            // changes the FOV manually, which right now I don't think they can.
            for_each_eye([&](Eye eye) {
                // For providing the stereo eye views, the HMD head pose has already been
                // applied to the avatar, so we need to get the difference between the head
                // pose applied to the avatar and the per eye pose, and use THAT as
                // the per-eye stereo matrix adjustment.
                mat4 eyeToHead = displayPlugin->getEyeToHeadTransform(eye);
                // Grab the translation
                vec3 eyeOffset = glm::vec3(eyeToHead[3]);
                // Apply IPD scaling
                mat4 eyeOffsetTransform = glm::translate(mat4(), eyeOffset * -1.0f * ipdScale);
                eyeOffsets[eye] = eyeOffsetTransform;
                eyeProjections[eye] = displayPlugin->getEyeProjection(eye, baseProjection);
            });
            renderArgs._context->setStereoProjections(eyeProjections);
            renderArgs._context->setStereoViews(eyeOffsets);

            // Configure the type of display / stereo
            renderArgs._displayMode = (isHMDMode() ? RenderArgs::STEREO_HMD : RenderArgs::STEREO_MONITOR);
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
#endif