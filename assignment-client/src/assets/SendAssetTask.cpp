//
//  SendAssetTask.cpp
//  assignment-client/src/assets
//
//  Created by Ryan Huffman on 2015/08/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SendAssetTask.h"

#include <QFile>

#include <DependencyManager.h>
#include <NetworkLogging.h>
#include <NLPacket.h>
#include <NLPacketList.h>
#include <NodeList.h>
#include <udt/Packet.h>

#include "AssetUtils.h"

SendAssetTask::SendAssetTask(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& sendToNode, const QDir& resourcesDir) :
    QRunnable(),
    _message(message),
    _senderNode(sendToNode),
    _resourcesDir(resourcesDir)
{
    
}

void SendAssetTask::run() {
    MessageID messageID;
    DataOffset start, end;
    
    _message->readPrimitive(&messageID);
    QByteArray assetHash = _message->read(SHA256_HASH_LENGTH);

    // `start` and `end` indicate the range of data to retrieve for the asset identified by `assetHash`.
    // `start` is inclusive, `end` is exclusive. Requesting `start` = 1, `end` = 10 will retrieve 9 bytes of data,
    // starting at index 1.
    _message->readPrimitive(&start);
    _message->readPrimitive(&end);
    
    QString hexHash = assetHash.toHex();
    
    qDebug() << "Received a request for the file (" << messageID << "): " << hexHash << " from " << start << " to " << end;
    
    qDebug() << "Starting task to send asset: " << hexHash << " for messageID " << messageID;
    auto replyPacketList = NLPacketList::create(PacketType::AssetGetReply, QByteArray(), true, true);

    replyPacketList->write(assetHash);

    replyPacketList->writePrimitive(messageID);

    if (end <= start) {
        replyPacketList->writePrimitive(AssetServerError::InvalidByteRange);
    } else {
        QString filePath = _resourcesDir.filePath(QString(hexHash));
        
        QFile file { filePath };

        if (file.open(QIODevice::ReadOnly)) {
            if (file.size() < end) {
                replyPacketList->writePrimitive(AssetServerError::InvalidByteRange);
                qCDebug(networking) << "Bad byte range: " << hexHash << " " << start << ":" << end;
            } else {
                auto size = end - start;
                file.seek(start);
                replyPacketList->writePrimitive(AssetServerError::NoError);
                replyPacketList->writePrimitive(size);
                replyPacketList->write(file.read(size));
                qCDebug(networking) << "Sending asset: " << hexHash;
            }
            file.close();
        } else {
            qCDebug(networking) << "Asset not found: " << filePath << "(" << hexHash << ")";
            replyPacketList->writePrimitive(AssetServerError::AssetNotFound);
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacketList(std::move(replyPacketList), *_senderNode);
}
