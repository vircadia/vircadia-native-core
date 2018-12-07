//
//  ResourceRequestObserver.cpp
//  libraries/shared/src
//
//  Created by Kerry Ivan Kurian on 9/27/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceRequestObserver.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QUrl>

void ResourceRequestObserver::update(const QUrl& requestUrl,
    const qint64 callerId,
    const QString& extra) {
    QJsonArray array;
    QJsonObject data { { "url", requestUrl.toString() },
        { "callerId", callerId },
        { "extra", extra }
    };
    emit resourceRequestEvent(data.toVariantMap());
}
