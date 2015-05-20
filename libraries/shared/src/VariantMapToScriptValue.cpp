//
//  VariantMapToScriptValue.cpp
//  libraries/shared/src/
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include "SharedLogging.h"
#include "VariantMapToScriptValue.h"

QScriptValue variantMapToScriptValue(QVariantMap& variantMap, QScriptEngine& scriptEngine) {
    QScriptValue scriptValue = scriptEngine.newObject();

    for (QVariantMap::const_iterator iter = variantMap.begin(); iter != variantMap.end(); ++iter) {
        QString key = iter.key();
        QVariant qValue = iter.value();

        switch(qValue.type()) {
        case QVariant::Bool:
            scriptValue.setProperty(key, qValue.toBool());
            break;
        case QVariant::Int:
            scriptValue.setProperty(key, qValue.toInt());
            break;
        case QVariant::Double:
            scriptValue.setProperty(key, qValue.toDouble());
            break;
        case QVariant::String: {
            scriptValue.setProperty(key, scriptEngine.newVariant(qValue));
            break;
        }
        case QVariant::Map: {
            QVariantMap childMap = qValue.toMap();
            scriptValue.setProperty(key, variantMapToScriptValue(childMap, scriptEngine));
            break;
        }
        default:
            qCDebug(shared) << "unhandled QScript type" << qValue.type();
        }
    }

    return scriptValue;
}
