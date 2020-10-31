//
//  DomainMetadata.h
//  domain-server/src
//
//  Created by Zach Pomerantz on 5/25/2016.
//  Copyright 2016 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#ifndef hifi_DomainMetadata_h
#define hifi_DomainMetadata_h

#include <stdint.h>

#include <QVariantMap>
#include <QJsonObject>
#include "HTTPManager.h"

class DomainMetadata : public QObject, public HTTPRequestHandler {
Q_OBJECT

public:
    using Tic = uint32_t;

    static const QString USERS;
    class Users {
    public:
        static const QString NUM_TOTAL;
        static const QString NUM_ANON;
        static const QString HOSTNAMES;
    };

    static const QString DESCRIPTORS;
    class Descriptors {
    public:
        static const QString NAME;
        static const QString DESCRIPTION;
        static const QString THUMBNAIL;
        static const QString IMAGES;
        static const QString CAPACITY;
        static const QString RESTRICTION;
        static const QString MATURITY;
        static const QString CONTACT;
        static const QString MANAGERS;
        static const QString TAGS;
    };

    DomainMetadata(QObject* domainServer);
    ~DomainMetadata() = default;
    // Get cached metadata
    QJsonObject get();
    QJsonObject get(const QString& group);

    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false) override;

public slots:
    void descriptorsChanged();
    void securityChanged(bool send);
    void usersChanged();

protected:
    void maybeUpdateUsers();
    void sendDescriptors();

    QVariantMap _metadata;
    uint32_t _lastTic{ (uint32_t)-1 };
    uint32_t _tic{ 0 };
};

#endif // hifi_DomainMetadata_h
