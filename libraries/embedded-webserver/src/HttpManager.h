//
//  HttpManager.h
//  hifi
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Heavily based on Andrzej Kapolka's original HttpManager class
//  found from another one of his projects.
//  (https://github.com/ey6es/witgap/tree/master/src/cpp/server/http)
//

#ifndef __hifi__HttpManager__
#define __hifi__HttpManager__

#include <QByteArray>
#include <QHash>
#include <QtNetwork/QTcpServer>

class HttpConnection;
class HttpRequestHandler;

/// Handles HTTP connections
class HttpManager : public QTcpServer {
   Q_OBJECT

public:

    /// Initializes the manager.
    HttpManager(quint16 port, const QString& documentRoot, QObject* parent = 0);
    
    /// Handles an HTTP request.
    virtual bool handleRequest (HttpConnection* connection, const QString& path);
    
protected slots:
    /// Accepts all pending connections
    void acceptConnections();
protected:
    QString _documentRoot;
};

#endif /* defined(__hifi__HttpManager__) */
