//
//  Client.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AddressManager.h>
#include <NodeList.h>

#include "Client.h"

Client::Client(QObject* parent) :
    QObject(parent)
{
    DependencyManager::set<AddressManager>();

    // setup the NodeList for this client
    auto nodeList = DependencyManager::set<NodeList>(NodeType::Agent, 0);
}
