//
//  HttpConnection.h
//  hifi
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Heavily based on Andrzej Kapolka's original HttpConnection class
//  found from another one of his projects.
//  (https://github.com/ey6es/witgap/tree/master/src/cpp/server/http)
//

#ifndef __hifi__HttpConnection__
#define __hifi__HttpConnection__

#include <QHash>
#include <QHostAddress>
#include <QIODevice>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPair>
#include <QUrl>

class QTcpSocket;
class HttpManager;
class MaskFilter;
class ServerApp;

/// Header hash
typedef QHash<QByteArray, QByteArray> Headers;

/// A form data element
typedef QPair<Headers, QByteArray> FormData;

/// Handles a single HTTP connection.
class HttpConnection : public QObject {
   Q_OBJECT

public:

    /// WebSocket close status codes.
    enum ReasonCode { NoReason = 0, NormalClosure = 1000, GoingAway = 1001 };

    /// Initializes the connection.
    HttpConnection (QTcpSocket* socket, HttpManager* parentManager);

    /// Destroys the connection.
    virtual ~HttpConnection ();

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
        const char* contentType = "text/plain; charset=ISO-8859-1",
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
    HttpManager* _parentManager;

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

#endif /* defined(__hifi__HttpConnection__) */
