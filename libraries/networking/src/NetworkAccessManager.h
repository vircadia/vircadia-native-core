//
//  NetworkAccessManager.h
//  libraries/networking/src
//
//  Created by Clement on 7/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkAccessManager_h
#define hifi_NetworkAccessManager_h

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtQml/QQmlNetworkAccessManagerFactory>

/// Wrapper around QNetworkAccessManager to restrict at one instance by thread
class NetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    static QNetworkAccessManager& getInstance();
protected:
    NetworkAccessManager(QObject* parent = Q_NULLPTR) : QNetworkAccessManager(parent) {}
    virtual QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* device = Q_NULLPTR) override;
};

#endif // hifi_NetworkAccessManager_h