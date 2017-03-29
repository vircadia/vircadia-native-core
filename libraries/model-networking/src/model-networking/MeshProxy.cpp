//
//  MeshProxy.cpp
//  libraries/model/src/model/
//
//  Created by Seth Alves on 2017-3-22.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MeshProxy.h"


QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in) {
    return engine->newQObject(in, QScriptEngine::QtOwnership,
                              QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects);
}

void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out) {
    out = qobject_cast<MeshProxy*>(value.toQObject());
}

QScriptValue meshesToScriptValue(QScriptEngine* engine, const MeshProxyList &in) {
    // QScriptValueList result;
    QScriptValue result = engine->newArray();
    int i = 0;
    foreach (MeshProxy* const meshProxy, in) {
        result.setProperty(i++, meshToScriptValue(engine, meshProxy));
    }
    return result;
}

void meshesFromScriptValue(const QScriptValue& value, MeshProxyList &out) {
    QScriptValueIterator itr(value);

    qDebug() << "in meshesFromScriptValue, value.length =" << value.property("length").toInt32();

    while(itr.hasNext()) {
        itr.next();
        MeshProxy* meshProxy = qscriptvalue_cast<MeshProxyList::value_type>(itr.value());
        if (meshProxy) {
            out.append(meshProxy);
        } else {
            qDebug() << "null meshProxy";
        }
    }
}
