//
//  GraphicsEngine.cpp
//
//  Created by Sam Gateau on 29/6/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GraphicsEngine.h"

#include <shared/GlobalAppProperties.h>

#include "WorldBox.h"
#include "LODManager.h"

#include <GeometryCache.h>
#include <TextureCache.h>
#include <UpdateSceneTask.h>
#include <RenderViewTask.h>
#include <SecondaryCamera.h>

#include "RenderEventHandler.h"

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/gl/GLBackend.h>

GraphicsEngine::GraphicsEngine() {
}

GraphicsEngine::~GraphicsEngine() {
}

void GraphicsEngine::initializeGPU(GLWidget* glwidget) {

    // Build an offscreen GL context for the main thread.
    _offscreenContext = new OffscreenGLCanvas();
    _offscreenContext->setObjectName("MainThreadContext");
    _offscreenContext->create(glwidget->qglContext());
    if (!_offscreenContext->makeCurrent()) {
        qFatal("Unable to make offscreen context current");
    }
    _offscreenContext->doneCurrent();
    _offscreenContext->setThreadContext();

    _renderEventHandler = new RenderEventHandler(glwidget->qglContext());
    if (!_offscreenContext->makeCurrent()) {
        qFatal("Unable to make offscreen context current");
    }

    // Requires the window context, because that's what's used in the actual rendering
    // and the GPU backend will make things like the VAO which cannot be shared across
    // contexts
    glwidget->makeCurrent();
    gpu::Context::init<gpu::gl::GLBackend>();
    qApp->setProperty(hifi::properties::gl::MAKE_PROGRAM_CALLBACK,
        QVariant::fromValue((void*)(&gpu::gl::GLBackend::makeProgram)));
    glwidget->makeCurrent();
    _gpuContext = std::make_shared<gpu::Context>();

    DependencyManager::get<TextureCache>()->setGPUContext(_gpuContext);

    // Restore the default main thread context
    _offscreenContext->makeCurrent();
}

void GraphicsEngine::initializeRender(bool disableDeferred) {

    // Set up the render engine
    render::CullFunctor cullFunctor = LODManager::shouldRender;
    _renderEngine->addJob<UpdateSceneTask>("UpdateScene");
#ifndef Q_OS_ANDROID
    _renderEngine->addJob<SecondaryCameraRenderTask>("SecondaryCameraJob", cullFunctor, !disableDeferred);
#endif
    _renderEngine->addJob<RenderViewTask>("RenderMainView", cullFunctor, !disableDeferred, render::ItemKey::TAG_BITS_0, render::ItemKey::TAG_BITS_0);
    _renderEngine->load();
    _renderEngine->registerScene(_renderScene);

    // Now that OpenGL is initialized, we are sure we have a valid context and can create the various pipeline shaders with success.
    DependencyManager::get<GeometryCache>()->initializeShapePipelines();


}

void GraphicsEngine::startup() {


    static_cast<RenderEventHandler*>(_renderEventHandler)->resumeThread();
}

void GraphicsEngine::shutdown() {
    // The cleanup process enqueues the transactions but does not process them.  Calling this here will force the actual
    // removal of the items.
    // See https://highfidelity.fogbugz.com/f/cases/5328
    _renderScene->enqueueFrame(); // flush all the transactions
    _renderScene->processTransactionQueue(); // process and apply deletions

    _gpuContext->shutdown();


    // shutdown render engine
    _renderScene = nullptr;
    _renderEngine = nullptr;
}


void GraphicsEngine::render_runRenderFrame(RenderArgs* renderArgs) {
    PROFILE_RANGE(render, __FUNCTION__);
    PerformanceTimer perfTimer("render");
//    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::runRenderFrame()");

    // The pending changes collecting the changes here
    render::Transaction transaction;

     // this is not in use at all anymore
    //if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
        // render models...
      //  PerformanceTimer perfTimer("entities");
  //      PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
     //       "Application::runRenderFrame() ... entities...");

        //RenderArgs::DebugFlags renderDebugFlags = RenderArgs::RENDER_DEBUG_NONE;

        //renderArgs->_debugFlags = renderDebugFlags;
    //}

    // Make sure the WorldBox is in the scene
    // For the record, this one RenderItem is the first one we created and added to the scene.
    // We could move that code elsewhere but you know...
    if (!render::Item::isValidID(WorldBoxRenderData::_item)) {
        auto worldBoxRenderData = std::make_shared<WorldBoxRenderData>();
        auto worldBoxRenderPayload = std::make_shared<WorldBoxRenderData::Payload>(worldBoxRenderData);

        WorldBoxRenderData::_item = _renderScene->allocateID();

        transaction.resetItem(WorldBoxRenderData::_item, worldBoxRenderPayload);
        _renderScene->enqueueTransaction(transaction);
    }

    {
        // PerformanceTimer perfTimer("EngineRun");
        _renderEngine->getRenderContext()->args = renderArgs;
        _renderEngine->run();
    }
}

static const unsigned int THROTTLED_SIM_FRAMERATE = 15;
static const int THROTTLED_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / THROTTLED_SIM_FRAMERATE;




bool GraphicsEngine::shouldPaint() const {

    // Throttle if requested
    if ((static_cast<RenderEventHandler*>(_renderEventHandler)->_lastTimeRendered.elapsed() < THROTTLED_SIM_FRAME_PERIOD_MS)) {
        return false;
    }
    
    return true;
}

bool GraphicsEngine::checkPendingRenderEvent() {
    bool expected = false;
    return (_renderEventHandler && static_cast<RenderEventHandler*>(_renderEventHandler)->_pendingRenderEvent.compare_exchange_strong(expected, true));
}
