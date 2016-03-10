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

HTTPSManager::HTTPSManager(QHostAddress listenAddress, quint16 port, const QSslCertificate& certificate, const QSslKey& privateKey,
                           const QString& documentRoot, HTTPSRequestHandler* requestHandler, QObject* parent) :
    HTTPManager(listenAddress, port, documentRoot, requestHandler, parent),
    _certificate(certificate),
    _privateKey(privateKey),
    _sslRequestHandler(requestHandler)
{
    
}

void HTTPSManager::incomingConnection(qintptr socketDescriptor) {
    QSslSocket* sslSocket = new QSslSocket(this);
    
    sslSocket->setLocalCertificate(_certificate);
    sslSocket->setPrivateKey(_privateKey);
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    
    if (sslSocket->setSocketDescriptor(socketDescriptor)) {
        new HTTPSConnection(sslSocket, this);
    } else {
        delete sslSocket;
    }
}

bool HTTPSManager::handleHTTPRequest(HTTPConnection* connection, const QUrl &url, bool skipSubHandler) {
    return handleHTTPSRequest(reinterpret_cast<HTTPSConnection*>(connection), url, skipSubHandler);
}

bool HTTPSManager::handleHTTPSRequest(HTTPSConnection* connection, const QUrl& url, bool skipSubHandler) {
    return HTTPManager::handleHTTPRequest(connection, url, skipSubHandler);
}

bool HTTPSManager::requestHandledByRequestHandler(HTTPConnection* connection, const QUrl& url) {
    return _sslRequestHandler && _sslRequestHandler->handleHTTPSRequest(reinterpret_cast<HTTPSConnection*>(connection), url);
}
