//
//  HTTPManager.h
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Heavily based on Andrzej Kapolka's original HTTPManager class
//  found from another one of his projects.
//  https://github.com/ey6es/witgap/tree/master/src/cpp/server/http
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HTTPManager_h
#define hifi_HTTPManager_h

#include <QtNetwork/QTcpServer>
#include <QtCore/QTimer>

class HTTPConnection;
class HTTPSConnection;

class HTTPRequestHandler {
public:
    /// Handles an HTTP request.
    virtual bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false) = 0;
};

/// Handles HTTP connections
class HTTPManager : public QTcpServer, public HTTPRequestHandler {
   Q_OBJECT
public:
    /// Initializes the manager.
    HTTPManager(const QHostAddress& listenAddress, quint16 port, const QString& documentRoot, HTTPRequestHandler* requestHandler = nullptr);

    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false) override;

private slots:
    void isTcpServerListening();
    void queuedExit(QString errorMessage);
    
private:
    bool bindSocket();
    
protected:
    /// Accepts all pending connections
    virtual void incomingConnection(qintptr socketDescriptor) override;
    virtual bool requestHandledByRequestHandler(HTTPConnection* connection, const QUrl& url);
    
    QHostAddress _listenAddress;
    QString _documentRoot;
    HTTPRequestHandler* _requestHandler;
    QTimer* _isListeningTimer;
    const quint16 _port;
};

#endif // hifi_HTTPManager_h
