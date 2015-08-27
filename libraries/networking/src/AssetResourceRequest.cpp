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

void AssetResourceRequest::doSend() {
    // Make request to atp
    auto assetClient = DependencyManager::get<AssetClient>();
    auto parts = _url.path().split(".", QString::SkipEmptyParts);
    auto hash = parts[0];
    auto extension = parts.length() > 1 ? parts[1] : "";

    auto request = assetClient->create(hash, extension);

    if (!request) {
        return;
    }

    connect(request, &AssetRequest::progress, this, &AssetResourceRequest::progress);
    QObject::connect(request, &AssetRequest::finished, [this](AssetRequest* req) mutable {
        if (_state != IN_PROGRESS) return;
        _state = FINISHED;
        if (true) {
            _data = req->getData();
            _result = ResourceRequest::SUCCESS;
            emit finished();
        } else {
            _result = ResourceRequest::ERROR;
            emit finished();
        }
    });

    request->start();
}

void AssetResourceRequest::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    qDebug() << "Got asset data: " << bytesReceived << " / " << bytesTotal;
    emit progress(bytesReceived, bytesTotal);
}
