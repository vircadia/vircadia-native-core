//
//  Created by Sam Gondelman on 5/16/19
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RenderScriptingInterface.h"

#include "LightingModel.h"


RenderScriptingInterface* RenderScriptingInterface::getInstance() {
    static RenderScriptingInterface sharedInstance;
    return &sharedInstance;
}

std::once_flag RenderScriptingInterface::registry_flag;

RenderScriptingInterface::RenderScriptingInterface() {
    std::call_once(registry_flag, [] {
        qmlRegisterType<RenderScriptingInterface>("RenderEnums", 1, 0, "RenderEnums");
    });
}

void RenderScriptingInterface::loadSettings() {
    _renderSettingLock.withReadLock([&] {
        _renderMethod = (_renderMethodSetting.get());
        _shadowsEnabled = (_shadowsEnabledSetting.get());
        _ambientOcclusionEnabled = (_ambientOcclusionEnabledSetting.get());
        //_antialiasingMode = (_antialiasingModeSetting.get());
        _antialiasingMode = static_cast<AntialiasingConfig::Mode>(_antialiasingModeSetting.get());
        _viewportResolutionScale = (_viewportResolutionScaleSetting.get());
    });
    forceRenderMethod((RenderMethod)_renderMethod);
    forceShadowsEnabled(_shadowsEnabled);
    forceAmbientOcclusionEnabled(_ambientOcclusionEnabled);
    forceAntialiasingMode(_antialiasingMode);
    forceViewportResolutionScale(_viewportResolutionScale);
}

RenderScriptingInterface::RenderMethod RenderScriptingInterface::getRenderMethod() const {
    return (RenderMethod) _renderMethod;
}

void RenderScriptingInterface::setRenderMethod(RenderMethod renderMethod) {
    if (isValidRenderMethod(renderMethod) && (_renderMethod != (int) renderMethod)) {
        forceRenderMethod(renderMethod);
        emit settingsChanged();
    }
}
void RenderScriptingInterface::forceRenderMethod(RenderMethod renderMethod) {
    _renderSettingLock.withWriteLock([&] {
        _renderMethod = (int)renderMethod;
        _renderMethodSetting.set((int)renderMethod);

        auto config = dynamic_cast<render::SwitchConfig*>(qApp->getRenderEngine()->getConfiguration()->getConfig("RenderMainView.DeferredForwardSwitch"));
        if (config) {
            config->setBranch((int)renderMethod);
        }
    });
}

QStringList RenderScriptingInterface::getRenderMethodNames() const {
    static const QStringList refrenderMethodNames = { "DEFERRED", "FORWARD" };
    return refrenderMethodNames;
}

bool RenderScriptingInterface::getShadowsEnabled() const {
    return _shadowsEnabled;
}

void RenderScriptingInterface::setShadowsEnabled(bool enabled) {
    if (_shadowsEnabled != enabled) {
        forceShadowsEnabled(enabled);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceShadowsEnabled(bool enabled) {
    _renderSettingLock.withWriteLock([&] {
        _shadowsEnabled = (enabled);
        _shadowsEnabledSetting.set(enabled);

        auto renderConfig = qApp->getRenderEngine()->getConfiguration();
        assert(renderConfig);
        auto lightingModelConfig = renderConfig->getConfig<MakeLightingModel>("RenderMainView.LightingModel");
        if (lightingModelConfig) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::Shadows, enabled);
            lightingModelConfig->setShadow(enabled);
        }
        auto secondaryLightingModelConfig = renderConfig->getConfig<MakeLightingModel>("RenderSecondView.LightingModel");
        if (secondaryLightingModelConfig) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::Shadows, enabled);
            secondaryLightingModelConfig->setShadow(enabled);
        }
    });
}

bool RenderScriptingInterface::getAmbientOcclusionEnabled() const {
    return _ambientOcclusionEnabled;
}

void RenderScriptingInterface::setAmbientOcclusionEnabled(bool enabled) {
    if (_ambientOcclusionEnabled != enabled) {
        forceAmbientOcclusionEnabled(enabled);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceAmbientOcclusionEnabled(bool enabled) {
    _renderSettingLock.withWriteLock([&] {
        _ambientOcclusionEnabled = (enabled);
        _ambientOcclusionEnabledSetting.set(enabled);

        auto lightingModelConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<MakeLightingModel>("RenderMainView.LightingModel");
        if (lightingModelConfig) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::AmbientOcclusion, enabled);
            lightingModelConfig->setAmbientOcclusion(enabled);
        }
    });
}

AntialiasingConfig::Mode RenderScriptingInterface::getAntialiasingMode() const {
    return _antialiasingMode;
}

void RenderScriptingInterface::setAntialiasingMode(AntialiasingConfig::Mode mode) {
    if (_antialiasingMode != mode) {
        forceAntialiasingMode(mode);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceAntialiasingMode(AntialiasingConfig::Mode mode) {
    _renderSettingLock.withWriteLock([&] {
        _antialiasingMode = mode;

        auto mainViewJitterCamConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<JitterSample>("RenderMainView.JitterCam");
        auto mainViewAntialiasingConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<Antialiasing>("RenderMainView.Antialiasing");
        if (mainViewJitterCamConfig && mainViewAntialiasingConfig) {
	    switch (mode) {
                case AntialiasingConfig::Mode::NONE:
                    mainViewJitterCamConfig->none();
                    mainViewAntialiasingConfig->blend = 1;
                    mainViewAntialiasingConfig->setDebugFXAA(false);
                    break;
                case AntialiasingConfig::Mode::TAA:
                    mainViewJitterCamConfig->play();
                    mainViewAntialiasingConfig->blend = 0.25;
                    mainViewAntialiasingConfig->setDebugFXAA(false);
                    break;
                case AntialiasingConfig::Mode::FXAA:
                    mainViewJitterCamConfig->none();
                    mainViewAntialiasingConfig->blend = 0.25;
                    mainViewAntialiasingConfig->setDebugFXAA(true);
                    break;
                default:
                    _antialiasingMode = AntialiasingConfig::Mode::NONE;
                    mainViewJitterCamConfig->none();
                    mainViewAntialiasingConfig->blend = 1;
                    mainViewAntialiasingConfig->setDebugFXAA(false);
                    break;
            }
        }

        _antialiasingModeSetting.set(_antialiasingMode);
    });
}


float RenderScriptingInterface::getViewportResolutionScale() const {
    return _viewportResolutionScale;
}

void RenderScriptingInterface::setViewportResolutionScale(float scale) {
    if (_viewportResolutionScale != scale) {
        forceViewportResolutionScale(scale);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceViewportResolutionScale(float scale) {
    // just not negative values or zero
    if (scale <= 0.f) {
        return;
    }
    _renderSettingLock.withWriteLock([&] {
        _viewportResolutionScale = (scale);
        _viewportResolutionScaleSetting.set(scale);

        auto renderConfig = qApp->getRenderEngine()->getConfiguration();
        assert(renderConfig);
        auto deferredView = renderConfig->getConfig("RenderMainView.RenderDeferredTask");
        // mainView can be null if we're rendering in forward mode
        if (deferredView) {
            deferredView->setProperty("resolutionScale", _viewportResolutionScale);
        }
        auto forwardView = renderConfig->getConfig("RenderMainView.RenderForwardTask");
        // mainView can be null if we're rendering in forward mode
        if (forwardView) {
            forwardView->setProperty("resolutionScale", _viewportResolutionScale);
        }
    });
}
