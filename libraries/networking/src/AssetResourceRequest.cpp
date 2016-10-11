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

#include "AssetClient.h"
#include "AssetUtils.h"
#include "MappingRequest.h"
#include "NetworkLogging.h"

static const int DOWNLOAD_PROGRESS_LOG_INTERVAL_SECONDS = 5;

AssetResourceRequest::AssetResourceRequest(const QUrl& url) :
    ResourceRequest(url)
{
    _lastProgressDebug = p_high_resolution_clock::now() - std::chrono::seconds(DOWNLOAD_PROGRESS_LOG_INTERVAL_SECONDS);
}

AssetResourceRequest::~AssetResourceRequest() {
    if (_assetMappingRequest) {
        _assetMappingRequest->deleteLater();
    }
    
    if (_assetRequest) {
        _assetRequest->deleteLater();
    }
}

bool AssetResourceRequest::urlIsAssetHash() const {
    static const QString ATP_HASH_REGEX_STRING { "^atp:([A-Fa-f0-9]{64})(\\.[\\w]+)?$" };

    QRegExp hashRegex { ATP_HASH_REGEX_STRING };
    return hashRegex.exactMatch(_url.toString());
}

void AssetResourceRequest::doSend() {
    // We'll either have a hash or an ATP path to a file (that maps to a hash)
    if (urlIsAssetHash()) {
        // We've detected that this is a hash - simply use AssetClient to request that asset
        auto parts = _url.path().split(".", QString::SkipEmptyParts);
        auto hash = parts.length() > 0 ? parts[0] : "";

        requestHash(hash);
    } else {
        // This is an ATP path, we'll need to figure out what the mapping is.
        // This may incur a roundtrip to the asset-server, or it may return immediately from the cache in AssetClient.

        auto path = _url.path();
        requestMappingForPath(path);
    }
}

void AssetResourceRequest::requestMappingForPath(const AssetPath& path) {
    auto assetClient = DependencyManager::get<AssetClient>();
    _assetMappingRequest = assetClient->createGetMappingRequest(path);

    // make sure we'll hear about the result of the get mapping request
    connect(_assetMappingRequest, &GetMappingRequest::finished, this, [this, path](GetMappingRequest* request){
        Q_ASSERT(_state == InProgress);
        Q_ASSERT(request == _assetMappingRequest);

        switch (request->getError()) {
            case MappingRequest::NoError:
                // we have no error, we should have a resulting hash - use that to send of a request for that asset
                qDebug() << "Got mapping for:" << path << "=>" << request->getHash();

                requestHash(request->getHash());

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

                // since we've failed we know we are finished
                _state = Finished;
                emit finished();

                break;
            }
        }

        _assetMappingRequest->deleteLater();
        _assetMappingRequest = nullptr;
    });

    _assetMappingRequest->start();
}

void AssetResourceRequest::requestHash(const AssetHash& hash) {

    // Make request to atp
    auto assetClient = DependencyManager::get<AssetClient>();
    _assetRequest = assetClient->createRequest(hash);

    connect(_assetRequest, &AssetRequest::progress, this, &AssetResourceRequest::onDownloadProgress);
    connect(_assetRequest, &AssetRequest::finished, this, [this](AssetRequest* req) {
        Q_ASSERT(_state == InProgress);
        Q_ASSERT(req == _assetRequest);
        Q_ASSERT(req->getState() == AssetRequest::Finished);
        
        switch (req->getError()) {
            case AssetRequest::Error::NoError:
                _data = req->getData();
                _result = Success;
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
        
        _state = Finished;
        emit finished();

        _assetRequest->deleteLater();
        _assetRequest = nullptr;
    });

    _assetRequest->start();
}

void AssetResourceRequest::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    Q_ASSERT(_state == InProgress);

    emit progress(bytesReceived, bytesTotal);

    auto now = p_high_resolution_clock::now();

    // if we haven't received the full asset check if it is time to output progress to log
    // we do so every X seconds to assist with ATP download tracking

    if (bytesReceived != bytesTotal
        && now - _lastProgressDebug > std::chrono::seconds(DOWNLOAD_PROGRESS_LOG_INTERVAL_SECONDS)) {

        int percentage =  roundf((float) bytesReceived / (float) bytesTotal * 100.0f);

        qCDebug(networking).nospace() << "Progress for " << _url.path() << " - "
            << bytesReceived << " bytes of " << bytesTotal << " - " << percentage << "%";

        _lastProgressDebug = now;
    }

}

