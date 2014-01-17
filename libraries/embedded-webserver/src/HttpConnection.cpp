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
    _stream(socket),
    _address(socket->peerAddress()) {
        
    // take over ownership of the socket
    _socket->setParent(this);

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readRequest()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), SLOT(deleteLater()));
}

HttpConnection::~HttpConnection() {
    // log the destruction
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        qDebug() << _socket->errorString();
    }
}

QList<FormData> HttpConnection::parseFormData() const {
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

void HttpConnection::respond(const char* code, const QByteArray& content, const char* contentType, const Headers& headers) {
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

void HttpConnection::readRequest() {
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

void HttpConnection::readHeaders() {
    while (_socket->canReadLine()) {
        QByteArray line = _socket->readLine();
        QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            _socket->disconnect(this, SLOT(readHeaders()));

            QByteArray clength = _requestHeaders.value("Content-Length");
            if (clength.isEmpty()) {
                _parentManager->handleRequest(this, _requestUrl.path());

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

void HttpConnection::readContent() {
    int size = _requestContent.size();
    if (_socket->bytesAvailable() < size) {
        return;
    }
    _socket->read(_requestContent.data(), size);
    _socket->disconnect(this, SLOT(readContent()));

    _parentManager->handleRequest(this, _requestUrl.path());
}
