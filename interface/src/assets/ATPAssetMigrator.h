//
//  ATPAssetMigrator.h
//  interface/src/assets
//
//  Created by Stephen Birarda on 2015-10-12.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_ATPAssetMigrator_h
#define hifi_ATPAssetMigrator_h

#include <QtCore/QJsonArray>
#include <QtCore/QObject>
#include <QtCore/QMultiHash>
#include <QtCore/QSet>

class AssetUpload;
class ResourceRequest;
class SetMappingRequest;

class ATPAssetMigrator : public QObject {
    Q_OBJECT
public:
    static ATPAssetMigrator& getInstance();
    
    void setDialogParent(QWidget* dialogParent) { _dialogParent = dialogParent; }
public slots:
    void loadEntityServerFile();
private slots:
    void assetUploadFinished(AssetUpload* upload, const QString& hash);
    void setMappingFinished(SetMappingRequest* request);
private:
    void migrateResource(ResourceRequest* request);

    void checkIfFinished();
    void saveEntityServerFile();
    
    void reset();
    
    bool wantsToMigrateResource(const QUrl& url);
    
    QWidget* _dialogParent = nullptr;
    QJsonArray _entitiesArray;
    
    bool _doneReading { false };

    using JSONTypePair = std::pair<QJsonValueRef, quint8>;

    QMultiHash<QUrl, JSONTypePair> _pendingReplacements;
    QHash<QUrl, QUrl> _uploadedAssets;
    QHash<QObject*, QUrl> _originalURLs;
    QSet<QUrl> _ignoredUrls;
    int _errorCount { 0 };
};


#endif // hifi_ATPAssetMigrator_h
