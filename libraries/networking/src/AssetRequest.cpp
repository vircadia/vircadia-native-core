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
#include "NetworkLogging.h"
#include "NodeList.h"

AssetRequest::AssetRequest(const QString& hash, const QString& extension) :
    QObject(),
    _hash(hash),
    _extension(extension)
{
    
}

void AssetRequest::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }

    if (_state != NotStarted) {
        qCWarning(networking) << "AssetRequest already started.";
        return;
    }
    
    _state = WaitingForInfo;
    
    auto assetClient = DependencyManager::get<AssetClient>();
    assetClient->getAssetInfo(_hash, _extension, [this](bool responseReceived, AssetServerError serverError, AssetInfo info) {
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
            qCDebug(networking) << "Got error retrieving asset info for" << _hash;

            _state = Finished;
            emit finished(this);
            
            return;
        }
        
        _state = WaitingForData;
        _data.resize(info.size);
        
        qCDebug(networking) << "Got size of " << _hash << " : " << info.size << " bytes";
        
        int start = 0, end = _info.size;
        
        auto assetClient = DependencyManager::get<AssetClient>();
        assetClient->getAsset(_hash, _extension, start, end, [this, start, end](bool responseReceived, AssetServerError serverError,
                                                                                const QByteArray& data) {
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
                } else {
                    // hash doesn't match - we have an error
                    _error = HashVerificationFailed;
                }
                
            }
            
            if (_error != NoError) {
                qCDebug(networking) << "Got error retrieving asset" << _hash << "- error code" << _error;
            }
            
            _state = Finished;
            emit finished(this);
        });
    });
}
