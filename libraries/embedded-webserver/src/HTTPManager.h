//
//  HTTPManager.h
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__HTTPManager__
#define __hifi__HTTPManager__

#include <QtNetwork/QTcpServer>

class HTTPConnection;

class HTTPRequestHandler {
public:
    /// Handles an HTTP request.
    virtual bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url) = 0;
};

/// Handles HTTP connections
class HTTPManager : public QTcpServer, public HTTPRequestHandler {
   Q_OBJECT
public:
    /// Initializes the manager.
    HTTPManager(quint16 port, const QString& documentRoot, HTTPRequestHandler* requestHandler = NULL, QObject* parent = 0);
    
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url);
    
protected slots:
    /// Accepts all pending connections
    void acceptConnections();
protected:
    QString _documentRoot;
    HTTPRequestHandler* _requestHandler;
};

#endif /* defined(__hifi__HTTPManager__) */
