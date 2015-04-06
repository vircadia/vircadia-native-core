//
//  ScriptsScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Thijs Wenker on 3/31/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include "ScriptsScriptingInterface.h"

ScriptsScriptingInterface* ScriptsScriptingInterface::getInstance() {
    static ScriptsScriptingInterface sharedInstance;
    return &sharedInstance;
}

QStringList ScriptsScriptingInterface::getRunning() {
    return Application::getInstance()->getRunningScripts();
}

QScriptValue ScriptsScriptingInterface::getPublic(QScriptContext* context, QScriptEngine* engine) {
    return QScriptValue::NullValue;
}
