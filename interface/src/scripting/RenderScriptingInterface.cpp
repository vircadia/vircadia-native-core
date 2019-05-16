//
//  Created by Sam Gondelman on 5/16/19
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RenderScriptingInterface.h"

const QString DEFERRED = "deferred";
const QString FORWARD = "forward";

RenderScriptingInterface* RenderScriptingInterface::getInstance() {
    static RenderScriptingInterface sharedInstance;
    return &sharedInstance;
}

RenderScriptingInterface::RenderScriptingInterface() {
    setRenderMethod((render::Args::RenderMethod)_renderMethodSetting.get() == render::Args::RenderMethod::DEFERRED ? DEFERRED : FORWARD);
}

QString RenderScriptingInterface::getRenderMethod() {
    return (render::Args::RenderMethod)_renderMethodSetting.get() == render::Args::RenderMethod::DEFERRED ? DEFERRED : FORWARD;
}

void RenderScriptingInterface::setRenderMethod(const QString& renderMethod) {
    auto config = dynamic_cast<task::SwitchConfig*>(qApp->getRenderEngine()->getConfiguration()->getConfig("RenderMainView.DeferredForwardSwitch"));
    if (config) {
        if (renderMethod == DEFERRED) {
            _renderMethodSetting.set(render::Args::RenderMethod::DEFERRED);
            config->setBranch(render::Args::RenderMethod::DEFERRED);
            emit config->dirtyEnabled();
        } else if (renderMethod == FORWARD) {
            _renderMethodSetting.set(render::Args::RenderMethod::FORWARD);
            config->setBranch(render::Args::RenderMethod::FORWARD);
            emit config->dirtyEnabled();
        }
    }
}