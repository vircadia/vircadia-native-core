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
    HTTPManager(quint16 port, const QString& documentRoot, HTTPRequestHandler* requestHandler = NULL, QObject* parent = 0);
    
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false);
    
protected:
    /// Accepts all pending connections
    virtual void incomingConnection(qintptr socketDescriptor);
    virtual bool requestHandledByRequestHandler(HTTPConnection* connection, const QUrl& url);
protected:
    QString _documentRoot;
    HTTPRequestHandler* _requestHandler;
};

#endif // hifi_HTTPManager_h
