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
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <GLMHelpers.h>
#include <plugins/PluginContainer.h>
#include <gpu/GLBackend.h>
#include <CursorManager.h>
#include <gl/GLWidget.h>
#include <shared/NsightHelpers.h>

#include "../Logging.h"
#include "../CompositorHelper.h"

static const QString MONO_PREVIEW = "Mono Preview";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const bool DEFAULT_MONO_VIEW = true;

glm::uvec2 HmdDisplayPlugin::getRecommendedUiSize() const {
    return CompositorHelper::VIRTUAL_SCREEN_SIZE;
}

void HmdDisplayPlugin::internalActivate() {
    _monoPreview = _container->getBoolSetting("monoPreview", DEFAULT_MONO_VIEW);

    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), MONO_PREVIEW,
        [this](bool clicked) {
        _monoPreview = clicked;
        _container->setBoolSetting("monoPreview", _monoPreview);
    }, true, _monoPreview);
    _container->removeMenu(FRAMERATE);
    Parent::internalActivate();
}

void HmdDisplayPlugin::customizeContext() {
    Parent::customizeContext();
    // Only enable mirroring if we know vsync is disabled
    enableVsync(false);
    _enablePreview = !isVsyncEnabled();
    _sphereSection = loadSphereSection(_program, CompositorHelper::VIRTUAL_UI_TARGET_FOV.y, CompositorHelper::VIRTUAL_UI_ASPECT_RATIO);
}

void HmdDisplayPlugin::uncustomizeContext() {
    _sphereSection.reset();
    _compositeFramebuffer.reset();
    Parent::uncustomizeContext();
}

void HmdDisplayPlugin::compositeOverlay() {
    using namespace oglplus;
    auto compositorHelper = DependencyManager::get<CompositorHelper>();

    // check the alpha
    auto overlayAlpha = compositorHelper->getAlpha();
    if (overlayAlpha > 0.0f) {
        // set the alpha
        Uniform<float>(*_program, _alphaUniform).Set(overlayAlpha);

        _sphereSection->Use();
        for_each_eye([&](Eye eye) {
            eyeViewport(eye);
            auto modelView = glm::inverse(_currentRenderEyePoses[eye]); // *glm::translate(mat4(), vec3(0, 0, -1));
            auto mvp = _eyeProjections[eye] * modelView;
            Uniform<glm::mat4>(*_program, _mvpUniform).Set(mvp);
            _sphereSection->Draw();
        });
    }
    Uniform<float>(*_program, _alphaUniform).Set(1.0);
}

void HmdDisplayPlugin::compositePointer() {
    using namespace oglplus;

    auto compositorHelper = DependencyManager::get<CompositorHelper>();

    // check the alpha
    auto overlayAlpha = compositorHelper->getAlpha();
    if (overlayAlpha > 0.0f) {
        // set the alpha
        Uniform<float>(*_program, _alphaUniform).Set(overlayAlpha);

        // Mouse pointer
        _plane->Use();
        // Reconstruct the headpose from the eye poses
        auto headPosition = (vec3(_currentRenderEyePoses[Left][3]) + vec3(_currentRenderEyePoses[Right][3])) / 2.0f;
        for_each_eye([&](Eye eye) {
            eyeViewport(eye);
            auto reticleTransform = compositorHelper->getReticleTransform(_currentRenderEyePoses[eye], headPosition);
            auto mvp = _eyeProjections[eye] * reticleTransform;
            Uniform<glm::mat4>(*_program, _mvpUniform).Set(mvp);
            _plane->Draw();
        });
    }
    Uniform<float>(*_program, _alphaUniform).Set(1.0);
}

void HmdDisplayPlugin::internalPresent() {

    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

    // Composite together the scene, overlay and mouse cursor
    hmdPresent();

    // screen preview mirroring
    if (_enablePreview) {
        auto window = _container->getPrimaryWidget();
        auto windowSize = toGlm(window->size());
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
        
        using namespace oglplus;
        Context::Clear().ColorBuffer();
        auto sourceSize = _compositeFramebuffer->size;
        if (_monoPreview) {
            sourceSize.x /= 2;
        }
        _compositeFramebuffer->Bound(Framebuffer::Target::Read, [&] {
            Context::BlitFramebuffer(
                0, 0, sourceSize.x, sourceSize.y,
                targetViewportPosition.x, targetViewportPosition.y, 
                targetViewportPosition.x + targetViewportSize.x, targetViewportPosition.y + targetViewportSize.y,
                BufferSelectBit::ColorBuffer, BlitFilter::Nearest);
        });
        swapBuffers();
    }

    postPreview();
}

void HmdDisplayPlugin::setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) {
    Lock lock(_mutex);
    _renderEyePoses[frameIndex][eye] = pose;
}

void HmdDisplayPlugin::updateFrameData() {
    Parent::updateFrameData();
    Lock lock(_mutex);
    _currentRenderEyePoses = _renderEyePoses[_currentRenderFrameIndex];
}

glm::mat4 HmdDisplayPlugin::getHeadPose() const {
    return _headPoseCache.get();
}
