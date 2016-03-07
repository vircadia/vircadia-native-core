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

#include "AssetClient.h"
#include "AssetUtils.h"

AssetResourceRequest::~AssetResourceRequest() {
    if (_assetRequest) {
        _assetRequest->deleteLater();
    }
}

bool AssetResourceRequest::urlIsAssetPath() const {
    static const QString ATP_HASH_REGEX_STRING = "^atp:([A-Fa-f0-9]{64})(\\.[\\w]+)?$";

    QRegExp hashRegex { ATP_HASH_REGEX_STRING };
    return !hashRegex.exactMatch(_url.toString());
}

void AssetResourceRequest::doSend() {
    auto parts = _url.path().split(".", QString::SkipEmptyParts);
    auto hash = parts.length() > 0 ? parts[0] : "";
    auto extension = parts.length() > 1 ? parts[1] : "";

    // We'll either have a hash or an ATP path to a file (that maps to a hash)

    if (urlIsAssetPath()) {
        // This is an ATP path, we'll need to figure out what the mapping is.
        // This may incur a roundtrip to the asset-server, or it may return immediately from the cache in AssetClient.

        qDebug() << "Detected an asset path! URL is" << _url;
    } else {

        qDebug() << "ATP URL was not an asset path - url is" << _url.toString();

        // We've detected that this is a hash - simply use AssetClient to request that asset
        auto parts = _url.path().split(".", QString::SkipEmptyParts);
        auto hash = parts.length() > 0 ? parts[0] : "";

        requestHash(hash);
    }
}

void AssetResourceRequest::requestHash(const AssetHash& hash) {

    // in case we haven't parsed a valid hash, return an error now
    if (hash.length() != SHA256_HASH_HEX_LENGTH) {
        _result = InvalidURL;
        _state = Finished;

        emit finished();
        return;
    }

    // Make request to atp
    auto assetClient = DependencyManager::get<AssetClient>();
    _assetRequest = assetClient->createRequest(hash);

    if (!_assetRequest) {
        _result = ServerUnavailable;
        _state = Finished;

        emit finished();
        return;
    }

    connect(_assetRequest, &AssetRequest::progress, this, &AssetResourceRequest::progress);
    connect(_assetRequest, &AssetRequest::finished, this, [this](AssetRequest* req) {
        Q_ASSERT(_state == InProgress);
        Q_ASSERT(req == _assetRequest);
        Q_ASSERT(req->getState() == AssetRequest::Finished);
        
        switch (req->getError()) {
            case AssetRequest::Error::NoError:
                _data = req->getData();
                _result = Success;
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
    emit progress(bytesReceived, bytesTotal);
}
