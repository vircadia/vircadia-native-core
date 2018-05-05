//
//  HTTPSConnection.cpp
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 2014-04-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HTTPSConnection.h"

#include "EmbeddedWebserverLogging.h"

HTTPSConnection::HTTPSConnection(QSslSocket* sslSocket, HTTPSManager* parentManager) :
    HTTPConnection(sslSocket, parentManager)
{
    connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(handleSSLErrors(const QList<QSslError>&)));
    sslSocket->startServerEncryption();
}

void HTTPSConnection::handleSSLErrors(const QList<QSslError>& errors) {
    qCDebug(embeddedwebserver) << "SSL errors:" << errors;
}
