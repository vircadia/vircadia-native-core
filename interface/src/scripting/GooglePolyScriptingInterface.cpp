//
//  GooglePolyScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 12/3/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GooglePolyScriptingInterface.h"
#include "ScriptEngineLogging.h"

GooglePolyScriptingInterface::GooglePolyScriptingInterface() {

}

void GooglePolyScriptingInterface::testPrint() {
    qCDebug(scriptengine) << "Google Poly interface exists";
}