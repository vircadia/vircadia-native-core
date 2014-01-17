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

#ifndef HTTP_MANAGER
#define HTTP_MANAGER

#include <QByteArray>
#include <QHash>
#include <QtNetwork/QTcpServer>

class HttpConnection;
class HttpRequestHandler;

/**
 * Interface for HTTP request handlers.
 */
class HttpRequestHandler
{
public:

    /**
     * Handles an HTTP request.
     */
    virtual bool handleRequest (
        HttpConnection* connection, const QString& name, const QString& path) = 0;
};

/**
 * Handles requests by forwarding them to subhandlers.
 */
class HttpSubrequestHandler : public HttpRequestHandler
{
public:

    /**
     * Registers a subhandler with the given name.
     */
    void registerSubhandler (const QString& name, HttpRequestHandler* handler);

    /**
     * Handles an HTTP request.
     */
    virtual bool handleRequest (
        HttpConnection* connection, const QString& name, const QString& path);

protected:

    /** Subhandlers mapped by name. */
    QHash<QString, HttpRequestHandler*> _subhandlers;
};

/**
 * Handles HTTP connections.
 */
class HttpManager : public QTcpServer, public HttpSubrequestHandler
{
   Q_OBJECT

public:

    /**
     * Initializes the manager.
     */
    HttpManager(quint16 port, QObject* parent = 0);

protected slots:

    /**
     * Accepts all pending connections.
     */
    void acceptConnections ();
};

#endif // HTTP_MANAGER
