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
#include "AssetRequest.h"
#include "AssetUtils.h"

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

    auto request = assetClient->createRequest(hash, extension);

    if (!request) {
        _result = ServerUnavailable;
        _state = Finished;

        emit finished();

        return;
    }

    connect(request, &AssetRequest::progress, this, &AssetResourceRequest::progress);
    QObject::connect(request, &AssetRequest::finished, [this](AssetRequest* req) mutable {
        Q_ASSERT(_state == InProgress);
        Q_ASSERT(req->getState() == AssetRequest::FINISHED);
        
        switch (req->getError()) {
            case NO_ERROR:
                _data = req->getData();
                _result = Success;
                break;
            case AssetNotFound:
                _result = NotFound;
                break;
            default:
                _result = Error;
                break;
        }
        
        _state = Finished;
        emit finished();
    });

    request->start();
}

void AssetResourceRequest::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit progress(bytesReceived, bytesTotal);
}
