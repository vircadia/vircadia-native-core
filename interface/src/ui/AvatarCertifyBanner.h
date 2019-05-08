//
//  AvatarCertifyBanner.h
//  interface/src/ui
//
//  Created by Simon Walton, April 2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarCertifyBanner_h
#define hifi_AvatarCertifyBanner_h

#include <QUuid>
#include "OffscreenQmlElement.h"
#include "EntityItemID.h"

class EntityItemID;

class AvatarCertifyBanner : QObject {
    Q_OBJECT
    HIFI_QML_DECL
public:
    AvatarCertifyBanner(QQuickItem* parent = nullptr);
    void show(const QUuid& avatarID);
    void clear();

private:
    const EntityItemID _bannerID { QUuid::createUuid() };
    bool _active { false };
};

#endif  // hifi_AvatarCertifyBanner_h
