//
//  DomainMetadata.h
//  domain-server/src
//
//  Created by Zach Pomerantz on 5/25/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#ifndef hifi_DomainMetadata_h
#define hifi_DomainMetadata_h

#include <stdint.h>

#include <QVariantMap>
#include <QJsonObject>

class DomainMetadata : public QObject {
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
        static const QString DESCRIPTION;
        static const QString CAPACITY;
        static const QString RESTRICTION;
        static const QString MATURITY;
        static const QString HOSTS;
        static const QString TAGS;
        static const QString HOURS;
        class Hours {
        public:
            static const QString WEEKDAY;
            static const QString WEEKEND;
            static const QString UTC_OFFSET;
            static const QString OPEN;
            static const QString CLOSE;
        };
    };

    DomainMetadata(QObject* domainServer);
    DomainMetadata() = delete;

    // Get cached metadata
    QJsonObject get();
    QJsonObject get(const QString& group);

public slots:
    void descriptorsChanged();
    void securityChanged(bool send);
    void securityChanged() { securityChanged(true); }
    void usersChanged();

protected:
    void maybeUpdateUsers();
    void sendDescriptors();

    QVariantMap _metadata;
    uint32_t _lastTic{ (uint32_t)-1 };
    uint32_t _tic{ 0 };
};

#endif // hifi_DomainMetadata_h
