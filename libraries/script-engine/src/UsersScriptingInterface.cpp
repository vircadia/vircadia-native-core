//
//  UsersScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-07-11.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UsersScriptingInterface.h"

#include <NodeList.h>

void UsersScriptingInterface::ignore(const QUuid& nodeID) {
    // ask the NodeList to ignore this user (based on the session ID of their node)
    DependencyManager::get<NodeList>()->ignoreNodeBySessionID(nodeID);
}
