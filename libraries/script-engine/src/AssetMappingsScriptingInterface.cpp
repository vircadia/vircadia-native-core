//
//  AssetMappingsScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Ryan Huffman on 2016-03-09.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetMappingsScriptingInterface.h"

#include <QtScript/QScriptEngine>

#include <AssetRequest.h>
#include <AssetUpload.h>
#include <MappingRequest.h>
#include <NetworkLogging.h>

AssetMappingsScriptingInterface::AssetMappingsScriptingInterface() {
}

void AssetMappingsScriptingInterface::setMapping(QString path, QString hash, QJSValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createSetMappingRequest(path, hash);

    connect(request, &SetMappingRequest::finished, this, [this, callback](SetMappingRequest* request) mutable {
        QJSValueList args { uint8_t(request->getError()) };

        callback.call(args);

        request->deleteLater();
         
    });

    request->start();
}

void AssetMappingsScriptingInterface::getMapping(QString path, QJSValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetMappingRequest(path);

    connect(request, &GetMappingRequest::finished, this, [this, callback](GetMappingRequest* request) mutable {
        QJSValueList args { uint8_t(request->getError()), request->getHash() };

        callback.call(args);

        request->deleteLater();
         
    });

    request->start();
}

void AssetMappingsScriptingInterface::deleteMappings(QStringList paths, QJSValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createDeleteMappingsRequest(paths);

    connect(request, &DeleteMappingsRequest::finished, this, [this, callback](DeleteMappingsRequest* request) mutable {
        QJSValueList args { uint8_t(request->getError()) };

        callback.call(args);

        request->deleteLater();
         
    });

    request->start();
}

void AssetMappingsScriptingInterface::getAllMappings(QJSValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetAllMappingsRequest();

    connect(request, &GetAllMappingsRequest::finished, this, [this, callback](GetAllMappingsRequest* request) mutable {
        auto mappings = request->getMappings();
        auto map = callback.engine()->newObject();

        for (auto& kv : mappings ) {
            map.setProperty(kv.first, kv.second);
        }

        QJSValueList args { uint8_t(request->getError()), map };

        callback.call(args);

        request->deleteLater();
         
    });

    request->start();
}

void AssetMappingsScriptingInterface::renameMapping(QString oldPath, QString newPath, QJSValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createRenameMappingRequest(oldPath, newPath);

    connect(request, &RenameMappingRequest::finished, this, [this, callback](RenameMappingRequest* request) mutable {
        QJSValueList args { uint8_t(request->getError()) };
        
        callback.call(args);
        
        request->deleteLater();
        
    });
    
    request->start();
}


AssetMappingItem::AssetMappingItem(const QString& name, const QString& fullPath, bool isFolder)
    : name(name),
    fullPath(fullPath),
    isFolder(isFolder) {

}

static int assetMappingModelMetatypeId = qRegisterMetaType<AssetMappingModel*>("AssetMappingModel*");

AssetMappingModel::AssetMappingModel(QObject* parent) {
}

void AssetMappingModel::refresh() {
    qDebug() << "Refreshing asset mapping model";
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetAllMappingsRequest();

    connect(request, &GetAllMappingsRequest::finished, this, [this](GetAllMappingsRequest* request) mutable {
        auto mappings = request->getMappings();
        for (auto& mapping : mappings) {
            auto& path = mapping.first;
            auto parts = path.split("/");
            auto length = parts.length();

            QString fullPath = parts[0];

            QStandardItem* lastItem = nullptr;

            auto it = _pathToItemMap.find(fullPath);
            if (it == _pathToItemMap.end()) {
                lastItem = new QStandardItem(parts[0]);
                lastItem->setData(parts[0], Qt::UserRole + 1);
                lastItem->setData(fullPath, Qt::UserRole + 2);
                _pathToItemMap[fullPath] = lastItem;
                appendRow(lastItem);
            }
            else {
                lastItem = it.value();
            }

            if (length > 1) {
                for (int i = 1; i < length; ++i) {
                    fullPath += "/" + parts[i];

                    auto it = _pathToItemMap.find(fullPath);
                    if (it == _pathToItemMap.end()) {
                        qDebug() << "prefix not found: " << fullPath;
                        auto item = new QStandardItem(parts[i]);
                        bool isFolder = i < length - 1;
                        item->setData(isFolder ? fullPath + "/" : fullPath, Qt::UserRole);
                        lastItem->setChild(lastItem->rowCount(), 0, item);
                        lastItem = item;
                        _pathToItemMap[fullPath] = lastItem;
                    }
                    else {
                        lastItem = it.value();
                    }
                }
            }

            Q_ASSERT(fullPath == path);
        }
    });

    request->start();
}

// QModelIndex AssetMappingModel::index(int row, int column, const QModelIndex& parent) const {
//     if (row < 0 || column < 0) {
//         return QModelIndex();
//     }

//     if (parent.isValid()) {
//         auto item = static_cast<AssetMappingItem*>(parent.internalPointer());
//         return createIndex(row, column, )
//     }
//     return createIndex(row, column, getFolderNodes(
//         static_cast<AssetMappingItem*>(getTreeNodeFromIndex(parent))).at(row));
// }

// QModelIndex AssetMappingModel::parent(const QModelIndex& child) const {
//     AssetMappingItem* parent = (static_cast<AssetMappingItem*>(child.internalPointer()))->getParent();
//     if (!parent) {
//         return QModelIndex();
//     }
//     AssetMappingItem* grandParent = parent->getParent();
//     int row = getFolderNodes(grandParent).indexOf(parent);
//     return createIndex(row, 0, parent);
// }

// QVariant AssetMappingModel::data(const QModelIndex& index, int role) const {
//     if (index.isValid()) {
//         AssetMappingItem* item = (static_cast<AssetMappingItem*>(index.internalPointer()));
//         if (item) {
//             return item->name;
//         }
//     }
//     return QVariant();
// }
//
// int AssetMappingModel::rowCount(const QModelIndex& parent) const {
//     return 1;
// }

// int AssetMappingModel::columnCount(const QModelIndex& parent) const {
//     return 1;
// }
