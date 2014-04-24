//
//  HTTPSManager.cpp
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 2014-04-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtNetwork/QSslSocket>

#include "HTTPSConnection.h"

#include "HTTPSManager.h"

HTTPSManager::HTTPSManager(quint16 port, const QSslCertificate& certificate, const QSslKey& privateKey,
                           const QString& documentRoot, HTTPRequestHandler* requestHandler, QObject* parent) :
    HTTPManager(port, documentRoot, requestHandler, parent),
    _certificate(certificate),
    _privateKey(privateKey)
{
    
}

void HTTPSManager::incomingConnection(qintptr socketDescriptor) {
    QSslSocket* sslSocket = new QSslSocket(this);
    
    if (sslSocket->setSocketDescriptor(socketDescriptor)) {
        new HTTPSConnection(sslSocket, this);
    } else {
        delete sslSocket;
    }
}