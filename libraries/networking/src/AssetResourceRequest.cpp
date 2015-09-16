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

void AssetResourceRequest::doSend() {
    // Make request to atp
    auto assetClient = DependencyManager::get<AssetClient>();
    auto parts = _url.path().split(".", QString::SkipEmptyParts);
    auto hash = parts[0];
    auto extension = parts.length() > 1 ? parts[1] : "";

    if (hash.length() != SHA256_HASH_HEX_LENGTH) {
        _result = InvalidURL;
        _state = Finished;

        emit finished();
        return;
    }

    _assetRequest = assetClient->createRequest(hash, extension);

    if (!_assetRequest) {
        _result = ServerUnavailable;
        _state = Finished;

        emit finished();
        return;
    }

    connect(_assetRequest, &AssetRequest::progress, this, &AssetResourceRequest::progress);
    connect(_assetRequest, &AssetRequest::finished, [this](AssetRequest* req) {
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
