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

UsersScriptingInterface::UsersScriptingInterface() {
    // emit a signal when kick permissions have changed
    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &LimitedNodeList::canKickChanged, this, &UsersScriptingInterface::canKickChanged);
}

void UsersScriptingInterface::ignore(const QUuid& nodeID) {
    // ask the NodeList to ignore this user (based on the session ID of their node)
    DependencyManager::get<NodeList>()->ignoreNodeBySessionID(nodeID);
}

void UsersScriptingInterface::kick(const QUuid& nodeID) {
    // ask the NodeList to kick the user with the given session ID
    DependencyManager::get<NodeList>()->kickNodeBySessionID(nodeID);
}

bool UsersScriptingInterface::getCanKick() {
    // ask the NodeList to return our ability to kick
    return DependencyManager::get<NodeList>()->getThisNodeCanKick();
}
