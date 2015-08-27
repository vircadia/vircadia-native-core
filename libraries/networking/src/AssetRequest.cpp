//
//  AssetRequest.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetRequest.h"

#include <algorithm>

#include <QThread>

#include "AssetClient.h"
#include "NodeList.h"


AssetRequest::AssetRequest(QObject* parent, QString hash, QString extension) :
    QObject(parent),
    _hash(hash),
    _extension(extension)
{
}

void AssetRequest::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }

    if (_state == NOT_STARTED) {
        _state = WAITING_FOR_INFO;

        auto assetClient = DependencyManager::get<AssetClient>();
        assetClient->getAssetInfo(_hash, _extension, [this](bool success, AssetInfo info) {
            _info = info;
            _data.resize(info.size);
            const DataOffset CHUNK_SIZE = 1024000000;

            qDebug() << "Got size of " << _hash << " : " << info.size << " bytes";

            // Round up
            int numChunks = (info.size + CHUNK_SIZE - 1) / CHUNK_SIZE;
            auto assetClient = DependencyManager::get<AssetClient>();
            for (int i = 0; i < numChunks; ++i) {
                ++_numPendingRequests;
                auto start = i * CHUNK_SIZE;
                auto end = std::min((i + 1) * CHUNK_SIZE, info.size);
                assetClient->getAsset(_hash, _extension, start, end, [this, start, end](bool success, QByteArray data) {
                    Q_ASSERT(data.size() == (end - start));

                    if (success) {
                        _result = Success;
                        memcpy((_data.data() + start), data.constData(), end - start);
                        _totalReceived += data.size();
                        emit progress(_totalReceived, _info.size);
                    } else {
                        _result = Error;
                        qDebug() << "Got error retrieving asset";
                    }

                    --_numPendingRequests;
                    if (_numPendingRequests == 0) {
                        _state = FINISHED;
                        emit finished(this);
                    }
                });
            }
        });
    }
}
