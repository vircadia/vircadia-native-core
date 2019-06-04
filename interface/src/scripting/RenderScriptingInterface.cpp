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

std::once_flag RenderScriptingInterface::registry_flag;

RenderScriptingInterface* RenderScriptingInterface::getInstance() {
    static RenderScriptingInterface sharedInstance;
    return &sharedInstance;
}

RenderScriptingInterface::RenderScriptingInterface() {
    std::call_once(registry_flag, [] {
        qmlRegisterType<RenderScriptingInterface>("RenderEnums", 1, 0, "RenderEnums");
    });
}

void RenderScriptingInterface::loadSettings() {
    _renderSettingLock.withReadLock([&] {
        _renderMethod.store(_renderMethodSetting.get());
        _shadowsEnabled.store(_shadowsEnabledSetting.get());
        _ambientOcclusionEnabled.store(_ambientOcclusionEnabledSetting.get());
        _antialiasingEnabled.store(_antialiasingEnabledSetting.get());
    });
    forceRenderMethod((RenderMethod)_renderMethod.load());
    forceShadowsEnabled(_shadowsEnabled.load());
    forceAmbientOcclusionEnabled(_ambientOcclusionEnabled.load());
    forceAntialiasingEnabled(_antialiasingEnabled.load());
}

RenderScriptingInterface::RenderMethod RenderScriptingInterface::getRenderMethod() {
    return (RenderMethod) _renderMethod.load();
}

void RenderScriptingInterface::setRenderMethod(RenderMethod renderMethod) {
    if (_renderMethod != (int) renderMethod) {
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
            _renderMethodSetting.set((int)renderMethod);
            config->setBranch((int)renderMethod);
        }
    });
}

QStringList RenderScriptingInterface::getRenderMethodNames() const {
    static const QStringList refrenderMethodNames = { "DEFERRED", "FORWARD" };
    return refrenderMethodNames;
}

bool RenderScriptingInterface::getShadowsEnabled() {
    return _shadowsEnabled.load();
}

void RenderScriptingInterface::setShadowsEnabled(bool enabled) {
    if (_shadowsEnabled != enabled) {
        forceShadowsEnabled(enabled);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceShadowsEnabled(bool enabled) {
    _renderSettingLock.withWriteLock([&] {
        _shadowsEnabled.store(enabled);
        _shadowsEnabledSetting.set(enabled);

        auto lightingModelConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<MakeLightingModel>("RenderMainView.LightingModel");
        if (lightingModelConfig) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::Shadows, enabled);
            lightingModelConfig->setShadow(enabled);
        }
    });
}

bool RenderScriptingInterface::getAmbientOcclusionEnabled() {
    return _ambientOcclusionEnabled.load();
}

void RenderScriptingInterface::setAmbientOcclusionEnabled(bool enabled) {
    if (_ambientOcclusionEnabled.load() != enabled) {
        forceAmbientOcclusionEnabled(enabled);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceAmbientOcclusionEnabled(bool enabled) {
    _renderSettingLock.withWriteLock([&] {
        _ambientOcclusionEnabled.store(enabled);
        _ambientOcclusionEnabledSetting.set(enabled);

        auto lightingModelConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<MakeLightingModel>("RenderMainView.LightingModel");
        if (lightingModelConfig) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::AmbientOcclusion, enabled);
            lightingModelConfig->setAmbientOcclusion(enabled);
        }
    });
}

bool RenderScriptingInterface::getAntialiasingEnabled() {
    return _antialiasingEnabled.load();
}

void RenderScriptingInterface::setAntialiasingEnabled(bool enabled) {
    if (_antialiasingEnabled.load() != enabled) {
        forceAntialiasingEnabled(enabled);
        emit settingsChanged();
    }
}

void RenderScriptingInterface::forceAntialiasingEnabled(bool enabled) {
    _renderSettingLock.withWriteLock([&] {
        _antialiasingEnabled.store(enabled);
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


