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

#include <QtCore/QThread>

#include "AssetClient.h"
#include "NetworkLogging.h"
#include "NodeList.h"
#include "ResourceCache.h"

AssetRequest::AssetRequest(const QString& hash) :
    _hash(hash)
{
}

AssetRequest::~AssetRequest() {
    auto assetClient = DependencyManager::get<AssetClient>();
    if (_assetRequestID) {
        assetClient->cancelGetAssetRequest(_assetRequestID);
    }
    if (_assetInfoRequestID) {
        assetClient->cancelGetAssetInfoRequest(_assetInfoRequestID);
    }
}

void AssetRequest::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }

    if (_state != NotStarted) {
        qCWarning(asset_client) << "AssetRequest already started.";
        return;
    }

    // in case we haven't parsed a valid hash, return an error now
    if (!isValidHash(_hash)) {
        _error = InvalidHash;
        _state = Finished;

        emit finished(this);
        return;
    }
    
    // Try to load from cache
    _data = loadFromCache(getUrl());
    if (!_data.isNull()) {
        _info.hash = _hash;
        _info.size = _data.size();
        _error = NoError;
        
        _state = Finished;
        emit finished(this);
        return;
    }
    
    _state = WaitingForInfo;
    
    auto assetClient = DependencyManager::get<AssetClient>();
    _assetInfoRequestID = assetClient->getAssetInfo(_hash,
            [this](bool responseReceived, AssetServerError serverError, AssetInfo info) {

        _assetInfoRequestID = AssetClient::INVALID_MESSAGE_ID;

        _info = info;

        if (!responseReceived) {
            _error = NetworkError;
        } else if (serverError != AssetServerError::NoError) {
            switch(serverError) {
                case AssetServerError::AssetNotFound:
                    _error = NotFound;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }

        if (_error != NoError) {
            qCWarning(asset_client) << "Got error retrieving asset info for" << _hash;
            _state = Finished;
            emit finished(this);
            
            return;
        }
        
        _state = WaitingForData;
        _data.resize(info.size);
        
        qCDebug(asset_client) << "Got size of " << _hash << " : " << info.size << " bytes";
        
        int start = 0, end = _info.size;
        
        auto assetClient = DependencyManager::get<AssetClient>();
        auto that = QPointer<AssetRequest>(this); // Used to track the request's lifetime
        auto hash = _hash;
        _assetRequestID = assetClient->getAsset(_hash, start, end,
                [this, that, hash, start, end](bool responseReceived, AssetServerError serverError, const QByteArray& data) {
            if (!that) {
                qCWarning(asset_client) << "Got reply for dead asset request " << hash << "- error code" << _error;
                // If the request is dead, return
                return;
            }
            _assetRequestID = AssetClient::INVALID_MESSAGE_ID;

            if (!responseReceived) {
                _error = NetworkError;
            } else if (serverError != AssetServerError::NoError) {
                switch (serverError) {
                    case AssetServerError::AssetNotFound:
                        _error = NotFound;
                        break;
                    case AssetServerError::InvalidByteRange:
                        _error = InvalidByteRange;
                        break;
                    default:
                        _error = UnknownError;
                        break;
                }
            } else {
                Q_ASSERT(data.size() == (end - start));
                
                // we need to check the hash of the received data to make sure it matches what we expect
                if (hashData(data).toHex() == _hash) {
                    memcpy(_data.data() + start, data.constData(), data.size());
                    _totalReceived += data.size();
                    emit progress(_totalReceived, _info.size);
                    
                    saveToCache(getUrl(), data);
                } else {
                    // hash doesn't match - we have an error
                    _error = HashVerificationFailed;
                }
                
            }
            
            if (_error != NoError) {
                qCWarning(asset_client) << "Got error retrieving asset" << _hash << "- error code" << _error;
            }
            
            _state = Finished;
            emit finished(this);
        }, [this, that](qint64 totalReceived, qint64 total) {
            if (!that) {
                // If the request is dead, return
                return;
            }
            emit progress(totalReceived, total);
        });
    });
}
