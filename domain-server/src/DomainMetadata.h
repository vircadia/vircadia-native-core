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

#include <QVariantMap>
#include <QJsonObject>

class DomainMetadata {
public:
    QVariantMap toVariantMap() { return _metadata; }
    QJsonObject toJSON() { return QJsonObject::fromVariantMap(_metadata); }

public slots:
    void usersChanged();

protected:
    QVariantMap _metadata;
};

#endif // hifi_DomainMetadata_h
