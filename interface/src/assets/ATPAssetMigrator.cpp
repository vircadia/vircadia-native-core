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
#include <MappingRequest.h>

#include "OffscreenUi.h"

Q_DECLARE_LOGGING_CATEGORY(asset_migrator);
Q_LOGGING_CATEGORY(asset_migrator, "hf.asset_migrator");

ATPAssetMigrator& ATPAssetMigrator::getInstance() {
    static ATPAssetMigrator instance;
    return instance;
}

static const QString ENTITIES_OBJECT_KEY = "Entities";
static const QString MODEL_URL_KEY = "modelURL";
static const QString COMPOUND_SHAPE_URL_KEY = "compoundShapeURL";
static const QString MESSAGE_BOX_TITLE = "ATP Asset Migration";

void ATPAssetMigrator::loadEntityServerFile() {
    auto filename = OffscreenUi::getOpenFileName(_dialogParent, tr("Select an entity-server content file to migrate"), QString(), tr("Entity-Server Content (*.gz)"));
    
    if (!filename.isEmpty()) {
        qCDebug(asset_migrator) << "Selected filename for ATP asset migration: " << filename;
        
        static const QString MIGRATION_CONFIRMATION_TEXT {
            "The ATP Asset Migration process will scan the selected entity-server file,\nupload discovered resources to the"\
            " current asset-server\nand then save a new entity-server file with the ATP URLs.\n\nAre you ready to"\
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
                QString compoundURLString = entityObject.value(COMPOUND_SHAPE_URL_KEY).toString();

                for (int i = 0; i < 2; ++i) {
                    bool isModelURL = (i == 0);
                    quint8 replacementType = i;
                    auto migrationURLString = (isModelURL) ? modelURLString : compoundURLString;

                    if (!migrationURLString.isEmpty()) {
                        QUrl migrationURL = QUrl(migrationURLString);

                        if (!_ignoredUrls.contains(migrationURL)
                            && (migrationURL.scheme() == URL_SCHEME_HTTP || migrationURL.scheme() == URL_SCHEME_HTTPS
                                || migrationURL.scheme() == URL_SCHEME_FILE || migrationURL.scheme() == URL_SCHEME_FTP)) {

                                if (_pendingReplacements.contains(migrationURL)) {
                                    // we already have a request out for this asset, just store the QJsonValueRef
                                    // so we can do the hash replacement when the request comes back
                                    _pendingReplacements.insert(migrationURL, { jsonValue, replacementType });
                                } else if (_uploadedAssets.contains(migrationURL)) {
                                    // we already have a hash for this asset
                                    // so just do the replacement immediately
                                    if (isModelURL) {
                                        entityObject[MODEL_URL_KEY] = _uploadedAssets.value(migrationURL).toString();
                                    } else {
                                        entityObject[COMPOUND_SHAPE_URL_KEY] = _uploadedAssets.value(migrationURL).toString();
                                    }

                                    jsonValue = entityObject;
                                } else if (wantsToMigrateResource(migrationURL)) {
                                    auto request =
                                        DependencyManager::get<ResourceManager>()->createResourceRequest(this, migrationURL);

                                    if (request) {
                                        qCDebug(asset_migrator) << "Requesting" << migrationURL << "for ATP asset migration";

                                        // add this combination of QUrl and QJsonValueRef to our multi hash so we can change the URL
                                        // to an ATP one once ready
                                        _pendingReplacements.insert(migrationURL, { jsonValue, (isModelURL ? 0 : 1)});

                                        connect(request, &ResourceRequest::finished, this, [=]() {
                                            if (request->getResult() == ResourceRequest::Success) {
                                                migrateResource(request);
                                            } else {
                                                ++_errorCount;
                                                _pendingReplacements.remove(migrationURL);
                                                qWarning() << "Could not retrieve asset at" << migrationURL.toString();

                                                checkIfFinished();
                                            }
                                            request->deleteLater();
                                        });

                                        request->send();
                                    } else {
                                        ++_errorCount;
                                        qWarning() << "Count not create request for asset at" << migrationURL.toString();
                                    }
                                    
                                } else {
                                    _ignoredUrls.insert(migrationURL);
                                }
                            }
                    }
                }

            }
            
            _doneReading = true;

            checkIfFinished();
            
        } else {
            OffscreenUi::warning(_dialogParent, "Error",
                                 "There was a problem loading that entity-server file for ATP asset migration. Please try again");
        }
    }
}

void ATPAssetMigrator::migrateResource(ResourceRequest* request) {
    // use an asset client to upload the asset
    auto assetClient = DependencyManager::get<AssetClient>();
    
    auto upload = assetClient->createUpload(request->getData());

    // add this URL to our hash of AssetUpload to original URL
    _originalURLs.insert(upload, request->getUrl());

    qCDebug(asset_migrator) << "Starting upload of asset from" << request->getUrl();

    // connect to the finished signal so we know when the AssetUpload is done
    QObject::connect(upload, &AssetUpload::finished, this, &ATPAssetMigrator::assetUploadFinished);

    // start the upload now
    upload->start();
}

void ATPAssetMigrator::assetUploadFinished(AssetUpload *upload, const QString& hash) {
    // remove this migrationURL from the key for the AssetUpload pointer
    auto migrationURL = _originalURLs.take(upload);

    if (upload->getError() == AssetUpload::NoError) {

        // use the path of the migrationURL to add a mapping in the Asset Server
        auto assetClient = DependencyManager::get<AssetClient>();
        auto setMappingRequest = assetClient->createSetMappingRequest(migrationURL.path(), hash);

        connect(setMappingRequest, &SetMappingRequest::finished, this, &ATPAssetMigrator::setMappingFinished);

        // add this migrationURL with the key for the SetMappingRequest pointer
        _originalURLs[setMappingRequest] = migrationURL;

        setMappingRequest->start();
    } else {
        // this is a fail for this modelURL, remove it from pending replacements
        _pendingReplacements.remove(migrationURL);

        ++_errorCount;
        qWarning() << "Failed to upload" << migrationURL << "- error was" << upload->getErrorString();
    }

    checkIfFinished();
    
    upload->deleteLater();
}

void ATPAssetMigrator::setMappingFinished(SetMappingRequest* request) {
    // take the migrationURL for this SetMappingRequest
    auto migrationURL = _originalURLs.take(request);

    if (request->getError() == MappingRequest::NoError) {

        // successfully uploaded asset - make any required replacements found in the pending replacements
        auto values = _pendingReplacements.values(migrationURL);

        QString atpURL = QString("atp:%1").arg(request->getPath());

        for (auto value : values) {
            // replace the modelURL in this QJsonValueRef with the hash
            QJsonObject valueObject = value.first.toObject();

            if (value.second == 0) {
                valueObject[MODEL_URL_KEY] = atpURL;
            } else {
                valueObject[COMPOUND_SHAPE_URL_KEY] = atpURL;
            }

            value.first = valueObject;
        }

        // add this URL to our list of uploaded assets
        _uploadedAssets.insert(migrationURL, atpURL);

        // pull the replaced urls from _pendingReplacements
        _pendingReplacements.remove(migrationURL);
    } else {
        // this is a fail for this migrationURL, remove it from pending replacements
        _pendingReplacements.remove(migrationURL);

        ++_errorCount;
        qWarning() << "Error setting mapping for" << migrationURL << "- error was " << request->getErrorString();
    }

    checkIfFinished();

    request->deleteLater();
}

void ATPAssetMigrator::checkIfFinished() {
    // are we out of pending replacements? if so it is time to save the entity-server file
    if (_doneReading && _pendingReplacements.empty()) {
        saveEntityServerFile();

        // reset after the attempted save, success or fail
        reset();
    }
}

bool ATPAssetMigrator::wantsToMigrateResource(const QUrl& url) {
    static bool hasAskedForCompleteMigration { false };
    static bool wantsCompleteMigration { false };
    
    if (!hasAskedForCompleteMigration) {
        // this is the first resource migration - ask the user if they just want to migrate everything
        static const QString COMPLETE_MIGRATION_TEXT { "Do you want to migrate all assets found in this entity-server file?\n"\
            "Select \"Yes\" to upload all discovered assets to the current asset-server immediately.\n"\
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
    QString saveName = OffscreenUi::getSaveFileName(_dialogParent, "Save Migrated Entities File");
    
    QFile saveFile { saveName };
    
    if (saveFile.open(QIODevice::WriteOnly)) {
        QJsonObject rootObject;
        rootObject[ENTITIES_OBJECT_KEY] = _entitiesArray;
        
        QJsonDocument newDocument { rootObject };
        QByteArray jsonDataForFile;
        
        if (gzip(newDocument.toJson(), jsonDataForFile, -1)) {
            
            saveFile.write(jsonDataForFile);
            saveFile.close();

            QString infoMessage = QString("Your new entities file has been saved at\n%1.").arg(saveName);

            if (_errorCount > 0) {
                infoMessage += QString("\nThere were %1 models that could not be migrated.\n").arg(_errorCount);
                infoMessage += "Check the warnings in your log for details.\n";
                infoMessage += "You can re-attempt migration on those models\nby restarting this process with the newly saved file.";
            }

            OffscreenUi::information(_dialogParent, "Success", infoMessage);
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
    _errorCount = 0;
}
