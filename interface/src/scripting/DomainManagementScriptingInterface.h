//
//  DomainManagementScriptingInterface.h
//  interface/src/scripting
//
//  Created by Liv Erickson on 7/21/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_DomainManagementScriptingInterface_h
#define hifi_DomainManagementScriptingInterface_h

#include <QObject>
#include <QScriptContext>
#include <QScriptEngine>
#include <QString>
#include <QStringList>
#include <DependencyManager.h>
#include <Octree.h>

#include "BaseScriptEngine.h"


class DomainManagementScriptingInterface : public QObject, public Dependency {
Q_OBJECT
public:
    DomainManagementScriptingInterface();
    ~DomainManagementScriptingInterface();


public slots:
    Q_INVOKABLE bool canReplaceDomainContent();

signals:
    void canReplaceDomainContentChanged(bool canReplaceDomainContent);
};

#endif //hifi_DomainManagementScriptingInterface_h