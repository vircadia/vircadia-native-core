//
//  ScriptValueUtils.cpp
//  libraries/shared/src
//
//  Created by Anthony Thibault on 4/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Utilities for working with QtScriptValues
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptValueUtils.h"

bool isListOfStrings(const QScriptValue& arg) {
    if (!arg.isArray()) {
        return false;
    }

    auto lengthProperty = arg.property("length");
    if (!lengthProperty.isNumber()) {
        return false;
    }

    int length = lengthProperty.toInt32();
    for (int i = 0; i < length; i++) {
        if (!arg.property(i).isString()) {
            return false;
        }
    }

    return true;
}
