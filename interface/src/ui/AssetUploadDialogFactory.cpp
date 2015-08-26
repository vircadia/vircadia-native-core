//
//  AssetUploadDialogFactory.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on 2015-08-26.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetUploadDialogFactory.h"

#include <AssetClient.h>

#include <QtCore/QDebug>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

AssetUploadDialogFactory& AssetUploadDialogFactory::getInstance() {
    static AssetUploadDialogFactory staticInstance;
    return staticInstance;
}

AssetUploadDialogFactory::AssetUploadDialogFactory() {
    
}

void AssetUploadDialogFactory::showDialog() {
    auto filename = QFileDialog::getOpenFileUrl(nullptr, "Select a file to upload");
    
    if (!filename.isEmpty()) {
        qDebug() << "Selected filename for upload to asset-server: " << filename;
        
        QFile file { filename.path() };
        
        if (file.open(QIODevice::ReadOnly)) {
            
            QFileInfo fileInfo { filename.path() };
            auto extension = fileInfo.suffix();
            
            auto data = file.readAll();
            
            auto assetClient = DependencyManager::get<AssetClient>();
            
            assetClient->uploadAsset(data, extension, [this, extension](bool result, QString hash) mutable {
                if (result) {
                    QMessageBox::information(_dialogParent, "Upload Successful", "URL: apt:/" + hash + "." + extension);
                } else {
                    QMessageBox::warning(_dialogParent, "Upload Failed", "There was an error uploading the file.");
                }
            });
        }
    }
}
