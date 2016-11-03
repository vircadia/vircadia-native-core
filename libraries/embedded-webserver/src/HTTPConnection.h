//
//  HTTPConnection.h
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Heavily based on Andrzej Kapolka's original HTTPConnection class
//  found from another one of his projects.
//  https://github.com/ey6es/witgap/tree/master/src/cpp/server/http
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HTTPConnection_h
#define hifi_HTTPConnection_h

#include <QDataStream>
#include <QHash>
#include <QtNetwork/QHostAddress>
#include <QIODevice>
#include <QList>
#include <QtNetwork/QNetworkAccessManager>
#include <QObject>
#include <QPair>
#include <QUrl>

class QTcpSocket;
class HTTPManager;
class MaskFilter;
class ServerApp;

/// Header hash
typedef QHash<QByteArray, QByteArray> Headers;

/// A form data element
typedef QPair<Headers, QByteArray> FormData;

/// Handles a single HTTP connection.
class HTTPConnection : public QObject {
   Q_OBJECT

public:
    static const char* StatusCode200;
    static const char* StatusCode301;
    static const char* StatusCode302;
    static const char* StatusCode400;
    static const char* StatusCode401;
    static const char* StatusCode403;
    static const char* StatusCode404;
    static const char* StatusCode500;
    static const char* DefaultContentType;

    /// WebSocket close status codes.
    enum ReasonCode { NoReason = 0, NormalClosure = 1000, GoingAway = 1001 };

    /// Initializes the connection.
    HTTPConnection (QTcpSocket* socket, HTTPManager* parentManager);

    /// Destroys the connection.
    virtual ~HTTPConnection ();

    /// Returns a pointer to the underlying socket, to which WebSocket message bodies should be written.
    QTcpSocket* socket () const { return _socket; }

    /// Returns the request operation.
    QNetworkAccessManager::Operation requestOperation () const { return _requestOperation; }

    /// Returns a reference to the request URL.
    const QUrl& requestUrl () const { return _requestUrl; }

    /// Returns a reference to the request headers.
    const Headers& requestHeaders () const { return _requestHeaders; }

    /// Returns a reference to the request content.
    const QByteArray& requestContent () const { return _requestContent; }

    /// Parses the request content as form data, returning a list of header/content pairs.
    QList<FormData> parseFormData () const;

    /// Sends a response and closes the connection.
    void respond (const char* code, const QByteArray& content = QByteArray(),
        const char* contentType = DefaultContentType,
        const Headers& headers = Headers());

protected slots:

    /// Reads the request line.
    void readRequest ();

    /// Reads the headers.
    void readHeaders ();

    /// Reads the content.
    void readContent ();

protected:

    /// The parent HTTP manager
    HTTPManager* _parentManager;

    /// The underlying socket.
    QTcpSocket* _socket;
    
    /// The data stream for writing to the socket.
    QDataStream _stream;

    /// The stored address.
    QHostAddress _address;

    /// The requested operation.
    QNetworkAccessManager::Operation _requestOperation;

    /// The requested URL.
    QUrl _requestUrl;

    /// The request headers.
    Headers _requestHeaders;

    /// The last request header processed (used for continuations).
    QByteArray _lastRequestHeader;

    /// The content of the request.
    QByteArray _requestContent;
};

#endif // hifi_HTTPConnection_h
