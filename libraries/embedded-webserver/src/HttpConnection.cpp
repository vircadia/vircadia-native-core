//
//  HttpConnection.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Heavily based on Andrzej Kapolka's original HttpConnection class
//  found from another one of his projects.
//  (https://github.com/ey6es/witgap/tree/master/src/cpp/server/http)
//


#include <QBuffer>
#include <QCryptographicHash>
#include <QTcpSocket>

#include "HttpConnection.h"
#include "HttpManager.h"

HttpConnection::HttpConnection (QTcpSocket* socket, HttpManager* parentManager) :
    QObject(parentManager),
    _parentManager(parentManager),
    _socket(socket),
    _unmasker(new MaskFilter(socket, this)),
    _stream(socket),
    _address(socket->peerAddress()),
    _webSocketPaused(false),
    _closeSent(false)
{
    // take over ownership of the socket
    _socket->setParent(this);

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readRequest()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), SLOT(deleteLater()));

    // log the connection
    qDebug() << "HTTP connection opened." << _address;
}

HttpConnection::~HttpConnection ()
{
    // log the destruction
    QString error;
    QDebug base = qDebug() << "HTTP connection closed." << _address;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }
}

bool HttpConnection::isWebSocketRequest ()
{
    return _requestHeaders.value("Upgrade") == "websocket";
}

QList<FormData> HttpConnection::parseFormData () const
{
    // make sure we have the correct MIME type
    QList<QByteArray> elements = _requestHeaders.value("Content-Type").split(';');
    if (elements.at(0).trimmed() != "multipart/form-data") {
        return QList<FormData>();
    }

    // retrieve the boundary marker
    QByteArray boundary;
    for (int ii = 1, nn = elements.size(); ii < nn; ii++) {
        QByteArray element = elements.at(ii).trimmed();
        if (element.startsWith("boundary")) {
            boundary = element.mid(element.indexOf('=') + 1).trimmed();
            break;
        }
    }
    QByteArray start = "--" + boundary;
    QByteArray end = "\r\n--" + boundary + "--\r\n";

    QList<FormData> data;
    QBuffer buffer(const_cast<QByteArray*>(&_requestContent));
    buffer.open(QIODevice::ReadOnly);
    while (buffer.canReadLine()) {
        QByteArray line = buffer.readLine().trimmed();
        if (line == start) {
            FormData datum;
            while (buffer.canReadLine()) {
                QByteArray line = buffer.readLine().trimmed();
                if (line.isEmpty()) {
                    // content starts after this line
                    int idx = _requestContent.indexOf(end, buffer.pos());
                    if (idx == -1) {
                        qWarning() << "Missing end boundary." << _address;
                        return data;
                    }
                    datum.second = _requestContent.mid(buffer.pos(), idx - buffer.pos());
                    data.append(datum);
                    buffer.seek(idx + end.length());

                } else {
                    // it's a header element
                    int idx = line.indexOf(':');
                    if (idx == -1) {
                        qWarning() << "Invalid header line." << _address << line;
                        continue;
                    }
                    datum.first.insert(line.left(idx).trimmed(), line.mid(idx + 1).trimmed());
                }
            }
        }
    }

    return data;
}

void HttpConnection::respond (
    const char* code, const QByteArray& content, const char* contentType, const Headers& headers)
{
    _socket->write("HTTP/1.1 ");
    _socket->write(code);
    _socket->write("\r\n");

    int csize = content.size();

    for (Headers::const_iterator it = headers.constBegin(), end = headers.constEnd();
            it != end; it++) {
        _socket->write(it.key());
        _socket->write(": ");
        _socket->write(it.value());
        _socket->write("\r\n");
    }
    if (csize > 0) {
        _socket->write("Content-Length: ");
        _socket->write(QByteArray::number(csize));
        _socket->write("\r\n");

        _socket->write("Content-Type: ");
        _socket->write(contentType);
        _socket->write("\r\n");
    }
    _socket->write("Connection: close\r\n\r\n");

    if (csize > 0) {
        _socket->write(content);
    }

    // make sure we receive no further read notifications
    _socket->disconnect(SIGNAL(readyRead()), this);

    _socket->disconnectFromHost();
}

void HttpConnection::switchToWebSocket (const char* protocol)
{
    _socket->write("HTTP/1.1 101 Switching Protocols\r\n");
    _socket->write("Upgrade: websocket\r\n");
    _socket->write("Connection: Upgrade\r\n");
    _socket->write("Sec-WebSocket-Accept: ");

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(_requestHeaders.value("Sec-WebSocket-Key"));
    hash.addData("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"); // from WebSocket draft RFC
    _socket->write(hash.result().toBase64());

    if (protocol != 0) {
        _socket->write("\r\nSec-WebSocket-Protocol: ");
        _socket->write(protocol);
    }
    _socket->write("\r\n\r\n");

    // connect socket, start reading frames
    setWebSocketPaused(false);
}

void HttpConnection::setWebSocketPaused (bool paused)
{
    if ((_webSocketPaused = paused)) {
        _socket->disconnect(this, SLOT(readFrames()));

    } else {
        connect(_socket, SIGNAL(readyRead()), SLOT(readFrames()));
        readFrames();
    }
}

void HttpConnection::closeWebSocket (quint16 reasonCode, const char* reason)
{
    if (reasonCode == NoReason) {
        writeFrameHeader(ConnectionClose);

    } else {
        int rlen = (reason == 0) ? 0 : qstrlen(reason);
        writeFrameHeader(ConnectionClose, 2 + rlen);
        _stream << reasonCode;
        if (rlen > 0) {
            _socket->write(reason);
        }
    }
    _closeSent = true;
}

void HttpConnection::readRequest ()
{
    if (!_socket->canReadLine()) {
        return;
    }
    // parse out the method and resource
    QByteArray line = _socket->readLine().trimmed();
    if (line.startsWith("HEAD")) {
        _requestOperation = QNetworkAccessManager::HeadOperation;

    } else if (line.startsWith("GET")) {
        _requestOperation = QNetworkAccessManager::GetOperation;

    } else if (line.startsWith("PUT")) {
        _requestOperation = QNetworkAccessManager::PutOperation;

    } else if (line.startsWith("POST")) {
        _requestOperation = QNetworkAccessManager::PostOperation;

    } else if (line.startsWith("DELETE")) {
        _requestOperation = QNetworkAccessManager::DeleteOperation;

    } else {
        qWarning() << "Unrecognized HTTP operation." << _address << line;
        respond("400 Bad Request", "Unrecognized operation.");
        return;
    }
    int idx = line.indexOf(' ') + 1;
    _requestUrl.setUrl(line.mid(idx, line.lastIndexOf(' ') - idx));

    // switch to reading the header
    _socket->disconnect(this, SLOT(readRequest()));
    connect(_socket, SIGNAL(readyRead()), SLOT(readHeaders()));

    // read any headers immediately available
    readHeaders();
}

void HttpConnection::readHeaders ()
{
    while (_socket->canReadLine()) {
        QByteArray line = _socket->readLine();
        QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            _socket->disconnect(this, SLOT(readHeaders()));

            QByteArray clength = _requestHeaders.value("Content-Length");
            if (clength.isEmpty()) {
                _parentManager->handleRequest(this, "", _requestUrl.path());

            } else {
                _requestContent.resize(clength.toInt());
                connect(_socket, SIGNAL(readyRead()), SLOT(readContent()));

                // read any content immediately available
                readContent();
            }
            return;
        }
        char first = line.at(0);
        if (first == ' ' || first == '\t') { // continuation
            _requestHeaders[_lastRequestHeader].append(trimmed);
            continue;
        }
        int idx = trimmed.indexOf(':');
        if (idx == -1) {
            qWarning() << "Invalid header." << _address << trimmed;
            respond("400 Bad Request", "The header was malformed.");
            return;
        }
        _lastRequestHeader = trimmed.left(idx);
        QByteArray& value = _requestHeaders[_lastRequestHeader];
        if (!value.isEmpty()) {
            value.append(", ");
        }
        value.append(trimmed.mid(idx + 1).trimmed());
    }
}

void HttpConnection::readContent ()
{
    int size = _requestContent.size();
    if (_socket->bytesAvailable() < size) {
        return;
    }
    _socket->read(_requestContent.data(), size);
    _socket->disconnect(this, SLOT(readContent()));

    _parentManager->handleRequest(this, "", _requestUrl.path());
}

void HttpConnection::readFrames()
{
    // read as many messages as are available
    while (maybeReadFrame());
}

void unget (QIODevice* device, quint32 value) {
    device->ungetChar(value & 0xFF);
    device->ungetChar((value >> 8) & 0xFF);
    device->ungetChar((value >> 16) & 0xFF);
    device->ungetChar(value >> 24);
}

bool HttpConnection::maybeReadFrame ()
{
    // make sure we have at least the first two bytes
    qint64 available = _socket->bytesAvailable();
    if (available < 2 || _webSocketPaused) {
        return false;
    }
    // read the first two, which tell us whether we need more for the length
    quint8 finalOpcode, maskLength;
    _stream >> finalOpcode;
    _stream >> maskLength;
    available -= 2;

    int byteLength = maskLength & 0x7F;
    bool masked = (maskLength & 0x80) != 0;
    int baseLength = (masked ? 4 : 0);
    int length = -1;
    if (byteLength == 127) {
        if (available >= 8) {
            quint64 longLength;
            _stream >> longLength;
            if (available >= baseLength + 8 + longLength) {
                length = longLength;
            } else {
                unget(_socket, longLength & 0xFFFFFFFF);
                unget(_socket, longLength >> 32);
            }
        }
    } else if (byteLength == 126) {
        if (available >= 2) {
            quint16 shortLength;
            _stream >> shortLength;
            if (available >= baseLength + 2 + shortLength) {
                length = shortLength;
            } else {
                _socket->ungetChar(shortLength & 0xFF);
                _socket->ungetChar(shortLength >> 8);
            }
        }
    } else if (available >= baseLength + byteLength) {
        length = byteLength;
    }
    if (length == -1) {
        _socket->ungetChar(maskLength);
        _socket->ungetChar(finalOpcode);
        return false;
    }

    // read the mask and set it in the filter
    quint32 mask = 0;
    if (masked) {
        _stream >> mask;
    }
    _unmasker->setMask(mask);

    // if not final, add to continuing message
    FrameOpcode opcode = (FrameOpcode)(finalOpcode & 0x0F);
    if ((finalOpcode & 0x80) == 0) {
        if (opcode != ContinuationFrame) {
            _continuingOpcode = opcode;
        }
        _continuingMessage += _unmasker->read(length);
        return true;
    }

    // if continuing, add to and read from buffer
    QIODevice* device = _unmasker;
    FrameOpcode copcode = opcode;
    if (opcode == ContinuationFrame) {
        _continuingMessage += _unmasker->read(length);
        device = new QBuffer(&_continuingMessage, this);
        device->open(QIODevice::ReadOnly);
        copcode = _continuingOpcode;
    }

    // act according to opcode
    switch (copcode) {
        case TextFrame:
            emit webSocketMessageAvailable(device, length, true);
            break;

        case BinaryFrame:
            emit webSocketMessageAvailable(device, length, false);
            break;

        case ConnectionClose:
            // if this is not a response to our own close request, send a close reply
            if (!_closeSent) {
                closeWebSocket(GoingAway);
            }
            if (length >= 2) {
                QDataStream stream(device);
                quint16 reasonCode;
                stream >> reasonCode;
                emit webSocketClosed(reasonCode, device->read(length - 2));
            } else {
                emit webSocketClosed(0, QByteArray());
            }
            _socket->disconnectFromHost();
            break;

        case Ping:
            // send the pong out immediately
            writeFrameHeader(Pong, length, true);
            _socket->write(device->read(length));
            break;

        case Pong:
            qWarning() << "Got unsolicited WebSocket pong." << _address << device->read(length);
            break;

        default:
            qWarning() << "Received unknown WebSocket opcode." << _address << opcode <<
                device->read(length);
            break;
    }

    // clear the continuing message buffer
    if (opcode == ContinuationFrame) {
        _continuingMessage.clear();
        delete device;
    }

    return true;
}

void HttpConnection::writeFrameHeader (FrameOpcode opcode, int size, bool final)
{
    if (_closeSent) {
        qWarning() << "Writing frame header after close message." << _address << opcode;
        return;
    }
    _socket->putChar((final ? 0x80 : 0x0) | opcode);
    if (size < 126) {
        _socket->putChar(size);

    } else if (size < 65536) {
        _socket->putChar(126);
        _stream << (quint16)size;

    } else {
        _socket->putChar(127);
        _stream << (quint64)size;
    }
}

MaskFilter::MaskFilter (QIODevice* device, QObject* parent) :
    QIODevice(parent),
    _device(device)
{
    open(ReadOnly);
}

void MaskFilter::setMask (quint32 mask)
{
    _mask[0] = (mask >> 24);
    _mask[1] = (mask >> 16) & 0xFF;
    _mask[2] = (mask >> 8) & 0xFF;
    _mask[3] = mask & 0xFF;
    _position = 0;
    reset();
}

qint64 MaskFilter::bytesAvailable () const
{
    return _device->bytesAvailable() + QIODevice::bytesAvailable();
}

qint64 MaskFilter::readData (char* data, qint64 maxSize)
{
    qint64 bytes = _device->read(data, maxSize);
    for (char* end = data + bytes; data < end; data++) {
        *data ^= _mask[_position];
        _position = (_position + 1) % 4;
    }
    return bytes;
}

qint64 MaskFilter::writeData (const char* data, qint64 maxSize)
{
    return _device->write(data, maxSize);
}
