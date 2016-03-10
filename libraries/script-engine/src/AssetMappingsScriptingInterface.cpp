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

AssetMappingModel::AssetMappingModel() {
}

AssetMappingModel::~AssetMappingModel() {
    qDebug() << " DEST";
}

void AssetMappingModel::refresh() {
    qDebug() << "Refreshing asset mapping model";
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetAllMappingsRequest();

    connect(request, &GetAllMappingsRequest::finished, this, [this](GetAllMappingsRequest* request) mutable {
        auto mappings = request->getMappings();
        auto existingPaths = _pathToItemMap.keys();
        for (auto& mapping : mappings) {
            auto& path = mapping.first;
            auto parts = path.split("/");
            auto length = parts.length();

            existingPaths.removeOne(mapping.first);

            QString fullPath = "";

            QStandardItem* lastItem = nullptr;

            for (int i = 0; i < length; ++i) {
                fullPath += (i == 0 ? "" : "/") + parts[i];

                auto it = _pathToItemMap.find(fullPath);
                if (it == _pathToItemMap.end()) {
                    qDebug() << "prefix not found: " << fullPath;
                    auto item = new QStandardItem(parts[i]);
                    bool isFolder = i < length - 1;
                    item->setData(isFolder ? fullPath + "/" : fullPath, Qt::UserRole);
                    item->setData(isFolder, Qt::UserRole + 1);
                    if (lastItem) {
                        lastItem->setChild(lastItem->rowCount(), 0, item);
                    } else {
                        appendRow(item);
                    }

                    lastItem = item;
                    _pathToItemMap[fullPath] = lastItem;
                }
                else {
                    lastItem = it.value();
                }
            }

            Q_ASSERT(fullPath == path);
        }

        // Remove folders from list
        auto it = existingPaths.begin();
        while (it != existingPaths.end()) {
            auto item = _pathToItemMap[*it];
            if (item->data(Qt::UserRole + 1).toBool()) {
                it = existingPaths.erase(it);
            } else {
                ++it;
            }
        }

        for (auto& path : existingPaths) {
            Q_ASSERT(_pathToItemMap.contains(path));
            qDebug() << "removing existing: " << path;

            auto item = _pathToItemMap[path];

            while (item) {
                // During each iteration, delete item
                QStandardItem* nextItem = nullptr;

                auto parent = item->parent();
                if (parent) {
                    parent->removeRow(item->row());
                    if (parent->rowCount() > 0) {
                        // The parent still contains children, set the nextItem to null so we stop processing
                        nextItem = nullptr;
                    } else {
                        nextItem = parent;
                    }
                } else {
                    removeRow(item->row());
                }

                _pathToItemMap.remove(path);
                //delete item;

                item = nextItem;
            }
            //removeitem->index();
        }
    });

    request->start();
}
