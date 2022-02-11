//
//  HTTPConnection.cpp
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HTTPConnection.h"

#include <assert.h>

#include <QBuffer>
#include <QCryptographicHash>
#include <QTcpSocket>
#include <QUrlQuery>

#include "EmbeddedWebserverLogging.h"
#include "HTTPManager.h"

const char* HTTPConnection::StatusCode200 = "200 OK";
const char* HTTPConnection::StatusCode204 = "204 No Content";
const char* HTTPConnection::StatusCode301 = "301 Moved Permanently";
const char* HTTPConnection::StatusCode302 = "302 Found";
const char* HTTPConnection::StatusCode400 = "400 Bad Request";
const char* HTTPConnection::StatusCode401 = "401 Unauthorized";
const char* HTTPConnection::StatusCode403 = "403 Forbidden";
const char* HTTPConnection::StatusCode404 = "404 Not Found";
const char* HTTPConnection::StatusCode500 = "500 Internal server error";
const char* HTTPConnection::DefaultContentType = "text/plain; charset=ISO-8859-1";


class MemoryStorage : public HTTPConnection::Storage {
public:
    static std::unique_ptr<MemoryStorage> make(qint64 size);
    virtual ~MemoryStorage() = default;

    const QByteArray& content() const override { return _array; }
    qint64 bytesLeftToWrite() const override { return _array.size() - _bytesWritten; }
    void write(const QByteArray& data) override;

private:
    MemoryStorage(qint64 size) { _array.resize(size); }

    QByteArray _array;
    qint64 _bytesWritten { 0 };
};

std::unique_ptr<MemoryStorage> MemoryStorage::make(qint64 size) {
    return std::unique_ptr<MemoryStorage>(new MemoryStorage(size));
}

void MemoryStorage::write(const QByteArray& data) {
    assert(data.size() <= bytesLeftToWrite());
    memcpy(_array.data() + _bytesWritten, data.data(), data.size());
    _bytesWritten += data.size();
}


class FileStorage : public HTTPConnection::Storage {
public:
    static std::unique_ptr<FileStorage> make(qint64 size);
    virtual ~FileStorage();

    const QByteArray& content() const override { return _wrapperArray; };
    qint64 bytesLeftToWrite() const override { return _mappedMemorySize - _bytesWritten; }
    void write(const QByteArray& data) override;

private:
    FileStorage(std::unique_ptr<QTemporaryFile> file, uchar* mapped, qint64 size);

    // Byte array is const because any edit will trigger a deep copy
    // and pull all the data we want to keep on disk in memory.
    const QByteArray _wrapperArray;
    std::unique_ptr<QTemporaryFile> _file;

    uchar* const _mappedMemoryAddress { nullptr };
    const qint64 _mappedMemorySize { 0 };
    qint64 _bytesWritten { 0 };
};

std::unique_ptr<FileStorage> FileStorage::make(qint64 size) {
    auto file = std::unique_ptr<QTemporaryFile>(new QTemporaryFile());
    file->open(); // Open for resize
    file->resize(size);
    auto mapped = file->map(0, size); // map the entire file

    return std::unique_ptr<FileStorage>(new FileStorage(std::move(file), mapped, size));
}

// Use QByteArray::fromRawData to avoid a new allocation and access the already existing
// memory directly as long as all operations on the array are const.
FileStorage::FileStorage(std::unique_ptr<QTemporaryFile> file, uchar* mapped, qint64 size) :
    _wrapperArray(QByteArray::fromRawData(reinterpret_cast<char*>(mapped), size)),
    _file(std::move(file)),
    _mappedMemoryAddress(mapped),
    _mappedMemorySize(size)
{
}

FileStorage::~FileStorage() {
    _file->unmap(_mappedMemoryAddress);
    _file->close();
}

void FileStorage::write(const QByteArray& data) {
    assert(data.size() <= bytesLeftToWrite());
    // We write directly to the mapped memory
    memcpy(_mappedMemoryAddress + _bytesWritten, data.data(), data.size());
    _bytesWritten += data.size();
}


HTTPConnection::HTTPConnection(QTcpSocket* socket, HTTPManager* parentManager) :
    QObject(parentManager),
    _parentManager(parentManager),
    _socket(socket),
    _address(socket->peerAddress())
{
    // take over ownership of the socket
    _socket->setParent(this);

    // connect initial slots
    connect(socket, &QAbstractSocket::readyRead, this, &HTTPConnection::readRequest);
    connect(socket, &QAbstractSocket::errorOccurred, this, &HTTPConnection::deleteLater);
    connect(socket, &QAbstractSocket::disconnected, this, &HTTPConnection::deleteLater);
}

HTTPConnection::~HTTPConnection() {
    // log the destruction
    if (_socket->error() != QAbstractSocket::UnknownSocketError
        && _socket->error() != QAbstractSocket::RemoteHostClosedError) {
        qCDebug(embeddedwebserver) << _socket->errorString() << "-" << _socket->error();
    }
}

QHash<QString, QString> HTTPConnection::parseUrlEncodedForm() {
    // make sure we have the correct MIME type
    QList<QByteArray> elements = requestHeader("Content-Type").split(';');

    QString contentType = elements.at(0).trimmed();
    if (contentType != "application/x-www-form-urlencoded") {
        return QHash<QString, QString>();
    }

    QUrlQuery form { _requestContent->content() };
    QHash<QString, QString> pairs;
    for (auto pair : form.queryItems()) {
        auto key = QUrl::fromPercentEncoding(pair.first.toLatin1().replace('+', ' '));
        auto value = QUrl::fromPercentEncoding(pair.second.toLatin1().replace('+', ' '));
        pairs[key] = value;
    }

    return pairs;
}

QList<FormData> HTTPConnection::parseFormData() const {
    // make sure we have the correct MIME type
    QList<QByteArray> elements = requestHeader("Content-Type").split(';');

    QString contentType = elements.at(0).trimmed();

    if (contentType != "multipart/form-data") {
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
    QBuffer buffer(const_cast<QByteArray*>(&_requestContent->content()));
    buffer.open(QIODevice::ReadOnly);
    while (buffer.canReadLine()) {
        QByteArray line = buffer.readLine().trimmed();
        if (line == start) {
            FormData datum;
            while (buffer.canReadLine()) {
                QByteArray line = buffer.readLine().trimmed();
                if (line.isEmpty()) {
                    // content starts after this line
                    int idx = _requestContent->content().indexOf(end, buffer.pos());
                    if (idx == -1) {
                        qWarning() << "Missing end boundary." << _address;
                        return data;
                    }
                    datum.second = QByteArray::fromRawData(_requestContent->content().data() + buffer.pos(),
                                                           idx - buffer.pos());
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

void HTTPConnection::respond(const char* code, const QByteArray& content, const char* contentType, const Headers& headers) {
    respondWithStatusAndHeaders(code, contentType, headers, content.size());

    _socket->write(content);

    _socket->disconnectFromHost();

    // make sure we receive no further read notifications
    disconnect(_socket, &QTcpSocket::readyRead, this, nullptr);
}

void HTTPConnection::respond(const char* code, std::unique_ptr<QIODevice> device, const char* contentType, const Headers& headers) {
    _responseDevice = std::move(device);

    if (_responseDevice->isSequential()) {
        qWarning() << "Error responding to HTTPConnection: sequential IO devices not supported";
        respondWithStatusAndHeaders(StatusCode500, contentType, headers, 0);
        _socket->disconnect(SIGNAL(readyRead()), this);
        _socket->disconnectFromHost();
        return;
    }

    int totalToBeWritten = _responseDevice->size();
    respondWithStatusAndHeaders(code, contentType, headers, totalToBeWritten);

    if (_responseDevice->atEnd()) {
        _socket->disconnectFromHost();
    } else {
        connect(_socket, &QTcpSocket::bytesWritten, this, [this, totalToBeWritten](size_t bytes) mutable {
            constexpr size_t HTTP_RESPONSE_CHUNK_SIZE = 1024 * 10;
            if (!_responseDevice->atEnd()) {
                totalToBeWritten -= _socket->write(_responseDevice->read(HTTP_RESPONSE_CHUNK_SIZE));
                if (_responseDevice->atEnd()) {
                    _socket->disconnectFromHost();
                    disconnect(_socket, &QTcpSocket::bytesWritten, this, nullptr);
                }
            }
        });

    }

    // make sure we receive no further read notifications
    disconnect(_socket, &QTcpSocket::readyRead, this, nullptr);
}

void HTTPConnection::respondWithStatusAndHeaders(const char* code, const char* contentType, const Headers& headers, qint64 contentLength) {
    _socket->write("HTTP/1.1 ");

    _socket->write(code);
    _socket->write("\r\n");

    for (Headers::const_iterator it = headers.constBegin(), end = headers.constEnd();
            it != end; it++) {
        _socket->write(it.key());
        _socket->write(": ");
        _socket->write(it.value());
        _socket->write("\r\n");
    }

    if (contentLength > 0) {
        _socket->write("Content-Length: ");
        _socket->write(QByteArray::number(contentLength));
        _socket->write("\r\n");

        _socket->write("Content-Type: ");
        _socket->write(contentType);
        _socket->write("\r\n");
    }
    _socket->write("Connection: close\r\n\r\n");
}

void HTTPConnection::readRequest() {
    if (!_socket->canReadLine()) {
        return;
    }
    if (!_requestUrl.isEmpty()) {
        qDebug() << "Request URL was already set";
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

void HTTPConnection::readHeaders() {
    while (_socket->canReadLine()) {
        QByteArray line = _socket->readLine();
        QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            _socket->disconnect(this, SLOT(readHeaders()));

            QByteArray clength = requestHeader("Content-Length");
            if (clength.isEmpty()) {
                _requestContent = MemoryStorage::make(0);
                _parentManager->handleHTTPRequest(this, _requestUrl);

            } else {
                bool success = false;
                auto length = clength.toInt(&success);
                if (!success) {
                    qWarning() << "Invalid header." << _address << trimmed;
                    respond("400 Bad Request", "The header was malformed.");
                    return;
                }

                // Storing big requests in memory gets expensive, especially on servers
                // with limited memory. So we store big requests in a temporary file on disk
                // and map it to faster read/write access.
                static const int MAX_CONTENT_SIZE_IN_MEMORY = 10 * 1000 * 1000;
                if (length < MAX_CONTENT_SIZE_IN_MEMORY) {
                    _requestContent = MemoryStorage::make(length);
                } else {
                    _requestContent = FileStorage::make(length);
                }

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
        _lastRequestHeader = trimmed.left(idx).toLower();
        QByteArray& value = _requestHeaders[_lastRequestHeader];
        if (!value.isEmpty()) {
            value.append(", ");
        }
        value.append(trimmed.mid(idx + 1).trimmed());
    }
}

void HTTPConnection::readContent() {
    auto size = std::min(_socket->bytesAvailable(), _requestContent->bytesLeftToWrite());

    _requestContent->write(_socket->read(size));

    if (_requestContent->bytesLeftToWrite() == 0) {
        _socket->disconnect(this, SLOT(readContent()));

        _parentManager->handleHTTPRequest(this, _requestUrl);
    }
}
