//
//  ATPAssetMigrator.cpp
//  interface/src/assets
//
//  Created by Stephen Birarda on 2015-10-12.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ATPAssetMigrator.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryFile>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <GZip.h>

#include <AssetClient.h>
#include <AssetUpload.h>
#include <ResourceManager.h>

#include "../ui/AssetUploadDialogFactory.h"

ATPAssetMigrator& ATPAssetMigrator::getInstance() {
    static ATPAssetMigrator instance;
    return instance;
}

static const QString MODEL_URL_KEY = "modelURL";

void ATPAssetMigrator::loadEntityServerFile() {
    auto filename = QFileDialog::getOpenFileName(_dialogParent, "Select an entity-server content file to migrate",
                                                 QString(), QString("Entity-Server Content (*.gz)"));
    
    if (!filename.isEmpty()) {
        qDebug() << "Selected filename for ATP asset migration: " << filename;
        
        // try to open the file at the given filename
        QFile modelsFile { filename };
        
        if (modelsFile.open(QIODevice::ReadOnly)) {
            QByteArray compressedJsonData = modelsFile.readAll();
            QByteArray jsonData;
            
            if (!gunzip(compressedJsonData, jsonData)) {
                QMessageBox::warning(_dialogParent, "Error", "The file at" + filename + "was not in gzip format.");
            }
            
            QJsonDocument modelsJSON = QJsonDocument::fromJson(jsonData);
            _entitiesArray = modelsJSON.object()["Entities"].toArray();
            
            for (auto jsonValue : _entitiesArray) {
                QJsonObject entityObject = jsonValue.toObject();
                QString modelURLString = entityObject.value(MODEL_URL_KEY).toString();
                
                if (!modelURLString.isEmpty()) {
                    QUrl modelURL = QUrl(modelURLString);
                    
                    if (modelURL.scheme() == URL_SCHEME_HTTP || modelURL.scheme() == URL_SCHEME_HTTPS
                        || modelURL.scheme() == URL_SCHEME_FILE || modelURL.scheme() == URL_SCHEME_FTP) {
                        
                        QMessageBox messageBox;
                        messageBox.setWindowTitle("Asset Migration");
                        messageBox.setText("Would you like to migrate the following file to the asset server?");
                        messageBox.setInformativeText(modelURL.toDisplayString());
                        messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                        messageBox.setDefaultButton(QMessageBox::Ok);
                        
                        if (messageBox.exec() == QMessageBox::Ok) {
                            // user wants to migrate this asset
                            
                            if (_pendingReplacements.contains(modelURL)) {
                                // we already have a request out for this asset, just store the QJsonValueRef
                                // so we can do the hash replacement when the request comes back
                                _pendingReplacements.insert(modelURL, jsonValue);
                            } else if (_uploadedAssets.contains(modelURL)) {
                                // we already have a hash for this asset
                                // so just do the replacement immediately
                                entityObject[MODEL_URL_KEY] = _uploadedAssets.value(modelURL).toString();
                                jsonValue = entityObject;
                            } else {
                                auto request = ResourceManager::createResourceRequest(this, modelURL);
                                
                                qDebug() << "Requesting" << modelURL << "for ATP asset migration";
                                
                                connect(request, &ResourceRequest::finished, this, [=]() {
                                    if (request->getResult() == ResourceRequest::Success) {
                                        migrateResource(request);
                                    } else {
                                        QMessageBox::warning(_dialogParent, "Error",
                                                             QString("Could not retreive asset at %1").arg(modelURL.toString()));
                                    }
                                    request->deleteLater();
                                });
                                
                                // add this combination of QUrl and QJsonValueRef to our multi hash so we can change the URL
                                // to an ATP one once ready
                                _pendingReplacements.insert(modelURL, jsonValue);
                                
                                request->send();
                            }

                        }
                    }
                }
            }
            
            _doneReading = true;
            
        } else {
            QMessageBox::warning(_dialogParent, "Error",
                                 "There was a problem loading that entity-server file for ATP asset migration. Please try again");
        }
    }
}

void ATPAssetMigrator::migrateResource(ResourceRequest* request) {
    // use an asset client to upload the asset
    auto assetClient = DependencyManager::get<AssetClient>();
    
    QFileInfo assetInfo { request->getUrl().fileName() };
    
    auto upload = assetClient->createUpload(request->getData(), assetInfo.completeSuffix());
    
    if (upload) {
        // add this URL to our hash of AssetUpload to original URL
        _originalURLs.insert(upload, request->getUrl());
        
        qDebug() << "Starting upload of asset from" << request->getUrl();
        
        // connect to the finished signal so we know when the AssetUpload is done
        QObject::connect(upload, &AssetUpload::finished, this, &ATPAssetMigrator::assetUploadFinished);
        
        // start the upload now
        upload->start();
    } else {
        // show a QMessageBox to say that there is no local asset server
        QString messageBoxText = QString("Could not upload \n\n%1\n\nbecause you are currently not connected" \
                                         " to a local asset-server.").arg(assetInfo.fileName());
        
        QMessageBox::information(_dialogParent, "Failed to Upload", messageBoxText);
    }
}

void ATPAssetMigrator::assetUploadFinished(AssetUpload *upload, const QString& hash) {
    if (upload->getError() == AssetUpload::NoError) {

        const auto& modelURL = _originalURLs[upload];
        
        // successfully uploaded asset - make any required replacements found in the pending replacements
        auto values = _pendingReplacements.values(modelURL);
        
        QString atpURL = QString("%1:%2.%3").arg(ATP_SCHEME).arg(hash).arg(upload->getExtension());
        
        for (auto value : values) {
            // replace the modelURL in this QJsonValueRef with the hash
            QJsonObject valueObject = value.toObject();
            valueObject[MODEL_URL_KEY] = atpURL;
            value = valueObject;
        }
        
        // pull the replaced models from _pendingReplacements
        _pendingReplacements.remove(modelURL);
        
        // are we out of pending replacements? if so it is time to save the entity-server file
        if (_doneReading && _pendingReplacements.empty()) {
            // show a dialog to ask the user where they want to save the file
        }
    } else {
        AssetUploadDialogFactory::showErrorDialog(upload, _dialogParent);
    }
}
