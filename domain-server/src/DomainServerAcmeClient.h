//
//  DomainServerAcmeClient.h
//  domain-server/src
//
//  Created by Nshan G. on 2021-11-15.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef VIRCADIA_DOMAIN_SERVER_SRC_DOMAINSERVERACMECLIENT_H
#define VIRCADIA_DOMAIN_SERVER_SRC_DOMAINSERVERACMECLIENT_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(acme_client)

class HTTPConnection;

class AcmeChallengeHandler {
public:
    virtual void addChallenge(const std::string& domain, const std::string& location, const std::string& content) = 0;
    virtual ~AcmeChallengeHandler(){};
};

class DomainServerAcmeClient : public QObject {
    Q_OBJECT
public:
    DomainServerAcmeClient();
    bool handleAuthenticatedHTTPRequest(HTTPConnection* connection, const QUrl& url);

signals:
    void certificatesUpdated();
    void serverKeyUpdated();

private slots:
    void certificateExpiryTimerHandler();

private:

    QTimer expiryTimer;
    std::unique_ptr<AcmeChallengeHandler> challengeHandler;
    std::vector<std::string> selfCheckUrls;
};


#endif /* end of include guard */
