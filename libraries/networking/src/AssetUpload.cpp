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

AssetUpload::AssetUpload(QObject* object, const QString& filename) :
    _filename(filename)
{
    
}

void AssetUpload::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }
    
    // try to open the file at the given filename
    QFile file { _filename };
    
    if (file.open(QIODevice::ReadOnly)) {
        
        // file opened, read the data and grab the extension
        auto extension = QFileInfo(_filename).suffix();
        
        auto data = file.readAll();
        
        // ask the AssetClient to upload the asset and emit the proper signals from the passed callback
        auto assetClient = DependencyManager::get<AssetClient>();
        
        assetClient->uploadAsset(data, extension, [this](bool result, QString hash){
            if (result) {
                // successful upload - emit finished with a point to ourselves and the resulting hash
                emit finished(this, hash);
            } else {
                
            }
        });
    }
}
