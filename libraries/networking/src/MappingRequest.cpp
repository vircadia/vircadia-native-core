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

#include <QtCore/QSharedPointer>
#include <QtCore/QThread>

#include <DependencyManager.h>

MappingRequest::~MappingRequest() {
    auto assetClient = DependencyManager::get<AssetClient>();
    if (_mappingRequestID) {
        assetClient->cancelMappingRequest(_mappingRequestID);
    }
}

void MappingRequest::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }
    doStart();
};

QString MappingRequest::getErrorString() const {
    switch (_error) {
        case MappingRequest::NoError:
            return QString();
        case MappingRequest::NotFound:
            return "Asset not found";
        case MappingRequest::NetworkError:
            return "Unable to communicate with Asset Server";
        case MappingRequest::PermissionDenied:
            return "Permission denied";
        case MappingRequest::InvalidPath:
            return "Path is invalid";
        case MappingRequest::InvalidHash:
            return "Hash is invalid";
        case MappingRequest::UnknownError:
            return "Asset Server internal error";
        default:
            return QString("Unknown error with code %1").arg(_error);
    }
}

GetMappingRequest::GetMappingRequest(const AssetUtils::AssetPath& path) : _path(path.trimmed()) {
};

void GetMappingRequest::doStart() {

    // short circuit the request if the path is invalid
    if (!AssetUtils::isValidFilePath(_path)) {
        _error = MappingRequest::InvalidPath;
        emit finished(this);
        return;
    }

    auto assetClient = DependencyManager::get<AssetClient>();

    _mappingRequestID = assetClient->getAssetMapping(_path,
            [this, assetClient](bool responseReceived, AssetUtils::AssetServerError error, QSharedPointer<ReceivedMessage> message) {

        _mappingRequestID = INVALID_MESSAGE_ID;
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetUtils::AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetUtils::AssetServerError::AssetNotFound:
                    _error = NotFound;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }

        if (!_error) {
            _hash = message->read(AssetUtils::SHA256_HASH_LENGTH).toHex();

            // check the boolean to see if this request got re-directed
            quint8 wasRedirected;
            message->readPrimitive(&wasRedirected);
            _wasRedirected = wasRedirected;

            // if it did grab that re-directed path
            if (_wasRedirected) {
                _redirectedPath = message->readString();
            }

        }
        emit finished(this);
    });
};

void GetAllMappingsRequest::doStart() {
    auto assetClient = DependencyManager::get<AssetClient>();
    _mappingRequestID = assetClient->getAllAssetMappings(
            [this, assetClient](bool responseReceived, AssetUtils::AssetServerError error, QSharedPointer<ReceivedMessage> message) {

        _mappingRequestID = INVALID_MESSAGE_ID;

        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetUtils::AssetServerError::NoError:
                    _error = NoError;
                    break;
                default:
                    _error = UnknownError;
                    break;
            }
        }


        if (!_error) {
            uint32_t numberOfMappings;
            message->readPrimitive(&numberOfMappings);
            for (uint32_t i = 0; i < numberOfMappings; ++i) {
                auto path = message->readString();
                auto hash = message->read(AssetUtils::SHA256_HASH_LENGTH).toHex();
                AssetUtils::BakingStatus status;
                QString lastBakeErrors;
                message->readPrimitive(&status);
                if (status == AssetUtils::BakingStatus::Error) {
                    lastBakeErrors = message->readString();
                }
                _mappings[path] = { hash, status, lastBakeErrors };
            }
        }
        emit finished(this);
    });
};

SetMappingRequest::SetMappingRequest(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash) :
    _path(path.trimmed()),
    _hash(hash)
{

};

void SetMappingRequest::doStart() {

    // short circuit the request if the hash or path are invalid
    auto validPath = AssetUtils::isValidFilePath(_path);
    auto validHash = AssetUtils::isValidHash(_hash);
    if (!validPath || !validHash) {
        _error = !validPath ? MappingRequest::InvalidPath : MappingRequest::InvalidHash;
        emit finished(this);
        return;
    }

    auto assetClient = DependencyManager::get<AssetClient>();

    _mappingRequestID = assetClient->setAssetMapping(_path, _hash,
            [this, assetClient](bool responseReceived, AssetUtils::AssetServerError error, QSharedPointer<ReceivedMessage> message) {

        _mappingRequestID = INVALID_MESSAGE_ID;
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetUtils::AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetUtils::AssetServerError::PermissionDenied:
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

DeleteMappingsRequest::DeleteMappingsRequest(const AssetUtils::AssetPathList& paths) : _paths(paths) {
    for (auto& path : _paths) {
        path = path.trimmed();
    }
};

void DeleteMappingsRequest::doStart() {

    // short circuit the request if any of the paths are invalid
    for (auto& path : _paths) {
        if (!AssetUtils::isValidPath(path)) {
            _error = MappingRequest::InvalidPath;
            emit finished(this);
            return;
        }
    }

    auto assetClient = DependencyManager::get<AssetClient>();

    _mappingRequestID = assetClient->deleteAssetMappings(_paths,
            [this, assetClient](bool responseReceived, AssetUtils::AssetServerError error, QSharedPointer<ReceivedMessage> message) {

        _mappingRequestID = INVALID_MESSAGE_ID;
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetUtils::AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetUtils::AssetServerError::PermissionDenied:
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

RenameMappingRequest::RenameMappingRequest(const AssetUtils::AssetPath& oldPath, const AssetUtils::AssetPath& newPath) :
    _oldPath(oldPath.trimmed()),
    _newPath(newPath.trimmed())
{

}

void RenameMappingRequest::doStart() {

    // short circuit the request if either of the paths are invalid
    if (!AssetUtils::isValidFilePath(_oldPath) || !AssetUtils::isValidFilePath(_newPath)) {
        _error = InvalidPath;
        emit finished(this);
        return;
    }

    auto assetClient = DependencyManager::get<AssetClient>();

    _mappingRequestID = assetClient->renameAssetMapping(_oldPath, _newPath,
            [this, assetClient](bool responseReceived, AssetUtils::AssetServerError error, QSharedPointer<ReceivedMessage> message) {

        _mappingRequestID = INVALID_MESSAGE_ID;
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetUtils::AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetUtils::AssetServerError::PermissionDenied:
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

SetBakingEnabledRequest::SetBakingEnabledRequest(const AssetUtils::AssetPathList& paths, bool enabled) : _paths(paths), _enabled(enabled) {
    for (auto& path : _paths) {
        path = path.trimmed();
    }
};

void SetBakingEnabledRequest::doStart() {

    // short circuit the request if any of the paths are invalid
    for (auto& path : _paths) {
        if (!AssetUtils::isValidPath(path)) {
            _error = MappingRequest::InvalidPath;
            emit finished(this);
            return;
        }
    }

    auto assetClient = DependencyManager::get<AssetClient>();

    _mappingRequestID = assetClient->setBakingEnabled(_paths, _enabled,
        [this, assetClient](bool responseReceived, AssetUtils::AssetServerError error, QSharedPointer<ReceivedMessage> message) {

        _mappingRequestID = INVALID_MESSAGE_ID;
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
            case AssetUtils::AssetServerError::NoError:
                _error = NoError;
                break;
            case AssetUtils::AssetServerError::PermissionDenied:
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
