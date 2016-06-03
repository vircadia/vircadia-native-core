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

#include <HifiConfigVariantMap.h>
#include <DependencyManager.h>
#include <LimitedNodeList.h>

#include "DomainServerNodeData.h"

const QString DomainMetadata::USERS = "users";
const QString DomainMetadata::USERS_NUM_TOTAL = "num_users";
const QString DomainMetadata::USERS_NUM_ANON = "num_anon_users";
const QString DomainMetadata::USERS_HOSTNAMES = "user_hostnames";
// users metadata will appear as (JSON):
// { "num_users": Number,
//   "num_anon_users": Number,
//   "user_hostnames": { <HOSTNAME>: Number }
// }

const QString DomainMetadata::DESCRIPTORS = "descriptors";
const QString DomainMetadata::DESCRIPTORS_DESCRIPTION = "description";
const QString DomainMetadata::DESCRIPTORS_CAPACITY = "capacity"; // parsed from security
const QString DomainMetadata::DESCRIPTORS_RESTRICTION = "restriction"; // parsed from ACL
const QString DomainMetadata::DESCRIPTORS_MATURITY = "maturity";
const QString DomainMetadata::DESCRIPTORS_HOSTS = "hosts";
const QString DomainMetadata::DESCRIPTORS_TAGS = "tags";
// descriptors metadata will appear as (JSON):
// { "capacity": Number,
//   TODO: "hours": String, // UTF-8 representation of the week, split into 15" segments
//   "restriction": String, // enum of either open, hifi, or acl
//   "maturity": String, // enum corresponding to ESRB ratings
//   "hosts": [ String ], // capped list of usernames
//   "description": String, // capped description
//   TODO: "img": {
//      "src": String,
//      "type": String,
//      "size": Number,
//      "updated_at": Number,
//   },
//   "tags": [ String ], // capped list of tags
// }

// metadata will appear as (JSON):
// { users: <USERS>, descriptors: <DESCRIPTORS> }
//
// it is meant to be sent to and consumed by an external API

DomainMetadata::DomainMetadata() {
    _metadata[USERS] = {};
    _metadata[DESCRIPTORS] = {};
}

void DomainMetadata::setDescriptors(QVariantMap& settings) {
    const QString CAPACITY = "security.maximum_user_capacity";
    const QVariant* capacityVariant = valueForKeyPath(settings, CAPACITY);
    unsigned int capacity = capacityVariant ? capacityVariant->toUInt() : 0;

    // TODO: Keep parity with ACL development.
    const QString RESTRICTION = "security.restricted_access";
    const QString RESTRICTION_OPEN = "open";
    // const QString RESTRICTION_HIFI = "hifi";
    const QString RESTRICTION_ACL = "acl";
    const QVariant* isRestrictedVariant = valueForKeyPath(settings, RESTRICTION);
    bool isRestricted = isRestrictedVariant ? isRestrictedVariant->toBool() : false;
    QString restriction = isRestricted ? RESTRICTION_ACL : RESTRICTION_OPEN;

    QVariantMap descriptors = settings[DESCRIPTORS].toMap();
    descriptors[DESCRIPTORS_CAPACITY] = capacity;
    descriptors[DESCRIPTORS_RESTRICTION] = restriction;
    _metadata[DESCRIPTORS] = descriptors;

#if DEV_BUILD || PR_BUILD
    qDebug() << "Regenerated domain metadata - descriptors:" << descriptors;
#endif
}

void DomainMetadata::usersChanged() {
    static const QString DEFAULT_HOSTNAME = "*";

    auto nodeList = DependencyManager::get<LimitedNodeList>();
    int numConnected = 0;
    int numConnectedAnonymously = 0;
    QVariantMap userHostnames;

    // figure out the breakdown of currently connected interface clients
    nodeList->eachNode([&numConnected, &numConnectedAnonymously, &userHostnames](const SharedNodePointer& node) {
        auto linkedData = node->getLinkedData();
        if (linkedData) {
            auto nodeData = static_cast<DomainServerNodeData*>(linkedData);

            if (!nodeData->wasAssigned()) {
                ++numConnected;

                if (nodeData->getUsername().isEmpty()) {
                    ++numConnectedAnonymously;
                }

                // increment the count for this hostname (or the default if we don't have one)
                auto placeName = nodeData->getPlaceName();
                auto hostname = placeName.isEmpty() ? DEFAULT_HOSTNAME : placeName;
                userHostnames[hostname] = userHostnames[hostname].toInt() + 1;
            }
        }
    });

    QVariantMap users = {
        { USERS_NUM_TOTAL, numConnected },
        { USERS_NUM_ANON, numConnectedAnonymously },
        { USERS_HOSTNAMES, userHostnames }};
    _metadata[USERS] = users;

#if DEV_BUILD || PR_BUILD
    qDebug() << "Regenerated domain metadata - users:" << users;
#endif
}
