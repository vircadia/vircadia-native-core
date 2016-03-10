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
#include <QtCore/QLoggingCategory>
#include <QtCore/QTemporaryFile>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <Gzip.h>

#include <AssetClient.h>
#include <AssetUpload.h>
#include <ResourceManager.h>

#include "OffscreenUi.h"
#include "../ui/AssetUploadDialogFactory.h"

Q_DECLARE_LOGGING_CATEGORY(asset_migrator);
Q_LOGGING_CATEGORY(asset_migrator, "hf.asset_migrator");

ATPAssetMigrator& ATPAssetMigrator::getInstance() {
    static ATPAssetMigrator instance;
    return instance;
}

static const QString ENTITIES_OBJECT_KEY = "Entities";
static const QString MODEL_URL_KEY = "modelURL";
static const QString MESSAGE_BOX_TITLE = "ATP Asset Migration";

void ATPAssetMigrator::loadEntityServerFile() {
    auto filename = QFileDialog::getOpenFileName(_dialogParent, "Select an entity-server content file to migrate",
                                                 QString(), QString("Entity-Server Content (*.gz)"));
    
    if (!filename.isEmpty()) {
        qCDebug(asset_migrator) << "Selected filename for ATP asset migration: " << filename;
        
        static const QString MIGRATION_CONFIRMATION_TEXT {
            "The ATP Asset Migration process will scan the selected entity-server file, upload discovered resources to the"\
            " current asset-server and then save a new entity-server file with the ATP URLs.\n\nAre you ready to"\
            " continue?\n\nMake sure you are connected to the right domain."
        };
        
        auto button = OffscreenUi::question(_dialogParent, MESSAGE_BOX_TITLE, MIGRATION_CONFIRMATION_TEXT,
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        
        if (button == QMessageBox::No) {
            return;
        }
        
        // try to open the file at the given filename
        QFile modelsFile { filename };
        
        if (modelsFile.open(QIODevice::ReadOnly)) {
            QByteArray compressedJsonData = modelsFile.readAll();
            QByteArray jsonData;
            
            if (!gunzip(compressedJsonData, jsonData)) {
                OffscreenUi::warning(_dialogParent, "Error", "The file at" + filename + "was not in gzip format.");
            }
            
            QJsonDocument modelsJSON = QJsonDocument::fromJson(jsonData);
            _entitiesArray = modelsJSON.object()["Entities"].toArray();
            
            for (auto jsonValue : _entitiesArray) {
                QJsonObject entityObject = jsonValue.toObject();
                QString modelURLString = entityObject.value(MODEL_URL_KEY).toString();
                
                if (!modelURLString.isEmpty()) {
                    QUrl modelURL = QUrl(modelURLString);
                    
                    if (!_ignoredUrls.contains(modelURL)
                        && (modelURL.scheme() == URL_SCHEME_HTTP || modelURL.scheme() == URL_SCHEME_HTTPS
                            || modelURL.scheme() == URL_SCHEME_FILE || modelURL.scheme() == URL_SCHEME_FTP)) {
                        
                        if (_pendingReplacements.contains(modelURL)) {
                            // we already have a request out for this asset, just store the QJsonValueRef
                            // so we can do the hash replacement when the request comes back
                            _pendingReplacements.insert(modelURL, jsonValue);
                        } else if (_uploadedAssets.contains(modelURL)) {
                            // we already have a hash for this asset
                            // so just do the replacement immediately
                            entityObject[MODEL_URL_KEY] = _uploadedAssets.value(modelURL).toString();
                            jsonValue = entityObject;
                        } else if (wantsToMigrateResource(modelURL)) {
                            auto request = ResourceManager::createResourceRequest(this, modelURL);
                            
                            if (request) {
                                qCDebug(asset_migrator) << "Requesting" << modelURL << "for ATP asset migration";
                                
                                // add this combination of QUrl and QJsonValueRef to our multi hash so we can change the URL
                                // to an ATP one once ready
                                _pendingReplacements.insert(modelURL, jsonValue);
                                
                                connect(request, &ResourceRequest::finished, this, [=]() {
                                    if (request->getResult() == ResourceRequest::Success) {
                                        migrateResource(request);
                                    } else {
                                        OffscreenUi::warning(_dialogParent, "Error",
                                                             QString("Could not retrieve asset at %1").arg(modelURL.toString()));
                                    }
                                    request->deleteLater();
                                });
                                
                                request->send();
                            } else {
                                OffscreenUi::warning(_dialogParent, "Error",
                                                     QString("Could not create request for asset at %1").arg(modelURL.toString()));
                            }
                            
                        } else {
                            _ignoredUrls.insert(modelURL);
                        }
                    }
                }
            }
            
            _doneReading = true;
            
        } else {
            OffscreenUi::warning(_dialogParent, "Error",
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
        
        qCDebug(asset_migrator) << "Starting upload of asset from" << request->getUrl();
        
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
        
        
        QString atpURL = getATPUrl(hash, upload->getExtension()).toString();
        
        for (auto value : values) {
            // replace the modelURL in this QJsonValueRef with the hash
            QJsonObject valueObject = value.toObject();
            valueObject[MODEL_URL_KEY] = atpURL;
            value = valueObject;
        }
        
        // add this URL to our list of uploaded assets
        _uploadedAssets.insert(modelURL, atpURL);
        
        // pull the replaced models from _pendingReplacements
        _pendingReplacements.remove(modelURL);
        
        // are we out of pending replacements? if so it is time to save the entity-server file
        if (_doneReading && _pendingReplacements.empty()) {
            saveEntityServerFile();
            
            // reset after the attempted save, success or fail
            reset();
        }
    } else {
        AssetUploadDialogFactory::showErrorDialog(upload, _dialogParent);
    }
    
    upload->deleteLater();
}

bool ATPAssetMigrator::wantsToMigrateResource(const QUrl& url) {
    static bool hasAskedForCompleteMigration { false };
    static bool wantsCompleteMigration { false };
    
    if (!hasAskedForCompleteMigration) {
        // this is the first resource migration - ask the user if they just want to migrate everything
        static const QString COMPLETE_MIGRATION_TEXT { "Do you want to migrate all assets found in this entity-server file?\n\n"\
            "Select \"Yes\" to upload all discovered assets to the current asset-server immediately.\n\n"\
            "Select \"No\" to be prompted for each discovered asset."
        };
        
        auto button = OffscreenUi::question(_dialogParent, MESSAGE_BOX_TITLE, COMPLETE_MIGRATION_TEXT,
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                              
        if (button == QMessageBox::Yes) {
            wantsCompleteMigration = true;
        }
        
        hasAskedForCompleteMigration = true;
    }
    
    if (wantsCompleteMigration) {
        return true;
    } else {
        // present a dialog asking the user if they want to migrate this specific resource
        auto button = OffscreenUi::question(_dialogParent, MESSAGE_BOX_TITLE,
                                            "Would you like to migrate the following resource?\n" + url.toString(),
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return button == QMessageBox::Yes;
    }
}

void ATPAssetMigrator::saveEntityServerFile() {
    // show a dialog to ask the user where they want to save the file
    QString saveName = QFileDialog::getSaveFileName(_dialogParent, "Save Migrated Entities File");
    
    QFile saveFile { saveName };
    
    if (saveFile.open(QIODevice::WriteOnly)) {
        QJsonObject rootObject;
        rootObject[ENTITIES_OBJECT_KEY] = _entitiesArray;
        
        QJsonDocument newDocument { rootObject };
        QByteArray jsonDataForFile;
        
        if (gzip(newDocument.toJson(), jsonDataForFile, -1)) {
            
            saveFile.write(jsonDataForFile);
            saveFile.close();
            
            QMessageBox::information(_dialogParent, "Success",
                                     QString("Your new entities file has been saved at %1").arg(saveName));
        } else {
            OffscreenUi::warning(_dialogParent, "Error", "Could not gzip JSON data for new entities file.");
        }
    
    } else {
        OffscreenUi::warning(_dialogParent, "Error",
                             QString("Could not open file at %1 to write new entities file to.").arg(saveName));
    }
}

void ATPAssetMigrator::reset() {
    _entitiesArray = QJsonArray();
    _doneReading = false;
    _pendingReplacements.clear();
    _uploadedAssets.clear();
    _originalURLs.clear();
    _ignoredUrls.clear();
}
