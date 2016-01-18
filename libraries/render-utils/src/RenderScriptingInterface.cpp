//
//  RenderScriptingInterface.cpp
//  libraries/render-utils
//
//  Created by Zach Pomerantz on 12/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderScriptingInterface.h"

RenderScriptingInterface::RenderScriptingInterface() {};

void RenderScripting::Tone::setCurve(const QString& curve) {
    if (curve == QString("None")) {
        toneCurve = 0;
    } else if (curve == QString("Gamma22")) {
        toneCurve = 1;
    } else if (curve == QString("Reinhard")) {
        toneCurve = 2;
    } else if (curve == QString("Filmic")) {
        toneCurve = 3;
    }
}

QString RenderScripting::Tone::getCurve() const {
    switch (toneCurve) {
    case 0:
        return QString("None");
    case 1:
        return QString("Gamma22");
    case 2:
        return QString("Reinhard");
    case 3:
        return QString("Filmic");
    default:
        return QString("Filmic");
    };
}

render::RenderContext RenderScriptingInterface::getRenderContext() {
    render::RenderContext::ItemsConfig items{ *_opaque, *_transparent, *_overlay3D };
    return render::RenderContext{ items, *_tone, *_ambientOcclusion, _drawStatus, _drawHitEffect, _deferredDebugSize, _deferredDebugMode };
}

void RenderScriptingInterface::setItemCounts(const render::RenderContext::ItemsConfig& items) {
    _opaque->setCounts(items.opaque);
    _transparent->setCounts(items.transparent);
    _overlay3D->setCounts(items.overlay3D);
}

void RenderScriptingInterface::setJobGPUTimes(double aoTime) {
    _ambientOcclusion->gpuTime = aoTime;
}