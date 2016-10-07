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

    QObject::connect(upload, &AssetUpload::finished, this, [this, callback](AssetUpload* upload, const QString& hash) mutable {
        if (callback.isFunction()) {
            QString url = "atp:" + hash;
            QScriptValueList args { url, hash };
            callback.call(_engine->currentContext()->thisObject(), args);
        }
        upload->deleteLater();
    });
    upload->start();
}

void AssetScriptingInterface::setMapping(QString path, QString hash, QScriptValue callback) {
    auto setMappingRequest = DependencyManager::get<AssetClient>()->createSetMappingRequest(path, hash);

    QObject::connect(setMappingRequest, &SetMappingRequest::finished, this, [this, callback](SetMappingRequest* request) mutable {
        if (callback.isFunction()) {
            QScriptValueList args { };
            callback.call(_engine->currentContext()->thisObject(), args);
        }
        request->deleteLater();
    });
    setMappingRequest->start();
}


void AssetScriptingInterface::downloadData(QString urlString, QScriptValue callback) {
    const QString ATP_SCHEME { "atp:" };

    if (!urlString.startsWith(ATP_SCHEME)) {
        return;
    }

    // Make request to atp
    auto path = urlString.right(urlString.length() - ATP_SCHEME.length());
    auto parts = path.split(".", QString::SkipEmptyParts);
    auto hash = parts.length() > 0 ? parts[0] : "";

    auto assetClient = DependencyManager::get<AssetClient>();
    auto assetRequest = assetClient->createRequest(hash);

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
