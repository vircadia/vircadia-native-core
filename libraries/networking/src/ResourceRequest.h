//
//  ResourceRequest.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ResourceRequest_h
#define hifi_ResourceRequest_h

#include <QObject>
#include <QUrl>

#include <cstdint>

class ResourceRequest : public QObject {
    Q_OBJECT
public:
    ResourceRequest(const QUrl& url);
    virtual ~ResourceRequest() = default;

    enum State {
        NotStarted = 0,
        InProgress,
        Finished
    };

    enum Result {
        Success,
        Error,
        Timeout,
        ServerUnavailable,
        AccessDenied,
        InvalidURL,
        NotFound
    };
    Q_ENUM(Result)

    QByteArray getData() { return _data; }
    State getState() const { return _state; }
    Result getResult() const { return _result; }
    QString getResultString() const;
    QUrl getUrl() const { return _url; }
    bool loadedFromCache() const { return _loadedFromCache; }

    void setCacheEnabled(bool value) { _cacheEnabled = value; }

public slots:
    void send();

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void finished();

protected:
    virtual void doSend() = 0;

    QUrl _url;
    State _state { NotStarted };
    Result _result;
    QByteArray _data;
    bool _cacheEnabled { true };
    bool _loadedFromCache { false };
};

#endif
