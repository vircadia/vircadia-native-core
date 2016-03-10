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
#include <QSortFilterProxyModel>

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
     Q_INVOKABLE void refresh();

     bool isKnownMapping(QString path) const { return _pathToItemMap.contains(path); };

 signals:
     void errorGettingMappings(int error);

 private:
     QHash<QString, QStandardItem*> _pathToItemMap;
 };


class AssetMappingsScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(AssetMappingModel* mappingModel READ getAssetMappingModel CONSTANT)
    Q_PROPERTY(QAbstractProxyModel* proxyModel READ getProxyModel CONSTANT)
public:
    AssetMappingsScriptingInterface();

    Q_INVOKABLE AssetMappingModel* getAssetMappingModel() { return &_assetMappingModel; }
    Q_INVOKABLE QAbstractProxyModel* getProxyModel() { return &_proxyModel; }

    Q_INVOKABLE bool isKnownMapping(QString path) const { return _assetMappingModel.isKnownMapping(path); };

    Q_INVOKABLE QString getErrorString(int errorCode) const;

    Q_INVOKABLE void setMapping(QString path, QString hash, QJSValue callback);
    Q_INVOKABLE void getMapping(QString path, QJSValue callback);
    Q_INVOKABLE void deleteMappings(QStringList paths, QJSValue callback);
    Q_INVOKABLE void deleteMapping(QString path, QJSValue callback) { deleteMappings(QStringList(path), callback); }
    Q_INVOKABLE void getAllMappings(QJSValue callback);
    Q_INVOKABLE void renameMapping(QString oldPath, QString newPath, QJSValue callback);

protected:
    QSet<AssetRequest*> _pendingRequests;
    AssetMappingModel _assetMappingModel;
    QSortFilterProxyModel _proxyModel;
};


#endif // hifi_AssetMappingsScriptingInterface_h
