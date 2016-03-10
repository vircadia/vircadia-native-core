//
//  MappingRequest.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MappingRequest.h"

#include <QtCore/QThread>

#include <DependencyManager.h>

#include "AssetClient.h"

void MappingRequest::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }
    doStart();
};

GetMappingRequest::GetMappingRequest(const AssetPath& path) : _path(path) {
};

void GetMappingRequest::doStart() {

    auto assetClient = DependencyManager::get<AssetClient>();

    assetClient->getAssetMapping(_path, [this, assetClient](bool responseReceived, AssetServerError error, QSharedPointer<ReceivedMessage> message) {
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetServerError::AssetNotFound:
                    _error = NotFound;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }

        if (!_error) {
            _hash = message->read(SHA256_HASH_LENGTH).toHex();
        }
        emit finished(this);
    });
};

GetAllMappingsRequest::GetAllMappingsRequest() {
};

void GetAllMappingsRequest::doStart() {
    auto assetClient = DependencyManager::get<AssetClient>();
    assetClient->getAllAssetMappings([this, assetClient](bool responseReceived, AssetServerError error, QSharedPointer<ReceivedMessage> message) {
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetServerError::NoError:
                    _error = NoError;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }


        if (!_error) {
            int numberOfMappings;
            message->readPrimitive(&numberOfMappings);
            for (auto i = 0; i < numberOfMappings; ++i) {
                auto path = message->readString();
                auto hash = message->read(SHA256_HASH_LENGTH).toHex();
                _mappings[path] = hash;
            }
        }
        emit finished(this);
    });
};

SetMappingRequest::SetMappingRequest(const AssetPath& path, const AssetHash& hash) : _path(path), _hash(hash) {
};

void SetMappingRequest::doStart() {
    auto assetClient = DependencyManager::get<AssetClient>();
    assetClient->setAssetMapping(_path, _hash, [this, assetClient](bool responseReceived, AssetServerError error, QSharedPointer<ReceivedMessage> message) {
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetServerError::PermissionDenied:
                    _error = PermissionDenied;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }

        emit finished(this);
    });
};

DeleteMappingsRequest::DeleteMappingsRequest(const AssetPathList& paths) : _paths(paths) {
};

void DeleteMappingsRequest::doStart() {
    auto assetClient = DependencyManager::get<AssetClient>();
    assetClient->deleteAssetMappings(_paths, [this, assetClient](bool responseReceived, AssetServerError error, QSharedPointer<ReceivedMessage> message) {
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetServerError::PermissionDenied:
                    _error = PermissionDenied;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }

        emit finished(this);
    });
};

RenameMappingRequest::RenameMappingRequest(const AssetPath& oldPath, const AssetPath& newPath) :
_oldPath(oldPath),
_newPath(newPath)
{

}

void RenameMappingRequest::doStart() {
    auto assetClient = DependencyManager::get<AssetClient>();
    assetClient->renameAssetMapping(_oldPath, _newPath, [this, assetClient](bool responseReceived,
                                                                            AssetServerError error,
                                                                            QSharedPointer<ReceivedMessage> message) {
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetServerError::PermissionDenied:
                    _error = PermissionDenied;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }

        emit finished(this);
    });
}
