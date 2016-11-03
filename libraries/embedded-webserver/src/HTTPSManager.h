//
//  HTTPSManager.h
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 2014-04-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HTTPSManager_h
#define hifi_HTTPSManager_h

#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslCertificate>

#include "HTTPManager.h"

class HTTPSRequestHandler : public HTTPRequestHandler {
public:
    /// Handles an HTTPS request
    virtual bool handleHTTPSRequest(HTTPSConnection* connection, const QUrl& url, bool skipSubHandler = false) = 0;
};

class HTTPSManager : public HTTPManager, public HTTPSRequestHandler {
    Q_OBJECT
public:
    HTTPSManager(QHostAddress listenAddress,
                 quint16 port,
                 const QSslCertificate& certificate,
                 const QSslKey& privateKey,
                 const QString& documentRoot,
                 HTTPSRequestHandler* requestHandler = NULL, QObject* parent = 0);
    
    void setCertificate(const QSslCertificate& certificate) { _certificate = certificate; }
    void setPrivateKey(const QSslKey& privateKey) { _privateKey = privateKey; }
    
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false) override;
    bool handleHTTPSRequest(HTTPSConnection* connection, const QUrl& url, bool skipSubHandler = false) override;
    
protected:
    void incomingConnection(qintptr socketDescriptor) override;
    bool requestHandledByRequestHandler(HTTPConnection* connection, const QUrl& url) override;
private:
    QSslCertificate _certificate;
    QSslKey _privateKey;
    HTTPSRequestHandler* _sslRequestHandler;
};

#endif // hifi_HTTPSManager_h