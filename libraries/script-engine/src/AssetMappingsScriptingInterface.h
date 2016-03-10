//
//  AssetMappingsScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Ryan Huffman on 2016-03-09.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_AssetMappingsScriptingInterface_h
#define hifi_AssetMappingsScriptingInterface_h

#include <QtCore/QObject>
#include <QtScript/QScriptValue>

#include <AssetClient.h>

 class AssetMappingItem : public QStandardItem {
 public:
     AssetMappingItem(const QString& name, const QString& fullPath, bool isFolder);

     QString name;
     QString fullPath;
     bool isFolder;
 };


 class AssetMappingModel : public QStandardItemModel {
     Q_OBJECT
 public:
     AssetMappingModel();
     ~AssetMappingModel();

//     QVariant AssetMappingModel::data(const QModelIndex& index, int role) const;

     Q_INVOKABLE void refresh();

 private:
     QHash<QString, QStandardItem*> _pathToItemMap;
 };


class AssetMappingsScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(AssetMappingModel* mappingModel READ getAssetMappingModel CONSTANT)
public:
    Q_INVOKABLE AssetMappingModel* getAssetMappingModel() { return &_assetMappingModel; }

    Q_INVOKABLE void setMapping(QString path, QString hash, QJSValue callback);
    Q_INVOKABLE void getMapping(QString path, QJSValue callback);
    Q_INVOKABLE void deleteMappings(QStringList paths, QJSValue callback);
    Q_INVOKABLE void deleteMapping(QString path, QJSValue callback) { deleteMappings(QStringList(path), callback); }
    Q_INVOKABLE void getAllMappings(QJSValue callback);
    Q_INVOKABLE void renameMapping(QString oldPath, QString newPath, QJSValue callback);
protected:
    QSet<AssetRequest*> _pendingRequests;
    AssetMappingModel _assetMappingModel;
};


#endif // hifi_AssetMappingsScriptingInterface_h
