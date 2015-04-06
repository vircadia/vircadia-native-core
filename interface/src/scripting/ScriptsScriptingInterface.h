//
//  ScriptsScriptingInterface.h
//  interface/src/scripting
//
//  Created by Thijs Wenker on 3/31/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptsScriptingInterface_h
#define hifi_ScriptsScriptingInterface_h

#include <QObject>

class ScriptsScriptingInterface : public QObject {
    Q_OBJECT
public:
    ScriptsScriptingInterface() {};
    static ScriptsScriptingInterface* getInstance();
    static QScriptValue getPublic(QScriptContext* context, QScriptEngine* engine);
public slots:
    QStringList getRunning();
};

#endif // hifi_ScriptsScriptingInterface_h
