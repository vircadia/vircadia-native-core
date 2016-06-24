//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ToolbarScriptingInterface_h
#define hifi_ToolbarScriptingInterface_h

#include <mutex>

#include <QtCore/QObject>

#include <DependencyManager.h>

class ToolbarProxy;

class ToolbarScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    Q_INVOKABLE QObject* getToolbar(const QString& toolbarId);
};

#endif // hifi_ToolbarScriptingInterface_h
