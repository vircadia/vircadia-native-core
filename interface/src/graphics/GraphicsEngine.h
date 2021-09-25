//
//  GraphicsEngine.h
//
//  Created by Sam Gateau on 29/6/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_GraphicsEngine_h
#define hifi_GraphicsEngine_h

#include <gl/OffscreenGLCanvas.h>
#include <gl/GLWidget.h>
#include <qmutex.h>

#include <render/Engine.h>
#include <procedural/ProceduralSkybox.h>

#include <OctreeConstants.h>
#include <shared/RateCounter.h>

#include "FrameTimingsScriptingInterface.h"


struct AppRenderArgs {
    render::Args _renderArgs;
    glm::mat4 _eyeToWorld;
    glm::mat4 _view;
    glm::mat4 _eyeOffsets[2];
    glm::mat4 _eyeProjections[2];
    glm::mat4 _headPose;
    glm::mat4 _sensorToWorld;
    float _sensorToWorldScale{ 1.0f };
    bool _isStereo{ false };
};

using RenderArgsEditor = std::function <void(AppRenderArgs&)>;


class GraphicsEngine {
public:
    GraphicsEngine();
    ~GraphicsEngine();

    void initializeGPU(GLWidget*);
    void initializeRender();
    void startup();
    void shutdown();

    render::ScenePointer getRenderScene() const { return _renderScene; }
    render::EnginePointer getRenderEngine() const { return _renderEngine; }
    gpu::ContextPointer getGPUContext() const { return _gpuContext; }

    // Same as the one in application
    bool shouldPaint() const;
    bool checkPendingRenderEvent();

    size_t getRenderFrameCount() const { return _renderFrameCount; }
    float getRenderLoopRate() const { return _renderLoopCounter.rate(); }

    // Feed GRaphics Engine with new frame configuration
    void editRenderArgs(RenderArgsEditor editor);

private:
    // Thread specific calls
    void render_performFrame();
    void render_runRenderFrame(RenderArgs* renderArgs);

protected:

    mutable QRecursiveMutex _renderArgsMutex;
    AppRenderArgs _appRenderArgs;

    RateCounter<500> _renderLoopCounter;

    uint32_t _renderFrameCount{ 0 };
    render::ScenePointer _renderScene{ new render::Scene(glm::vec3(-0.5f * (float)TREE_SCALE), (float)TREE_SCALE) };
    render::EnginePointer _renderEngine{ new render::RenderEngine() };

    gpu::ContextPointer _gpuContext; // initialized during window creation

    QObject* _renderEventHandler{ nullptr };
    friend class RenderEventHandler;

    FrameTimingsScriptingInterface _frameTimingsScriptingInterface;

    std::shared_ptr<ProceduralSkybox> _splashScreen { std::make_shared<ProceduralSkybox>() };
#ifndef Q_OS_ANDROID
    std::atomic<bool> _programsCompiled { false };
#else
    std::atomic<bool> _programsCompiled { true };
#endif

    friend class Application;
};

#endif // hifi_GraphicsEngine_h
