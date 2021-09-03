//
//  LocationScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocationScriptingInterface_h
#define hifi_LocationScriptingInterface_h

#include "ScriptValue.h"

class ScriptContext;
class ScriptEngine;

class LocationScriptingInterface : public QObject {
    Q_OBJECT
public:
    static LocationScriptingInterface* getInstance();

    static ScriptValue locationGetter(ScriptContext* context, ScriptEngine* engine);
    static ScriptValue locationSetter(ScriptContext* context, ScriptEngine* engine);
private:
    LocationScriptingInterface() {};
};

#endif // hifi_LocationScriptingInterface_h
