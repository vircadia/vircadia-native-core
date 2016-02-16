//
//  Created by Bradley Austin Davis on 2016/02/15
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "HmdDisplayPlugin.h"

#include <memory>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QLoggingCategory>
#include <QtWidgets/QWidget>

#include <GLMHelpers.h>
#include <plugins/PluginContainer.h>

Q_DECLARE_LOGGING_CATEGORY(displayplugins)
Q_LOGGING_CATEGORY(displayplugins, "hifi.displayplugins")

static const QString MONO_PREVIEW = "Mono Preview";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const bool DEFAULT_MONO_VIEW = true;

void HmdDisplayPlugin::activate() {
    _monoPreview = _container->getBoolSetting("monoPreview", DEFAULT_MONO_VIEW);

    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), MONO_PREVIEW,
        [this](bool clicked) {
        _monoPreview = clicked;
        _container->setBoolSetting("monoPreview", _monoPreview);
    }, true, _monoPreview);
    _container->removeMenu(FRAMERATE);
    WindowOpenGLDisplayPlugin::activate();
}

void HmdDisplayPlugin::deactivate() {
    WindowOpenGLDisplayPlugin::deactivate();
}

void HmdDisplayPlugin::customizeContext() {
    WindowOpenGLDisplayPlugin::customizeContext();
    // Only enable mirroring if we know vsync is disabled
    enableVsync(false);
    _enablePreview = !isVsyncEnabled();
}

void HmdDisplayPlugin::internalPresent() {
    // screen preview mirroring
    if (_enablePreview) {
        auto windowSize = toGlm(_window->size());
        float windowAspect = aspect(windowSize);
        float sceneAspect = aspect(_renderTargetSize);
        if (_monoPreview) {
            sceneAspect /= 2.0f;
        }
        float aspectRatio = sceneAspect / windowAspect;

        uvec2 targetViewportSize = windowSize;
        if (aspectRatio < 1.0f) {
            targetViewportSize.x *= aspectRatio;
        } else {
            targetViewportSize.y /= aspectRatio;
        }

        uvec2 targetViewportPosition;
        if (targetViewportSize.x < windowSize.x) {
            targetViewportPosition.x = (windowSize.x - targetViewportSize.x) / 2;
        } else if (targetViewportSize.y < windowSize.y) {
            targetViewportPosition.y = (windowSize.y - targetViewportSize.y) / 2;
        }
        
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(
            targetViewportPosition.x, targetViewportPosition.y,
            targetViewportSize.x * (_monoPreview ? 2 : 1), targetViewportSize.y);
        glEnable(GL_SCISSOR_TEST);
        glScissor(
            targetViewportPosition.x, targetViewportPosition.y,
            targetViewportSize.x, targetViewportSize.y);
        glBindTexture(GL_TEXTURE_2D, _currentSceneTexture);
        GLenum err = glGetError();
        Q_ASSERT(0 == err);
        drawUnitQuad();
        glDisable(GL_SCISSOR_TEST);
        swapBuffers();
    }
}


//// screen preview mirroring
//if (_enablePreview) {
//    auto windowSize = toGlm(_window->size());
//    if (_monoPreview) {
//        // Find the aspect ratio for one eye
//        auto eyeAspect = (float)(size.x / 2) / (float)size.y;
//        auto windowAspect = (float)windowSize.x / (float)windowSize.y;
//        if (eyeAspect < windowAspect) {
//            Context::Viewport(windowSize.x * 2, windowSize.y);
//            Context::Scissor(0, windowSize.y, windowSize.x, windowSize.y);
//        } else {
//
//        }
//    } else {
//        Context::Viewport(windowSize.x, windowSize.y);
//    }
//    glBindTexture(GL_TEXTURE_2D, _currentSceneTexture);
//    GLenum err = glGetError();
//    Q_ASSERT(0 == err);
//    drawUnitQuad();
//}
///*
//The swapbuffer call here is only required if we want to mirror the content to the screen.
//However, it should only be done if we can reliably disable v-sync on the mirror surface,
//otherwise the swapbuffer delay will interefere with the framerate of the headset
//*/
//if (_enablePreview) {
//    swapBuffers();
//}
