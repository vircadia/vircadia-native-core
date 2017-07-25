//
//  DomainManagementScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Liv Erickson on 7/21/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DomainManagementScriptingInterface.h"
#include "Application.h"
#include "OffscreenUi.h"

DomainManagementScriptingInterface::DomainManagementScriptingInterface()
{
    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::canReplaceContentChanged, this, &DomainManagementScriptingInterface::canReplaceDomainContentChanged);
}

DomainManagementScriptingInterface::~DomainManagementScriptingInterface() {
    auto nodeList = DependencyManager::get<NodeList>();
    disconnect(nodeList.data(), &NodeList::canReplaceContentChanged, this, &DomainManagementScriptingInterface::canReplaceDomainContentChanged);
}

bool DomainManagementScriptingInterface::canReplaceDomainContent() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->getThisNodeCanReplaceContent();
}
