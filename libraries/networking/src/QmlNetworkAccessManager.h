//
//  QmlNetworkAccessManager.h
//
//
//  Created by Clement on 7/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QmlNetworkAccessManager_h
#define hifi_QmlNetworkAccessManager_h

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtQml/QQmlNetworkAccessManagerFactory>

class QmlNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory {
public:
    QNetworkAccessManager* create(QObject* parent);
};


class QmlNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    QmlNetworkAccessManager(QObject* parent = Q_NULLPTR) : QNetworkAccessManager(parent) { }
protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* device = Q_NULLPTR);
};

#endif // hifi_QmlNetworkAccessManager_h
