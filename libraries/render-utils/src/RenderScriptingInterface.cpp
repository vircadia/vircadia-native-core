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

void RenderScriptingInterface::setEngineToneMappingToneCurve(const QString& toneCurve) {
    if (toneCurve == QString("None")) {
        _tone.toneCurve = 0;
    } else if (toneCurve == QString("Gamma22")) {
        _tone.toneCurve = 1;
    } else if (toneCurve == QString("Reinhard")) {
        _tone.toneCurve = 2;
    } else if (toneCurve == QString("Filmic")) {
        _tone.toneCurve = 3;
    }
}

QString RenderScriptingInterface::getEngineToneMappingToneCurve() const {
    switch (_tone.toneCurve) {
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