//
//  DomainMetadata.cpp
//  domain-server/src
//
//  Created by Zach Pomerantz on 5/25/2016.
//  Copyright 2016 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include "DomainMetadata.h"
#include "HTTPConnection.h"

#include <AccountManager.h>
#include <DependencyManager.h>
#include <HifiConfigVariantMap.h>
#include <LimitedNodeList.h>
#include <QLoggingCategory>

#include "DomainServer.h"
#include "DomainServerNodeData.h"

Q_LOGGING_CATEGORY(domain_metadata_exporter, "hifi.domain_server.metadata_exporter")

const QString DomainMetadata::USERS = "users";
const QString DomainMetadata::Users::NUM_TOTAL = "num_users";
const QString DomainMetadata::Users::NUM_ANON = "num_anon_users";
const QString DomainMetadata::Users::HOSTNAMES = "user_hostnames";
// users metadata will appear as (JSON):
// { "num_users": Number,
//   "num_anon_users": Number,
//   "user_hostnames": { <HOSTNAME>: Number }
// }

const QString DomainMetadata::DESCRIPTORS = "descriptors";
const QString DomainMetadata::Descriptors::NAME = "world_name";
const QString DomainMetadata::Descriptors::DESCRIPTION = "description";
const QString DomainMetadata::Descriptors::THUMBNAIL = "thumbnail";
const QString DomainMetadata::Descriptors::IMAGES = "images";
const QString DomainMetadata::Descriptors::CAPACITY = "capacity"; // parsed from security
const QString DomainMetadata::Descriptors::RESTRICTION = "restriction"; // parsed from ACL
const QString DomainMetadata::Descriptors::MATURITY = "maturity";
const QString DomainMetadata::Descriptors::CONTACT = "contact_info";
const QString DomainMetadata::Descriptors::MANAGERS = "managers";
const QString DomainMetadata::Descriptors::TAGS = "tags";

// descriptors metadata will appear as (JSON):
// { 
//   "world_name": String, // capped name
//   "description": String, // capped description
//   "thumbnail": String, // capped thumbnail URL
//   "images": [ String ], // capped list of image URLs
//   "capacity": Number,
//   "restriction": String, // enum of either open, hifi, or acl
//   "maturity": String, // enum corresponding to ESRB ratings
//   "contact_info": [ String ], // capped list of usernames
//   "managers": [ String ], // capped list of usernames
//   "tags": [ String ], // capped list of tags
// }

// metadata will appear as (JSON):
// { users: <USERS>, descriptors: <DESCRIPTORS> }
//
// it is meant to be sent to and consumed by an external API

DomainMetadata::DomainMetadata(QObject* domainServer) : QObject(domainServer) {
    // set up the structure necessary for casting during parsing
    _metadata[USERS] = QVariantMap {};
    _metadata[DESCRIPTORS] = QVariantMap {};

    // initialize the descriptors
    securityChanged(false);
    descriptorsChanged();
}

QJsonObject DomainMetadata::get() {
    maybeUpdateUsers();
    return QJsonObject::fromVariantMap(_metadata);
}

QJsonObject DomainMetadata::get(const QString& group) {
    maybeUpdateUsers();
    return QJsonObject::fromVariantMap(_metadata[group].toMap());
}

void DomainMetadata::descriptorsChanged() {
    // get descriptors
    assert(_metadata[DESCRIPTORS].canConvert<QVariantMap>());
    auto& state = *static_cast<QVariantMap*>(_metadata[DESCRIPTORS].data());

    static const QString DESCRIPTORS_GROUP_KEYPATH = "descriptors";
    auto descriptorsMap = static_cast<DomainServer*>(parent())->_settingsManager.valueForKeyPath(DESCRIPTORS).toMap();

    // copy simple descriptors
    if (!descriptorsMap[Descriptors::NAME].isNull()) {
        state[Descriptors::NAME] = descriptorsMap[Descriptors::NAME];
    } else {
        state[Descriptors::NAME] = "";
    }

    if (!descriptorsMap[Descriptors::DESCRIPTION].isNull()) {
        state[Descriptors::DESCRIPTION] = descriptorsMap[Descriptors::DESCRIPTION];
    } else {
        state[Descriptors::DESCRIPTION] = "";
    }
    
    if (!descriptorsMap[Descriptors::THUMBNAIL].isNull()) {
        state[Descriptors::THUMBNAIL] = descriptorsMap[Descriptors::THUMBNAIL];
    } else {
        state[Descriptors::THUMBNAIL] = "";
    }
    
    if (!descriptorsMap[Descriptors::MATURITY].isNull()) {
        state[Descriptors::MATURITY] = descriptorsMap[Descriptors::MATURITY];
    } else {
        state[Descriptors::MATURITY] = "";
    }
    
    if (!descriptorsMap[Descriptors::CONTACT].isNull()) {
        state[Descriptors::CONTACT] = descriptorsMap[Descriptors::CONTACT];
    } else {
        state[Descriptors::CONTACT] = "";
    }

    // copy array descriptors
    state[Descriptors::IMAGES] = descriptorsMap[Descriptors::IMAGES].toList();
    state[Descriptors::MANAGERS] = descriptorsMap[Descriptors::MANAGERS].toList();
    state[Descriptors::TAGS] = descriptorsMap[Descriptors::TAGS].toList();

    // parse capacity
    static const QString CAPACITY = "security.maximum_user_capacity";
    QVariant capacityVariant = static_cast<DomainServer*>(parent())->_settingsManager.valueForKeyPath(CAPACITY);
    unsigned int capacity = capacityVariant.isValid() ? capacityVariant.toUInt() : 0;
    state[Descriptors::CAPACITY] = capacity;

#if DEV_BUILD || PR_BUILD
    qDebug() << "Domain metadata descriptors set:" << QJsonObject::fromVariantMap(_metadata[DESCRIPTORS].toMap());
#endif

    sendDescriptors();
}

void DomainMetadata::securityChanged(bool send) {
    // get descriptors
    assert(_metadata[DESCRIPTORS].canConvert<QVariantMap>());
    auto& state = *static_cast<QVariantMap*>(_metadata[DESCRIPTORS].data());

    const QString RESTRICTION_OPEN = "open";
    const QString RESTRICTION_HIFI = "hifi";
    const QString RESTRICTION_ACL = "acl";

    QString restriction;

    const auto& settingsManager = static_cast<DomainServer*>(parent())->_settingsManager;
    bool hasAnonymousAccess = settingsManager.getStandardPermissionsForName(NodePermissions::standardNameAnonymous).can(
        NodePermissions::Permission::canConnectToDomain);
    bool hasHifiAccess = settingsManager.getStandardPermissionsForName(NodePermissions::standardNameLoggedIn).can(
        NodePermissions::Permission::canConnectToDomain);
    if (hasAnonymousAccess) {
        restriction = RESTRICTION_OPEN;
    } else if (hasHifiAccess) {
        restriction = RESTRICTION_HIFI;
    } else {
        restriction = RESTRICTION_ACL;
    }

    state[Descriptors::RESTRICTION] = restriction;

#if DEV_BUILD || PR_BUILD
    qDebug() << "Domain metadata restriction set:" << restriction;
#endif

    if (send) {
        sendDescriptors();
    }
}

void DomainMetadata::usersChanged() {
    ++_tic;

#if DEV_BUILD || PR_BUILD
    qDebug() << "Domain metadata users change detected";
#endif
}

void DomainMetadata::maybeUpdateUsers() {
    if (_lastTic == _tic) {
        return;
    }
    _lastTic = _tic;

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

            if (!nodeData->wasAssigned() && node->getType() == NodeType::Agent) {
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

    assert(_metadata[USERS].canConvert<QVariantMap>());
    auto& users = *static_cast<QVariantMap*>(_metadata[USERS].data());
    users[Users::NUM_TOTAL] = numConnected;
    users[Users::NUM_ANON] = numConnectedAnonymously;
    users[Users::HOSTNAMES] = userHostnames;

#if DEV_BUILD || PR_BUILD
    qDebug() << "Domain metadata users set:" << QJsonObject::fromVariantMap(_metadata[USERS].toMap());
#endif
}

void DomainMetadata::sendDescriptors() {
    QString domainUpdateJSON = QString("{\"domain\":{\"meta\":%1}}").arg(QString(QJsonDocument(get(DESCRIPTORS)).toJson(QJsonDocument::Compact)));
    const QUuid& domainID = DependencyManager::get<LimitedNodeList>()->getSessionUUID();
    if (!domainID.isNull()) {
        static const QString DOMAIN_UPDATE = "/api/v1/domains/%1";
        QString path { DOMAIN_UPDATE.arg(uuidStringWithoutCurlyBraces(domainID)) };
        DependencyManager::get<AccountManager>()->sendRequest(path,
            AccountManagerAuth::Required,
            QNetworkAccessManager::PutOperation,
            JSONCallbackParameters(),
            domainUpdateJSON.toUtf8());

#if DEV_BUILD || PR_BUILD
        qDebug() << "Domain metadata sent to" << path;
        qDebug() << "Domain metadata update:" << domainUpdateJSON;
#endif
    }
}

bool DomainMetadata::handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) {
    QString domainMetadataJSON = QString("{\"domain\":{\"meta\":%1}, \"users\":%2}")
                                     .arg(QString(QJsonDocument(get(DESCRIPTORS)).toJson(QJsonDocument::Compact)))
                                     .arg(QString(QJsonDocument(get(USERS)).toJson(QJsonDocument::Compact)));
    const QString URI_METADATA = "/metadata";
    const QString EXPORTER_MIME_TYPE = "application/json";

    if (url.path() == URI_METADATA) {
        connection->respond(HTTPConnection::StatusCode200, domainMetadataJSON.toUtf8(), qPrintable(EXPORTER_MIME_TYPE));
        return true;
    }

#if DEV_BUILD || PR_BUILD
    qCDebug(domain_metadata_exporter) << "Metadata request on URL " << url;
#endif

    return false;
}
