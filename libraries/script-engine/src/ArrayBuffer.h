//
//  ArrayBuffer.h
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ArrayBuffer_h
#define hifi_ArrayBuffer_h

#include <QScriptClass>
#include <QtCore/QObject>
#include <QtScript/QScriptClass>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptString>
#include <QtScript/QScriptValue>

class ArrayBuffer : public QObject, public QScriptClass {
public:
    ArrayBuffer(QScriptEngine* engine);
    QScriptValue newInstance(unsigned long size = 0);
    QScriptValue newInstance(const QByteArray& ba);
    
    QueryFlags queryProperty(const QScriptValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint* id);
    QScriptValue property(const QScriptValue& object,
                          const QScriptString& name,
                          uint id);
    QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object,
                                              const QScriptString& name,
                                              uint id);
    
    QString name() const;
    QScriptValue prototype() const;
    QScriptValue constructor() const;
    
private:
    static QScriptValue construct(QScriptContext* ctx, QScriptEngine* eng);
    
    static QScriptValue toScriptValue(QScriptEngine* eng, const QByteArray& ba);
    static void fromScriptValue(const QScriptValue& obj, QByteArray& ba);
    
    QScriptValue _proto;
    QScriptValue _ctor;
    
    // JS Object attributes
    QScriptString _byteLength;
};

#endif // hifi_ArrayBuffer_h