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

#include <QHash>
#include <QIODevice>
#include <QList>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkAccessManager>
#include <QObject>
#include <QPair>
#include <QTemporaryFile>
#include <QUrl>

#include <memory>

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
    static const char* StatusCode204;
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

    class Storage {
    public:
        Storage() = default;
        virtual ~Storage() = default;

        virtual const QByteArray& content() const = 0;

        virtual qint64 bytesLeftToWrite() const = 0;
        virtual void write(const QByteArray& data) = 0;
    };

    /// Initializes the connection.
    HTTPConnection(QTcpSocket* socket, HTTPManager* parentManager);

    /// Destroys the connection.
    virtual ~HTTPConnection();

    /// Returns a pointer to the underlying socket, to which WebSocket message bodies should be written.
    QTcpSocket* socket() const { return _socket; }

    /// Returns the IP address on the other side of the connection
    const QHostAddress &peerAddress() const { return _address; }

    /// Returns the request operation.
    QNetworkAccessManager::Operation requestOperation() const { return _requestOperation; }

    /// Returns a reference to the request URL.
    const QUrl& requestUrl() const { return _requestUrl; }

    /// Returns a copy of the request header value. If it does not exist, it will return a default constructed QByteArray.
    QByteArray requestHeader(const QString& key) const { return _requestHeaders.value(key.toLower().toLocal8Bit()); }

    /// Returns a reference to the request content.
    const QByteArray& requestContent() const { return _requestContent->content(); }

    /// Parses the request content as form data, returning a list of header/content pairs.
    QList<FormData> parseFormData() const;

    /// Parses the request content as a url encoded form, returning a hash of key/value pairs.
    /// Duplicate keys are not supported.
    QHash<QString, QString> parseUrlEncodedForm();

    /// Sends a response and closes the connection.
    void respond(const char* code, const QByteArray& content = QByteArray(),
        const char* contentType = DefaultContentType,
        const Headers& headers = Headers());
    void respond(const char* code, std::unique_ptr<QIODevice> device,
        const char* contentType = DefaultContentType,
        const Headers& headers = Headers());

protected slots:

    /// Reads the request line.
    void readRequest();

    /// Reads the headers.
    void readHeaders();

    /// Reads the content.
    void readContent();

protected:
    void respondWithStatusAndHeaders(const char* code, const char* contentType, const Headers& headers, qint64 size);

    /// The parent HTTP manager
    HTTPManager* _parentManager;

    /// The underlying socket.
    QTcpSocket* _socket;

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
    std::unique_ptr<Storage> _requestContent;

    /// Response content
    std::unique_ptr<QIODevice> _responseDevice;
};

#endif // hifi_HTTPConnection_h
