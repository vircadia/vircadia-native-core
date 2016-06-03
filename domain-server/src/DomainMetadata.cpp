//
//  DomainMetadata.cpp
//  domain-server/src
//
//  Created by Zach Pomerantz on 5/25/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include "DomainMetadata.h"

#include <DependencyManager.h>
#include <LimitedNodeList.h>

#include "DomainServerNodeData.h"

const QString DomainMetadata::USERS_KEY = "users";
const QString DomainMetadata::USERS_NUM_KEY = "num_users";
const QString DomainMetadata::USERS_HOSTNAMES_KEY = "users_hostnames";

DomainMetadata::DomainMetadata() :
    _metadata{{ USERS_KEY, {} }} {
}

void DomainMetadata::usersChanged() {
    static const QString DEFAULT_HOSTNAME = "*";

    auto nodeList = DependencyManager::get<LimitedNodeList>();
    int numConnectedUnassigned = 0;
    QVariantMap userHostnames;

    // figure out the breakdown of currently connected interface clients
    nodeList->eachNode([&numConnectedUnassigned, &userHostnames](const SharedNodePointer& node) {
        auto linkedData = node->getLinkedData();
        if (linkedData) {
            auto nodeData = static_cast<DomainServerNodeData*>(linkedData);

            if (!nodeData->wasAssigned()) {
                ++numConnectedUnassigned;

                // increment the count for this hostname (or the default if we don't have one)
                auto placeName = nodeData->getPlaceName();
                auto hostname = placeName.isEmpty() ? DEFAULT_HOSTNAME : placeName;
                userHostnames[hostname] = userHostnames[hostname].toInt() + 1;
            }
        }
    });

    QVariantMap users = {{ USERS_NUM_KEY, numConnectedUnassigned }, { USERS_HOSTNAMES_KEY, userHostnames }};
    _metadata[USERS_KEY] = users;

#if DEV_BUILD
    qDebug() << "Regenerated domain metadata - users:" << users;
#endif
}
