//
//  LocationScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AddressManager.h>

#include "LocationScriptingInterface.h"

LocationScriptingInterface* LocationScriptingInterface::getInstance() {
    static LocationScriptingInterface sharedInstance;
    return &sharedInstance;
}

QScriptValue LocationScriptingInterface::locationGetter(QScriptContext* context, QScriptEngine* engine) {
    return engine->newQObject(&AddressManager::getInstance());
}

QScriptValue LocationScriptingInterface::locationSetter(QScriptContext* context, QScriptEngine* engine) {
    const QVariant& argumentVariant = context->argument(0).toVariant();
    
    
    if (argumentVariant.canConvert(QMetaType::QVariantMap)) {
        // this argument is a variant map, so we'll assume it's an address map
        QMetaObject::invokeMethod(&AddressManager::getInstance(), "goToAddressFromObject",
                                  Q_ARG(const QVariantMap&, argumentVariant.toMap()));
    } else {
        // just try and convert the argument to a string, should be a hifi:// address
        QMetaObject::invokeMethod(&AddressManager::getInstance(), "handleLookupString",
                                  Q_ARG(const QString&, argumentVariant.toString()));
    }
    
    return QScriptValue::UndefinedValue;
}
