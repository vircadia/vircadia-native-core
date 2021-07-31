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

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ArrayBufferClass_h
#define hifi_ArrayBufferClass_h

#include <QScriptClass>
#include <QtCore/QObject>
#include <QtScript/QScriptClass>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptString>
#include <QtScript/QScriptValue>
#include <QtCore/QDateTime>

class ScriptEngine;

/// Implements the <code><a href="https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer">ArrayBuffer</a></code> scripting class
class ArrayBufferClass : public QObject, public QScriptClass {
    Q_OBJECT
public:
    ArrayBufferClass(ScriptEngine* scriptEngine);
    QScriptValue newInstance(qint32 size);
    QScriptValue newInstance(const QByteArray& ba);

    QueryFlags queryProperty(const QScriptValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint* id) override;
    QScriptValue property(const QScriptValue& object,
                          const QScriptString& name, uint id) override;
    QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object,
                                              const QScriptString& name, uint id) override;

    QString name() const override;
    QScriptValue prototype() const override;


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

/// @}
