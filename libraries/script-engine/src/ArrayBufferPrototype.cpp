//
//  ArrayBufferPrototype.cpp
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QScriptEngine>

#include "ArrayBufferClass.h"

#include "ArrayBufferPrototype.h"

Q_DECLARE_METATYPE(QByteArray*)

ArrayBufferPrototype::ArrayBufferPrototype(QObject* parent) : QObject(parent) {
}

QByteArray ArrayBufferPrototype::slice(long begin, long end) const {
    return thisArrayBuffer()->mid(begin, end);
}

QByteArray* ArrayBufferPrototype::thisArrayBuffer() const {
    return qscriptvalue_cast<QByteArray*>(thisObject().data());
}
