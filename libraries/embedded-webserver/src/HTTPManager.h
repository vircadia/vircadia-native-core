//
//  HTTPManager.h
//  hifi
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Heavily based on Andrzej Kapolka's original HTTPManager class
//  found from another one of his projects.
//  https://github.com/ey6es/witgap/tree/master/src/cpp/server/http
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
