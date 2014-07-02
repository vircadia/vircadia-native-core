//
//  NetworkAccessManager.h
//
//
//  Created by Clement on 7/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkAccessManager_h
#define hifi_NetworkAccessManager_h

#include <QNetworkAccessManager>
#include <QNetworkConfiguration>
#include <QNetworkProxy>

/// Wrapper around QNetworkAccessManager wo that we only use one instance
class NetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    static NetworkAccessManager& getInstance();
    
    Q_INVOKABLE QNetworkReply* get(const QNetworkRequest& request);
    Q_INVOKABLE QNetworkReply* head(const QNetworkRequest& request);
    Q_INVOKABLE QNetworkReply* post(const QNetworkRequest& request, QIODevice* data);
    Q_INVOKABLE QNetworkReply* post(const QNetworkRequest& request, const QByteArray& data);
    Q_INVOKABLE QNetworkReply* post(const QNetworkRequest& request, QHttpMultiPart* multiPart);
    Q_INVOKABLE QNetworkReply* put(const QNetworkRequest& request, QIODevice* data);
    Q_INVOKABLE QNetworkReply* put(const QNetworkRequest& request, QHttpMultiPart* multiPart);
    Q_INVOKABLE QNetworkReply* put(const QNetworkRequest& request, const QByteArray& data);
    
private:
    NetworkAccessManager();
};

#endif // hifi_NetworkAccessManager_h