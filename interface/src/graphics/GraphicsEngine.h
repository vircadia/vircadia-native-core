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

#include <render/Engine.h>

#include <OctreeConstants.h>

class GraphicsEngine {
public:
    GraphicsEngine();
    ~GraphicsEngine();

    void initializeGPU(GLWidget*);
    void initializeRender(bool disableDeferred);
    void startup();
    void shutdown();

    render::ScenePointer getRenderScene() const { return _renderScene; }
    render::EnginePointer getRenderEngine() const { return _renderEngine; }
    gpu::ContextPointer getGPUContext() const { return _gpuContext; }

    // Same as the one in application
    bool shouldPaint() const;
    bool checkPendingRenderEvent();

private:
    // THread specific calls
    void render_runRenderFrame(RenderArgs* renderArgs);

protected:

    render::ScenePointer _renderScene{ new render::Scene(glm::vec3(-0.5f * (float)TREE_SCALE), (float)TREE_SCALE) };
    render::EnginePointer _renderEngine{ new render::RenderEngine() };

    gpu::ContextPointer _gpuContext; // initialized during window creation

    QObject* _renderEventHandler{ nullptr };
    friend class RenderEventHandler;

    OffscreenGLCanvas* _offscreenContext{ nullptr };

    friend class Application;
};

#endif // hifi_GraphicsEngine_h
