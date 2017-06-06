//
//  Audio.cpp
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 28/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Audio.h"

using namespace scripting;

static const QString DESKTOP_CONTEXT { "Desktop" };
static const QString HMD_CONTEXT { "VR" };

QString Audio::getContext() {
    return _contextIsHMD ? HMD_CONTEXT : DESKTOP_CONTEXT;
}