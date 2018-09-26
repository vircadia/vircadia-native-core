//
//  ResourceAccessMonitor.h
//  libraries/networking/src
//
//  Created by Kerry Ivan Kurian on 9/27/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QUrl>
#include "ResourceRequestObserver.h"
#include "QUrlAncestry.h"


// void ResourceRequestObserver::update(const QNetworkRequest& request, const qint64 callerId, const QString& extra) {
//     update(QUrlAncestry(request.url()), callerId, extra);
// }

void ResourceRequestObserver::update(
    const QUrlAncestry& urlAncestry,
    const qint64 callerId,
    const QString& extra
) {
    QJsonArray array;
    urlAncestry.toJson(array);
    QJsonObject data {
        { "url", array },
        { "callerId", callerId },
        { "extra", extra }
    };
    emit resourceRequestEvent(data.toVariantMap());
}
