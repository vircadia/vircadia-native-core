//
//  ResourceRequestObserver.h
//  libraries/commerce/src
//
//  Created by Kerry Ivan Kurian on 9/27/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QJsonObject>
#include <QString>
#include <QNetworkRequest>

#include "DependencyManager.h"
#include "QUrlAncestry.h"


class ResourceRequestObserver : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    // void update(const QNetworkRequest& request, const qint64 callerId = -1, const QString& extra = "");
    void update(const QUrlAncestry& urlAncestry, const qint64 callerId = -1, const QString& extra = "");

signals:
    void resourceRequestEvent(QVariantMap result);
};
