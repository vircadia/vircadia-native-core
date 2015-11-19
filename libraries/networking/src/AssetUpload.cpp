//
//  AssetUpload.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2015-08-26.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetUpload.h"

#include <QtCore/QFileInfo>
#include <QtCore/QThread>

#include "AssetClient.h"
#include "NetworkLogging.h"

const QString AssetUpload::PERMISSION_DENIED_ERROR = "You do not have permission to upload content to this asset-server.";

AssetUpload::AssetUpload(const QByteArray& data, const QString& extension) :
    _data(data),
    _extension(extension)
{
    
}

AssetUpload::AssetUpload(const QString& filename) :
    _filename(filename)
{
    
}

QString AssetUpload::getErrorString() const {
    // figure out the right error message for error
    switch (_error) {
        case AssetUpload::PermissionDenied:
            return PERMISSION_DENIED_ERROR;
        case AssetUpload::TooLarge:
            return "The uploaded content was too large and could not be stored in the asset-server.";
        case AssetUpload::FileOpenError:
            return "The file could not be opened. Please check your permissions and try again.";
        case AssetUpload::NetworkError:
            return "The file could not be opened. Please check your network connectivity.";
        default:
            // not handled, do not show a message box
            return QString();
    }
}

void AssetUpload::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start");
        return;
    }
    
    if (_data.isEmpty() && !_filename.isEmpty()) {
        // try to open the file at the given filename
        QFile file { _filename };
        
        if (file.open(QIODevice::ReadOnly)) {
            
            // file opened, read the data and grab the extension
            _extension = QFileInfo(_filename).suffix();
            
            _data = file.readAll();
        } else {
            // we couldn't open the file - set the error result
            _error = FileOpenError;
            
            // emit that we are done
            emit finished(this, QString());
        }
    }
    
    // ask the AssetClient to upload the asset and emit the proper signals from the passed callback
    auto assetClient = DependencyManager::get<AssetClient>();
   
    if (!_filename.isEmpty()) {
        qCDebug(asset_client) << "Attempting to upload" << _filename << "to asset-server.";
    }
    
    assetClient->uploadAsset(_data, _extension, [this](bool responseReceived, AssetServerError error, const QString& hash){
        if (!responseReceived) {
            _error = NetworkError;
        } else {
            switch (error) {
                case AssetServerError::NoError:
                    _error = NoError;
                    break;
                case AssetServerError::AssetTooLarge:
                    _error = TooLarge;
                    break;
                case AssetServerError::PermissionDenied:
                    _error = PermissionDenied;
                    break;
                default:
                    _error = FileOpenError;
                    break;
            }
        }
        
        if (_error == NoError && hash == hashData(_data).toHex()) {
            saveToCache(getATPUrl(hash, _extension), _data);
        }
        
        emit finished(this, hash);
    });
}
