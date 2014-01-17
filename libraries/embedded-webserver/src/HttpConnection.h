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

#ifndef HTTP_CONNECTION
#define HTTP_CONNECTION

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

/** Header hash. */
typedef QHash<QByteArray, QByteArray> Headers;

/** A form data element. */
typedef QPair<Headers, QByteArray> FormData;

/**
 * Handles a single HTTP connection.
 */
class HttpConnection : public QObject
{
   Q_OBJECT

public:

    /** WebSocket close status codes. */
    enum ReasonCode { NoReason = 0, NormalClosure = 1000, GoingAway = 1001 };

    /**
     * Initializes the connection.
     */
    HttpConnection (QTcpSocket* socket, HttpManager* parentManager);

    /**
     * Destroys the connection.
     */
    virtual ~HttpConnection ();

    /**
     * Returns a pointer to the underlying socket, to which WebSocket message bodies should be
     * written.
     */
    QTcpSocket* socket () const { return _socket; }

    /**
     * Returns the request operation.
     */
    QNetworkAccessManager::Operation requestOperation () const { return _requestOperation; }

    /**
     * Returns a reference to the request URL.
     */
    const QUrl& requestUrl () const { return _requestUrl; }

    /**
     * Returns a reference to the request headers.
     */
    const Headers& requestHeaders () const { return _requestHeaders; }

    /**
     * Returns a reference to the request content.
     */
    const QByteArray& requestContent () const { return _requestContent; }

    /**
     * Checks whether the request is asking to switch to a WebSocket.
     */
    bool isWebSocketRequest ();

    /**
     * Parses the request content as form data, returning a list of header/content pairs.
     */
    QList<FormData> parseFormData () const;

    /**
     * Sends a response and closes the connection.
     */
    void respond (const char* code, const QByteArray& content = QByteArray(),
        const char* contentType = "text/plain; charset=ISO-8859-1",
        const Headers& headers = Headers());

    /**
     * Switches to a WebSocket.
     */
    void switchToWebSocket (const char* protocol = 0);

    /**
     * Writes a header for a WebSocket message of the specified size.  The body of the message
     * should be written through the socket.
     */
    void writeWebSocketHeader (int size) { writeFrameHeader(BinaryFrame, size); }

    /**
     * Pauses or unpauses the WebSocket.  A paused WebSocket buffers messages until unpaused.
     */
    void setWebSocketPaused (bool paused);

    /**
     * Closes the WebSocket.
     */
    void closeWebSocket (quint16 reasonCode = NormalClosure, const char* reason = 0);

signals:

    /**
     * Fired when a WebSocket message of the specified size is available to read.
     */
    void webSocketMessageAvailable (QIODevice* device, int length, bool text);

    /**
     * Fired when the WebSocket has been closed by the other side.
     */
    void webSocketClosed (quint16 reasonCode, QByteArray reason);

protected slots:

    /**
     * Reads the request line.
     */
    void readRequest ();

    /**
     * Reads the headers.
     */
    void readHeaders ();

    /**
     * Reads the content.
     */
    void readContent ();

    /**
     * Reads any incoming WebSocket frames.
     */
    void readFrames ();

protected:

    /** The available WebSocket frame opcodes. */
    enum FrameOpcode { ContinuationFrame, TextFrame, BinaryFrame,
        ConnectionClose = 0x08, Ping, Pong };

    /**
     * Attempts to read a single WebSocket frame, returning true if successful.
     */
    bool maybeReadFrame ();

    /**
     * Writes a WebSocket frame header.
     */
    void writeFrameHeader (FrameOpcode opcode, int size = 0, bool final = true);

    /** The parent HTTP manager. */
    HttpManager* _parentManager;

    /** The underlying socket. */
    QTcpSocket* _socket;

    /** The mask filter for WebSocket frames. */
    MaskFilter* _unmasker;

    /** The data stream for writing to the socket. */
    QDataStream _stream;

    /** The stored address. */
    QHostAddress _address;

    /** The requested operation. */
    QNetworkAccessManager::Operation _requestOperation;

    /** The requested URL. */
    QUrl _requestUrl;

    /** The request headers. */
    Headers _requestHeaders;

    /** The last request header processed (used for continuations). */
    QByteArray _lastRequestHeader;

    /** The content of the request. */
    QByteArray _requestContent;

    /** The opcode for the WebSocket message being continued. */
    FrameOpcode _continuingOpcode;

    /** The WebSocket message being continued. */
    QByteArray _continuingMessage;

    /** Whether or not the WebSocket is paused (buffering messages for future processing). */
    bool _webSocketPaused;

    /** Whether or not we've sent a WebSocket close message. */
    bool _closeSent;
};

/**
 * A filter device that applies a 32-bit mask.
 */
class MaskFilter : public QIODevice
{
    Q_OBJECT

public:

    /**
     * Creates a new masker to filter the supplied device.
     */
    MaskFilter (QIODevice* device, QObject* parent = 0);

    /**
     * Sets the mask to apply.
     */
    void setMask (quint32 mask);

    /**
     * Returns the number of bytes available to read.
     */
    virtual qint64 bytesAvailable () const;

protected:

    /**
     * Reads masked data from the underlying device.
     */
    virtual qint64 readData (char* data, qint64 maxSize);

    /**
     * Writes masked data to the underlying device.
     */
    virtual qint64 writeData (const char* data, qint64 maxSize);

    /** The underlying device. */
    QIODevice* _device;

    /** The current mask. */
    char _mask[4];

    /** The current position within the mask. */
    int _position;
};

#endif // HTTP_CONNECTION
