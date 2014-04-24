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

class HTTPSManager : public HTTPManager {
    Q_OBJECT
public:
    HTTPSManager(quint16 port,
                 const QString& documentRoot,
                 HTTPRequestHandler* requestHandler = NULL, QObject* parent = 0);
    
    void setCertificate(const QSslCertificate& certificate) { _certificate = certificate; }
    void setPrivateKey(const QSslKey& privateKey) { _privateKey = privateKey; }
    
protected:
    virtual void incomingConnection(qintptr socketDescriptor);
private:
    QSslCertificate _certificate;
    QSslKey _privateKey;
};

#endif // hifi_HTTPSManager_h