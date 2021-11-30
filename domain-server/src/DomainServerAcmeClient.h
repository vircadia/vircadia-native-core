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

#include <HTTPConnection.h>

Q_DECLARE_LOGGING_CATEGORY(acme_client)

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

    QTimer _expiryTimer;
};


#endif /* end of include guard */
