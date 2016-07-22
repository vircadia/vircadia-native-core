//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtGui/QWindow>
#include <QtCore/QTime>

#include <GLMHelpers.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gpu/Forward.h>

#include <DeferredLightingEffect.h>
#include <render/ShapePipeline.h>
#include <render/Context.h>

#define DEFERRED_LIGHTING

class TestWindow : public QWindow {
protected:
    QOpenGLContextWrapper _glContext;
    QSize _size;
    glm::mat4 _projectionMatrix;
    bool _aboutToQuit { false };

#ifdef DEFERRED_LIGHTING
    // Prepare the ShapePipelines
    render::ShapePlumberPointer _shapePlumber { std::make_shared<render::ShapePlumber>() };
    render::SceneContextPointer _sceneContext{ std::make_shared<render::SceneContext>() };
    render::RenderContextPointer _renderContext{ std::make_shared<render::RenderContext>() };
    gpu::PipelinePointer _opaquePipeline;
    model::LightPointer _light { std::make_shared<model::Light>() };

    GenerateDeferredFrameTransform _generateDeferredFrameTransform;
    MakeLightingModel _generateLightingModel;
    PreparePrimaryFramebuffer _preparePrimaryFramebuffer;

    PrepareDeferred::Inputs _prepareDeferredInputs;
    PrepareDeferred _prepareDeferred;
    PrepareDeferred::Outputs _prepareDeferredOutputs;

    RenderDeferred::Inputs _renderDeferredInputs;
    RenderDeferred _renderDeferred;

#endif

    RenderArgs* _renderArgs { new RenderArgs() };

    TestWindow();
    virtual void initGl();
    virtual void renderFrame() = 0;

private:
    void resizeWindow(const QSize& size);

    void beginFrame();
    void endFrame();
    void draw();
    void resizeEvent(QResizeEvent* ev) override;
};

