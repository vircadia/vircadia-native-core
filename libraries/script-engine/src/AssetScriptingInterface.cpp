//
//  AssetScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetScriptingInterface.h"

#include <QtScript/QScriptEngine>

#include <AssetRequest.h>
#include <AssetUpload.h>
#include <MappingRequest.h>
#include <NetworkLogging.h>

AssetScriptingInterface::AssetScriptingInterface(QScriptEngine* engine) :
    _engine(engine)
{

}

void AssetScriptingInterface::uploadData(QString data, QScriptValue callback) {
    QByteArray dataByteArray = data.toUtf8();
    auto upload = DependencyManager::get<AssetClient>()->createUpload(dataByteArray);
    if (!upload) {
        qCWarning(asset_client) << "Error uploading file to asset server";
        return;
    }

    QObject::connect(upload, &AssetUpload::finished, this, [this, callback](AssetUpload* upload, const QString& hash) mutable {
        if (callback.isFunction()) {
            QString url = "atp://" + hash;
            QScriptValueList args { url };
            callback.call(_engine->currentContext()->thisObject(), args);
        }
    });
    upload->start();
}

void AssetScriptingInterface::downloadData(QString urlString, QScriptValue callback) {
    const QString ATP_SCHEME { "atp://" };

    if (!urlString.startsWith(ATP_SCHEME)) {
        return;
    }

    // Make request to atp
    auto path = urlString.right(urlString.length() - ATP_SCHEME.length());
    auto parts = path.split(".", QString::SkipEmptyParts);
    auto hash = parts.length() > 0 ? parts[0] : "";

    if (hash.length() != SHA256_HASH_HEX_LENGTH) {
        return;
    }

    auto assetClient = DependencyManager::get<AssetClient>();
    auto assetRequest = assetClient->createRequest(hash);

    if (!assetRequest) {
        return;
    }

    _pendingRequests << assetRequest;

    connect(assetRequest, &AssetRequest::finished, this, [this, callback](AssetRequest* request) mutable {
        Q_ASSERT(request->getState() == AssetRequest::Finished);

        if (request->getError() == AssetRequest::Error::NoError) {
            if (callback.isFunction()) {
                QString data = QString::fromUtf8(request->getData());
                QScriptValueList args { data };
                callback.call(_engine->currentContext()->thisObject(), args);
            }
        }

        request->deleteLater();
        _pendingRequests.remove(request);
    });

    assetRequest->start();
}

void AssetScriptingInterface::setMapping(QString path, QString hash, QScriptValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createSetMappingRequest(path, hash);

    connect(request, &SetMappingRequest::finished, this, [this, callback](SetMappingRequest* request) mutable {
        QScriptValueList args { uint8_t(request->getError()) };

        callback.call(_engine->currentContext()->thisObject(), args);

        request->deleteLater();

    });

    request->start();
}

void AssetScriptingInterface::getMapping(QString path, QScriptValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetMappingRequest(path);

    connect(request, &GetMappingRequest::finished, this, [this, callback](GetMappingRequest* request) mutable {
        QScriptValueList args { uint8_t(request->getError()), request->getHash() };

        callback.call(_engine->currentContext()->thisObject(), args);

        request->deleteLater();

    });

    request->start();
}

void AssetScriptingInterface::deleteMappings(QStringList paths, QScriptValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createDeleteMappingsRequest(paths);

    connect(request, &DeleteMappingsRequest::finished, this, [this, callback](DeleteMappingsRequest* request) mutable {
        QScriptValueList args { uint8_t(request->getError()) };

        callback.call(_engine->currentContext()->thisObject(), args);

        request->deleteLater();

    });

    request->start();
}

void AssetScriptingInterface::getAllMappings(QScriptValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetAllMappingsRequest();

    connect(request, &GetAllMappingsRequest::finished, this, [this, callback](GetAllMappingsRequest* request) mutable {
        auto mappings = request->getMappings();
        auto map = callback.engine()->newObject();

        for (auto& kv : mappings ) {
            map.setProperty(kv.first, kv.second);
        }

        QScriptValueList args { uint8_t(request->getError()), map };

        callback.call(_engine->currentContext()->thisObject(), args);

        request->deleteLater();

    });

    request->start();
}

void AssetScriptingInterface::renameMapping(QString oldPath, QString newPath, QScriptValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createRenameMappingRequest(oldPath, newPath);

    connect(request, &RenameMappingRequest::finished, this, [this, callback](RenameMappingRequest* request) mutable {
        QScriptValueList args { uint8_t(request->getError()) };
        
        callback.call(_engine->currentContext()->thisObject(), args);
        
        request->deleteLater();
        
    });
    
    request->start();
}
