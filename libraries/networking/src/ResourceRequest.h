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

#include "ByteRange.h"

const QString STAT_ATP_REQUEST_STARTED = "StartedATPRequest";
const QString STAT_HTTP_REQUEST_STARTED = "StartedHTTPRequest";
const QString STAT_FILE_REQUEST_STARTED = "StartedFileRequest";
const QString STAT_ATP_REQUEST_SUCCESS = "SuccessfulATPRequest";
const QString STAT_HTTP_REQUEST_SUCCESS = "SuccessfulHTTPRequest";
const QString STAT_FILE_REQUEST_SUCCESS = "SuccessfulFileRequest";
const QString STAT_ATP_REQUEST_FAILED = "FailedATPRequest";
const QString STAT_HTTP_REQUEST_FAILED = "FailedHTTPRequest";
const QString STAT_FILE_REQUEST_FAILED = "FailedFileRequest";
const QString STAT_ATP_REQUEST_CACHE = "CacheATPRequest";
const QString STAT_HTTP_REQUEST_CACHE = "CacheHTTPRequest";
const QString STAT_ATP_MAPPING_REQUEST_STARTED = "StartedATPMappingRequest";
const QString STAT_ATP_MAPPING_REQUEST_FAILED = "FailedATPMappingRequest";
const QString STAT_ATP_MAPPING_REQUEST_SUCCESS = "SuccessfulATPMappingRequest";
const QString STAT_HTTP_RESOURCE_TOTAL_BYTES = "HTTPBytesDownloaded";
const QString STAT_ATP_RESOURCE_TOTAL_BYTES = "ATPBytesDownloaded";
const QString STAT_FILE_RESOURCE_TOTAL_BYTES = "FILEBytesDownloaded";

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
        InvalidByteRange,
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
    bool getRangeRequestSuccessful() const { return _rangeRequestSuccessful; }
    bool getTotalSizeOfResource() const { return _totalSizeOfResource; }

    void setCacheEnabled(bool value) { _cacheEnabled = value; }
    void setByteRange(ByteRange byteRange) { _byteRange = byteRange; }

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
    ByteRange _byteRange;
    bool _rangeRequestSuccessful { false };
    uint64_t _totalSizeOfResource { 0 };
};

#endif
