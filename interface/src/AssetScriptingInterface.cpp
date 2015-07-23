//
//  AssetScriptingInterface.cpp
//
//  Created by Ryan Huffman on 2015/07/22
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetScriptingInterface.h"

#include <QScriptEngine>

#include <AssetClient.h>
#include <DependencyManager.h>
#include <NLPacket.h>
#include <NodeList.h>
#include <PacketReceiver.h>

#include <AssetManager.h>

const int HASH_HEX_LENGTH = 32;

AssetScriptingInterface::AssetScriptingInterface() {
}

QScriptValue AssetScriptingInterface::getAsset(QString url, QScriptValue callback) {
    bool success = AssetManager::getAsset(QUrl(url), [callback](AssetRequestUpdateType type, QByteArray data) mutable {
        auto result = callback.engine()->newVariant(data);
        QList<QScriptValue> arguments { type == AssetRequestUpdateType::COMPLETE, result };
        callback.call(QScriptValue(), arguments);
    });
    return success;
}

QScriptValue AssetScriptingInterface::uploadAsset(QString data, QString extension, QScriptValue callback) {
    auto assetClient = DependencyManager::get<AssetClient>();
    return assetClient->uploadAsset(data.toLatin1(), extension, [callback](bool success, QString hash) mutable {
        QList<QScriptValue> arguments { success, hash };
        auto result = callback.call(QScriptValue(), arguments);
    });
}
