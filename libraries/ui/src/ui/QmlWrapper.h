//
//  Created by Anthony J. Thibault on 2016-12-12
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QmlWrapper_h
#define hifi_QmlWrapper_h

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>

class QmlWrapper : public QObject {
    Q_OBJECT
public:
    QmlWrapper(QObject* qmlObject, QObject* parent = nullptr);

    Q_INVOKABLE void writeProperty(QString propertyName, QVariant propertyValue);
    Q_INVOKABLE void writeProperties(QVariant propertyMap);
    Q_INVOKABLE QVariant readProperty(const QString& propertyName);
    Q_INVOKABLE QVariant readProperties(const QVariant& propertyList);

protected:
    QObject* _qmlObject{ nullptr };
};

template <typename T>
QScriptValue wrapperToScriptValue(QScriptEngine* engine, T* const &in) {
    if (!in) {
        return engine->undefinedValue();
    }
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects);
}

template <typename T>
void wrapperFromScriptValue(const QScriptValue& value, T* &out) {
    out = qobject_cast<T*>(value.toQObject());
}

#endif