//
//  AssetResourceRequest.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetResourceRequest.h"

#include <QtCore/QLoggingCategory>

#include <Trace.h>
#include <Profile.h>
#include <StatTracker.h>

#include "AssetClient.h"
#include "AssetUtils.h"
#include "MappingRequest.h"
#include "NetworkLogging.h"

static const int DOWNLOAD_PROGRESS_LOG_INTERVAL_SECONDS = 5;

AssetResourceRequest::AssetResourceRequest(
    const QUrl& url,
    const bool isObservable,
    const qint64 callerId,
    const QString& extra) :
    ResourceRequest(url, isObservable, callerId, extra)
{
    _lastProgressDebug = p_high_resolution_clock::now() - std::chrono::seconds(DOWNLOAD_PROGRESS_LOG_INTERVAL_SECONDS);
}

AssetResourceRequest::~AssetResourceRequest() {
    if (_assetRequest || _assetMappingRequest) {
        if (_assetMappingRequest) {
            _assetMappingRequest->deleteLater();
        }
        
        if (_assetRequest) {
            _assetRequest->deleteLater();
        }
    }
}

bool AssetResourceRequest::urlIsAssetHash(const QUrl& url) {
    static const QString ATP_HASH_REGEX_STRING { "^atp:([A-Fa-f0-9]{64})(\\.[\\w]+)?$" };

    QRegExp hashRegex { ATP_HASH_REGEX_STRING };
    return hashRegex.exactMatch(url.toString());
}

void AssetResourceRequest::doSend() {
    DependencyManager::get<StatTracker>()->incrementStat(STAT_ATP_REQUEST_STARTED);

    // We'll either have a hash or an ATP path to a file (that maps to a hash)
    if (urlIsAssetHash(_url)) {
        // We've detected that this is a hash - simply use AssetClient to request that asset
        auto parts = _url.path().split(".", Qt::SkipEmptyParts);
        auto hash = parts.length() > 0 ? parts[0] : "";

        requestHash(hash);
    } else {
        // This is an ATP path, we'll need to figure out what the mapping is.
        // This may incur a roundtrip to the asset-server, or it may return immediately from the cache in AssetClient.

        auto path = _url.path() + (_url.hasQuery() ? "?" + _url.query() : "");
        requestMappingForPath(path);
    }
}

void AssetResourceRequest::requestMappingForPath(const AssetUtils::AssetPath& path) {
    auto statTracker = DependencyManager::get<StatTracker>();
    statTracker->incrementStat(STAT_ATP_MAPPING_REQUEST_STARTED);

    auto assetClient = DependencyManager::get<AssetClient>();
    _assetMappingRequest = assetClient->createGetMappingRequest(path);

    // make sure we'll hear about the result of the get mapping request
    connect(_assetMappingRequest, &GetMappingRequest::finished, this, [this, path](GetMappingRequest* request){
        auto statTracker = DependencyManager::get<StatTracker>();
        
        Q_ASSERT(_state == InProgress);
        Q_ASSERT(request == _assetMappingRequest);

        bool failed = false;

        switch (request->getError()) {
            case MappingRequest::NoError:
                // we have no error, we should have a resulting hash - use that to send of a request for that asset
                qCDebug(networking) << "Got mapping for:" << path << "=>" << request->getHash();

                statTracker->incrementStat(STAT_ATP_MAPPING_REQUEST_SUCCESS);

                // if we got a redirected path we need to store that with the resource request as relative path URL
                if (request->wasRedirected()) {
                    _relativePathURL = ATP_SCHEME + request->getRedirectedPath();
                }

                if (request->wasRedirected() && _failOnRedirect) {
                    _result = RedirectFail;
                    failed = true;
                } else {
                    requestHash(request->getHash());
                }

                break;
            default: {
                switch (request->getError()) {
                    case MappingRequest::NotFound:
                        // no result for the mapping request, set error to not found
                        _result = NotFound;
                        break;
                    case MappingRequest::NetworkError:
                        // didn't hear back from the server, mark it unavailable
                        _result = ServerUnavailable;
                        break;
                    default:
                        _result = Error;
                        break;
                }

                failed = true;

                break;
            }
        }

        if (failed) {
            _state = Finished;
            emit finished();

            statTracker->incrementStat(STAT_ATP_MAPPING_REQUEST_FAILED);
            statTracker->incrementStat(STAT_ATP_REQUEST_FAILED);
        }

        _assetMappingRequest->deleteLater();
        _assetMappingRequest = nullptr;
    });

    _assetMappingRequest->start();
}

void AssetResourceRequest::requestHash(const AssetUtils::AssetHash& hash) {
    // Make request to atp
    auto assetClient = DependencyManager::get<AssetClient>();
    _assetRequest = assetClient->createRequest(hash, _byteRange);

    connect(_assetRequest, &AssetRequest::progress, this, &AssetResourceRequest::onDownloadProgress);
    connect(_assetRequest, &AssetRequest::finished, this, [this](AssetRequest* req) {
        Q_ASSERT(_state == InProgress);
        Q_ASSERT(req == _assetRequest);
        Q_ASSERT(req->getState() == AssetRequest::Finished);

        switch (req->getError()) {
            case AssetRequest::Error::NoError:
                _data = req->getData();
                _result = Success;
                recordBytesDownloadedInStats(STAT_ATP_RESOURCE_TOTAL_BYTES, _data.size());
                break;
            case AssetRequest::InvalidHash:
                _result = InvalidURL;
                break;
            case AssetRequest::Error::NotFound:
                _result = NotFound;
                break;
            case AssetRequest::Error::NetworkError:
                _result = ServerUnavailable;
                break;
            default:
                _result = Error;
                break;
        }

        auto statTracker = DependencyManager::get<StatTracker>();

        if (_assetRequest->loadedFromCache()) {
            _loadedFromCache = true;
        }

        _state = Finished;
        emit finished();

        if (_result == Success) {
            statTracker->incrementStat(STAT_ATP_REQUEST_SUCCESS);

            if (loadedFromCache()) {
                statTracker->incrementStat(STAT_ATP_REQUEST_CACHE);
            }
        } else {
            statTracker->incrementStat(STAT_ATP_REQUEST_FAILED);
        }

        _assetRequest->deleteLater();
        _assetRequest = nullptr;
    });

    _assetRequest->start();
}

void AssetResourceRequest::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    Q_ASSERT(_state == InProgress);

    emit progress(bytesReceived, bytesTotal);

    auto now = p_high_resolution_clock::now();

    recordBytesDownloadedInStats(STAT_ATP_RESOURCE_TOTAL_BYTES, bytesReceived);

    // if we haven't received the full asset check if it is time to output progress to log
    // we do so every X seconds to assist with ATP download tracking

    if (bytesReceived != bytesTotal
        && now - _lastProgressDebug > std::chrono::seconds(DOWNLOAD_PROGRESS_LOG_INTERVAL_SECONDS)) {

        _lastProgressDebug = now;
    }
}

