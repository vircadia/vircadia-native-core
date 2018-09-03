//
//  HTTPSConnection.h
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 2014-04-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HTTPSConnection_h
#define hifi_HTTPSConnection_h

#include "HTTPConnection.h"
#include "HTTPSManager.h"

class HTTPSConnection : public HTTPConnection {
    Q_OBJECT
public:
    HTTPSConnection(QSslSocket* sslSocket, HTTPSManager* parentManager);
protected slots:
    void handleSSLErrors(const QList<QSslError>& errors);
};

#endif // hifi_HTTPSConnection_h
