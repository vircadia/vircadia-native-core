//
//  ArrayBufferClass.h
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ArrayBufferClass_h
#define hifi_ArrayBufferClass_h

#include <QScriptClass>
#include <QtCore/QObject>
#include <QtScript/QScriptClass>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptString>
#include <QtScript/QScriptValue>

class ArrayBufferClass : public QObject, public QScriptClass {
    Q_OBJECT
public:
    ArrayBufferClass(QScriptEngine* engine);
    QScriptValue newInstance(quint32 size);
    QScriptValue newInstance(const QByteArray& ba);
    
    QueryFlags queryProperty(const QScriptValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint* id);
    QScriptValue property(const QScriptValue &object,
                          const QScriptString &name, uint id);
    QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object,
                                              const QScriptString& name, uint id);
    
    QString name() const;
    QScriptValue prototype() const;
    
private:
    static QScriptValue construct(QScriptContext* context, QScriptEngine* engine);
    
    static QScriptValue toScriptValue(QScriptEngine* eng, const QByteArray& ba);
    static void fromScriptValue(const QScriptValue& obj, QByteArray& ba);
    
    QScriptValue _proto;
    QScriptValue _ctor;
    
    // JS Object attributes
    QScriptString _name;
    QScriptString _byteLength;
};

#endif // hifi_ArrayBufferClass_h