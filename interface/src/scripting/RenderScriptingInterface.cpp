//
//  Created by Sam Gondelman on 5/16/19
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RenderScriptingInterface.h"

#include "LightingModel.h"
#include "AntialiasingEffect.h"


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
        _antialiasingEnabled = (_antialiasingEnabledSetting.get());
        _viewportResolutionScale = (_viewportResolutionScaleSetting.get());
    });
    forceRenderMethod((RenderMethod)_renderMethod);
    forceShadowsEnabled(_shadowsEnabled);
    forceAmbientOcclusionEnabled(_ambientOcclusionEnabled);
    forceAntialiasingEnabled(_antialiasingEnabled);
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

        auto config = dynamic_cast<task::SwitchConfig*>(qApp->getRenderEngine()->getConfiguration()->getConfig("RenderMainView.DeferredForwardSwitch"));
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

        auto lightingModelConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<MakeLightingModel>("RenderMainView.LightingModel");
        if (lightingModelConfig) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::Shadows, enabled);
            lightingModelConfig->setShadow(enabled);
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

bool RenderScriptingInterface::getAntialiasingEnabled() const {
    return _antialiasingEnabled;
}

void RenderScriptingInterface::setAntialiasingEnabled(bool enabled) {
    if (_antialiasingEnabled != enabled) {
        forceAntialiasingEnabled(enabled);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceAntialiasingEnabled(bool enabled) {
    _renderSettingLock.withWriteLock([&] {
        _antialiasingEnabled = (enabled);
        _antialiasingEnabledSetting.set(enabled);

        auto mainViewJitterCamConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<JitterSample>("RenderMainView.JitterCam");
        auto mainViewAntialiasingConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<Antialiasing>("RenderMainView.Antialiasing");
        if (mainViewJitterCamConfig && mainViewAntialiasingConfig) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::AntiAliasing, enabled);
            if (enabled) {
                mainViewJitterCamConfig->play();
                mainViewAntialiasingConfig->setDebugFXAA(false);
            }
            else {
                mainViewJitterCamConfig->none();
                mainViewAntialiasingConfig->setDebugFXAA(true);
            }
        }
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
