//
//  ScriptContext.h
//  libraries/script-engine/src
//
//  Created by Heather Anderson on 12/5/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptContext.h"

#include "Scriptable.h"

ScriptContextGuard::ScriptContextGuard(ScriptContext* context) {
    _oldContext = Scriptable::context();
    Scriptable::setContext(context);
}

ScriptContextGuard::~ScriptContextGuard() {
    Scriptable::setContext(_oldContext);
}
