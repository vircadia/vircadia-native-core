//
//  UsersScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-07-11.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_UsersScriptingInterface_h
#define hifi_UsersScriptingInterface_h

#include <DependencyManager.h>

class UsersScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public slots:
    void ignore(const QUuid& nodeID);
};


#endif // hifi_UsersScriptingInterface_h
