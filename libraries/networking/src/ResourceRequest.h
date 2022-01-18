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
    static const bool IS_OBSERVABLE = true;
    static const bool IS_NOT_OBSERVABLE = false;

    ResourceRequest(
        const QUrl& url,
        const bool isObservable = IS_OBSERVABLE,
        const qint64 callerId = -1,
        const QString& extra = ""
    ) : _url(url),
        _isObservable(isObservable),
        _callerId(callerId),
        _extra(extra)
    { }

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
        NotFound,
        RedirectFail
    };
    Q_ENUM(Result)

    QByteArray getData() { return _data; }
    State getState() const { return _state; }
    Result getResult() const { return _result; }
    QString getResultString() const;
    QUrl getUrl() const { return _url; }
    QUrl getRelativePathUrl() const { return _relativePathURL; }
    bool loadedFromCache() const { return _loadedFromCache; }
    bool getRangeRequestSuccessful() const { return _rangeRequestSuccessful; }
    bool getTotalSizeOfResource() const { return _totalSizeOfResource; }
    QString getWebMediaType() const { return _webMediaType; }
    void setFailOnRedirect(bool failOnRedirect) { _failOnRedirect = failOnRedirect; }

    void setCacheEnabled(bool value) { _cacheEnabled = value; }
    void setByteRange(ByteRange byteRange) { _byteRange = byteRange; }

    static QString toHttpDateString(uint64_t msecsSinceEpoch);
public slots:
    void send();

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void finished();

protected:
    virtual void doSend() = 0;
    void recordBytesDownloadedInStats(const QString& statName, int64_t bytesReceived);

    QUrl _url;
    QUrl _relativePathURL;
    State _state { NotStarted };
    Result _result;
    QByteArray _data;
    bool _failOnRedirect { false };
    bool _cacheEnabled { true };
    bool _loadedFromCache { false };
    ByteRange _byteRange;
    bool _rangeRequestSuccessful { false };
    uint64_t _totalSizeOfResource { 0 };
    QString _webMediaType;
    int64_t _lastRecordedBytesDownloaded { 0 };
    bool _isObservable;
    qint64 _callerId;
    QString _extra;
};

#endif
