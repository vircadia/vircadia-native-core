//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Basic2DWindowOpenGLDisplayPlugin.h"
#include "CompositorHelper.h"
#include "VirtualPadManager.h"

#include <mutex>

#include <QScreen>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QAction>

#include <ui-plugins/PluginContainer.h>
#include <PathUtils.h>

const QString Basic2DWindowOpenGLDisplayPlugin::NAME("Desktop");

static const QString FULLSCREEN = "Fullscreen";

void Basic2DWindowOpenGLDisplayPlugin::customizeContext() {
#if defined(Q_OS_ANDROID)
    qreal dpi = getFullscreenTarget()->physicalDotsPerInch();
    _virtualPadPixelSize = dpi * VirtualPad::Manager::BASE_DIAMETER_PIXELS / VirtualPad::Manager::DPI;

    if (!_virtualPadStickTexture) {
        auto iconPath = PathUtils::resourcesPath() + "images/analog_stick.png";
        auto image = QImage(iconPath);
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
        if ((image.width() > 0) && (image.height() > 0)) {
            image = image.scaled(_virtualPadPixelSize, _virtualPadPixelSize, Qt::KeepAspectRatio);

            _virtualPadStickTexture = gpu::Texture::createStrict(
                    gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA),
                    image.width(), image.height(),
                    gpu::Texture::MAX_NUM_MIPS,
                    gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
            _virtualPadStickTexture->setSource("virtualPad stick");
            auto usage = gpu::Texture::Usage::Builder().withColor().withAlpha();
            _virtualPadStickTexture->setUsage(usage.build());
            _virtualPadStickTexture->setStoredMipFormat(gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
            _virtualPadStickTexture->assignStoredMip(0, image.byteCount(), image.constBits());
            _virtualPadStickTexture->setAutoGenerateMips(true);
        }
    }

    if (!_virtualPadStickBaseTexture) {
        auto iconPath = PathUtils::resourcesPath() + "images/analog_stick_base.png";
        auto image = QImage(iconPath);
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
        if ((image.width() > 0) && (image.height() > 0)) {
            image = image.scaled(_virtualPadPixelSize, _virtualPadPixelSize, Qt::KeepAspectRatio);

            _virtualPadStickBaseTexture = gpu::Texture::createStrict(
                    gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA),
                    image.width(), image.height(),
                    gpu::Texture::MAX_NUM_MIPS,
                    gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
            _virtualPadStickBaseTexture->setSource("virtualPad base");
            auto usage = gpu::Texture::Usage::Builder().withColor().withAlpha();
            _virtualPadStickBaseTexture->setUsage(usage.build());
            _virtualPadStickBaseTexture->setStoredMipFormat(gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
            _virtualPadStickBaseTexture->assignStoredMip(0, image.byteCount(), image.constBits());
            _virtualPadStickBaseTexture->setAutoGenerateMips(true);
        }
    }

    if (_virtualPadButtons.size() == 0) {
        _virtualPadButtons.append(VirtualPadButton(
                dpi * VirtualPad::Manager::BTN_FULL_PIXELS / VirtualPad::Manager::DPI,
                PathUtils::resourcesPath() + "images/fly.png",
                VirtualPad::Manager::Button::JUMP));
        _virtualPadButtons.append(VirtualPadButton(
                dpi * VirtualPad::Manager::BTN_FULL_PIXELS / VirtualPad::Manager::DPI,
                PathUtils::resourcesPath() + "images/handshake.png",
                VirtualPad::Manager::Button::HANDSHAKE));
    }
#endif
    Parent::customizeContext();
}

void Basic2DWindowOpenGLDisplayPlugin::uncustomizeContext() {
    Parent::uncustomizeContext();
}

bool Basic2DWindowOpenGLDisplayPlugin::internalActivate() {
    _framerateActions.clear();
#if defined(Q_OS_ANDROID)
    _container->setFullscreen(nullptr, true);
#endif
    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), FULLSCREEN,
        [this](bool clicked) {
            if (clicked) {
                _container->setFullscreen(getFullscreenTarget());
            } else {
                _container->unsetFullscreen();
            }
        }, true, false);

    return Parent::internalActivate();
}

gpu::PipelinePointer Basic2DWindowOpenGLDisplayPlugin::getRenderTexturePipeline() {
#if defined(Q_OS_ANDROID)
    return _linearToSRGBPipeline;
#else
    return _drawTexturePipeline;
#endif
}

void Basic2DWindowOpenGLDisplayPlugin::compositeExtra() {
#if defined(Q_OS_ANDROID)
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if(virtualPadManager.getLeftVirtualPad()->isShown()) {
        // render stick base
        auto stickBaseTransform = DependencyManager::get<CompositorHelper>()->getPoint2DTransform(virtualPadManager.getLeftVirtualPad()->getFirstTouch(),
                                                                                                    _virtualPadPixelSize, _virtualPadPixelSize);
        auto stickTransform = DependencyManager::get<CompositorHelper>()->getPoint2DTransform(virtualPadManager.getLeftVirtualPad()->getCurrentTouch(),
                                                                                              _virtualPadPixelSize, _virtualPadPixelSize);

        render([&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.setFramebuffer(_compositeFramebuffer);
            batch.resetViewTransform();
            batch.setProjectionTransform(mat4());
            batch.setPipeline(_cursorPipeline);

            batch.setResourceTexture(0, _virtualPadStickBaseTexture);
            batch.setModelTransform(stickBaseTransform);
            batch.draw(gpu::TRIANGLE_STRIP, 4);

            batch.setResourceTexture(0, _virtualPadStickTexture);
            batch.setModelTransform(stickTransform);
            batch.draw(gpu::TRIANGLE_STRIP, 4);

            foreach(VirtualPadButton virtualPadButton, _virtualPadButtons) {
                virtualPadButton.draw(batch, virtualPadManager.getButtonPosition(virtualPadButton._button));
            }
        });
    }
#endif
    Parent::compositeExtra();
}

static const uint32_t MIN_THROTTLE_CHECK_FRAMES = 60;

bool Basic2DWindowOpenGLDisplayPlugin::isThrottled() const {
    static auto lastCheck = presentCount();
    // Don't access the menu API every single frame
    if ((presentCount() - lastCheck) > MIN_THROTTLE_CHECK_FRAMES) {
        static const QString ThrottleFPSIfNotFocus = "Throttle FPS If Not Focus"; // FIXME - this value duplicated in Menu.h
        _isThrottled  = (!_container->isForeground() && _container->isOptionChecked(ThrottleFPSIfNotFocus));
        lastCheck = presentCount();
    }

    return _isThrottled;
}

// FIXME target the screen the window is currently on
QScreen* Basic2DWindowOpenGLDisplayPlugin::getFullscreenTarget() {
    return qApp->primaryScreen();
}

#if defined(Q_OS_ANDROID)

Basic2DWindowOpenGLDisplayPlugin::VirtualPadButton::VirtualPadButton(qreal pixelSize,
                                                                     QString iconPath,
                                                                     VirtualPad::Manager::Button button) :
    _pixelSize { pixelSize },
    _button { button }
{
    if (!_texture) {
        auto image = QImage(iconPath);
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
        if ((image.width() > 0) && (image.height() > 0)) {
            image = image.scaled(_pixelSize, _pixelSize, Qt::KeepAspectRatio);
            image = image.mirrored();

            _texture = gpu::Texture::createStrict(
                    gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA),
                    image.width(), image.height(),
                    gpu::Texture::MAX_NUM_MIPS,
                    gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
            _texture->setSource(iconPath.toStdString());
            auto usage = gpu::Texture::Usage::Builder().withColor().withAlpha();
            _texture->setUsage(usage.build());
            _texture->setStoredMipFormat(gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
            _texture->assignStoredMip(0, image.byteCount(), image.constBits());
            _texture->setAutoGenerateMips(true);
        }
    }
}

void Basic2DWindowOpenGLDisplayPlugin::VirtualPadButton::draw(gpu::Batch &batch,
                                                              glm::vec2 buttonPosition) {
    auto transform = DependencyManager::get<CompositorHelper>()->getPoint2DTransform(
            buttonPosition,
            _pixelSize, _pixelSize);
    batch.setResourceTexture(0, _texture);
    batch.setModelTransform(transform);
    batch.draw(gpu::TRIANGLE_STRIP, 4);
}

#endif