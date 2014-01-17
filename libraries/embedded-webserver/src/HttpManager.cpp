//
//  HttpManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Heavily based on Andrzej Kapolka's original HttpManager class
//  found from another one of his projects.
//  (https://github.com/ey6es/witgap/tree/master/src/cpp/server/http)
//

#include <QTcpSocket>
#include <QtDebug>

#include "HttpConnection.h"
#include "HttpManager.h"

void HttpSubrequestHandler::registerSubhandler (const QString& name, HttpRequestHandler* handler) {
    _subhandlers.insert(name, handler);
}

bool HttpSubrequestHandler::handleRequest (
    HttpConnection* connection, const QString& name, const QString& path) {
    QString subpath = path;
    if (subpath.startsWith('/')) {
        subpath.remove(0, 1);
    }
    QString subname;
    int idx = subpath.indexOf('/');
    if (idx == -1) {
        subname = subpath;
        subpath = "";
    } else {
        subname = subpath.left(idx);
        subpath = subpath.mid(idx + 1);
    }
    HttpRequestHandler* handler = _subhandlers.value(subname);
    if (handler == 0 || !handler->handleRequest(connection, subname, subpath)) {
        connection->respond("404 Not Found", "Resource not found.");
    }
    return true;
}

HttpManager::HttpManager(quint16 port, QObject* parent) :
    QTcpServer(parent) {
    // start listening on the passed port
    if (!listen(QHostAddress("0.0.0.0"), port)) {
        qDebug() << "Failed to open HTTP server socket:" << errorString();
        return;
    }
    
    // connect the connection signal
    connect(this, SIGNAL(newConnection()), SLOT(acceptConnections()));
}

void HttpManager::acceptConnections () {
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new HttpConnection(socket, this);
    }
}
