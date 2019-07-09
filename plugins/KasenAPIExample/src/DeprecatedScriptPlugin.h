//
//  DeprecatedScriptPlugin.h
//  plugins/KasenAPIExample/src
//
//  Created by Kasen IO on 2019.07.14 | realities.dev | kasenvr@gmail.com
//  Copyright 2019 Kasen IO
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Supporting file containing all QtScript specific integration.

#ifndef DEPRECATED_SCRIPT_PLUGIN_H
#define DEPRECATED_SCRIPT_PLUGIN_H

#if DEV_BUILD
#pragma message("QtScript is deprecated see: doc.qt.io/qt-5/topics-scripting.html")
#endif
#include <QtScript/QScriptEngine>

#include <QtCore/QLoggingCategory>
#include <QCoreApplication>
#include <shared/ScriptInitializerMixin.h>

namespace deprecated {

extern const QLoggingCategory& logger;

inline void setGlobalInstance(QScriptEngine* engine, const QString& name, QObject* object) {
    auto value = engine->newQObject(object, QScriptEngine::QtOwnership);
    engine->globalObject().setProperty(name, value);
    if (1 || getenv("DEBUG")) {
        qCDebug(logger) << "(via ScriptInitializers)...setGlobalInstance" << name << engine->property("fileName");
    }
}

class ScriptPlugin : public QObject {
    Q_OBJECT
    QString _version;
    Q_PROPERTY(QString version MEMBER _version CONSTANT)
protected:
    inline ScriptPlugin(const QString& name, const QString& version) : _version(version) {
        setObjectName(name);
        if (!DependencyManager::get<ScriptInitializers>()) {
            qCWarning(logger) << "COULD NOT INITIALIZE (ScriptInitializers unavailable)" << qApp << this;
            return;
        }
        qCWarning(logger) << "registering w/ScriptInitializerMixin..." << DependencyManager::get<ScriptInitializers>().data();
        DependencyManager::get<ScriptInitializers>()->registerScriptInitializer(
            [this](QScriptEngine* engine) { setGlobalInstance(engine, objectName(), this); });
    }
public slots:
    inline QString toString() const { return QString("[%1 version=%2]").arg(objectName()).arg(_version); }
};

}  // namespace deprecated

#endif