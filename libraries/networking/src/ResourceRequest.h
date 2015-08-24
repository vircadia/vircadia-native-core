//
//  ResourceRequest.h
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

class ResourceRequest : public QObject {
    Q_OBJECT
public:
    ResourceRequest(QObject* parent, const QUrl& url);

    enum State {
        UNSENT = 0,
        IN_PROGRESS,
        FINISHED
    };

    enum Result {
        SUCCESS,
        ERROR,
        TIMEOUT,
        NOT_FOUND
    };

    void send();
    QByteArray moveData() { return _data; }
    State getState() const { return _state; }
    Result getResult() const { return _result; }
    QUrl getUrl() const { return _url; }
    bool loadedFromCache() const { return _loadedFromCache; }

    void setCacheEnabled(bool value) { _cacheEnabled = value; }

signals:
    void progress(uint64_t bytesReceived, uint64_t bytesTotal);
    void finished();

protected:
    virtual void doSend() = 0;

    QUrl _url;
    State _state { UNSENT };
    Result _result;
    QByteArray _data;
    bool _cacheEnabled { true };
    bool _loadedFromCache { false };
};

#endif
