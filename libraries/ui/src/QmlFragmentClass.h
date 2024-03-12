//
//  Created by Gabriel Calero & Cristian Duarte on Aug 25, 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ui_QmlFragmentClass_h
#define hifi_ui_QmlFragmentClass_h

#include "QmlWindowClass.h"
#include <ScriptValue.h>

class ScriptContext;
class ScriptEngine;

class QmlFragmentClass : public QmlWindowClass {
    Q_OBJECT

private:
    static ScriptValue internal_constructor(ScriptContext* context, ScriptEngine* engine, bool restricted);
public: 
    static ScriptValue constructor(ScriptContext* context, ScriptEngine* engine) {
        return internal_constructor(context, engine, false);
    }

    static ScriptValue restricted_constructor(ScriptContext* context, ScriptEngine* engine ){
        return internal_constructor(context, engine, true);
    }

    QmlFragmentClass(bool restricted, QString id);

    /*@jsdoc
     * Creates a new button, adds it to this and returns it.
     * @function QmlFragmentClass#addButton
     * @param properties {object} button properties 
     * @returns {TabletButtonProxy}
     */
    Q_INVOKABLE QObject* addButton(const QVariant& properties);

    /*
     * TODO - not yet implemented
     */
    Q_INVOKABLE void removeButton(QObject* tabletButtonProxy);
public slots:
    Q_INVOKABLE void close();

protected:
    QString qmlSource() const override { return qml; }

    static std::mutex _mutex;
    static std::map<QString, ScriptValue> _fragments;
private:
    QString qml;

};

#endif
