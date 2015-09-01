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

SendAssetTask::SendAssetTask(QSharedPointer<NLPacket> packet, const SharedNodePointer& sendToNode, const QDir& resourcesDir) :
    QRunnable(),
    _packet(packet),
    _senderNode(sendToNode),
    _resourcesDir(resourcesDir)
{
    
}

void SendAssetTask::run() {
    MessageID messageID;
    uint8_t extensionLength;
    DataOffset start, end;
    
    _packet->readPrimitive(&messageID);
    QByteArray assetHash = _packet->read(SHA256_HASH_LENGTH);
    _packet->readPrimitive(&extensionLength);
    QByteArray extension = _packet->read(extensionLength);
    _packet->readPrimitive(&start);
    _packet->readPrimitive(&end);
    
    QString hexHash = assetHash.toHex();
    
    qDebug() << "Received a request for the file (" << messageID << "): " << hexHash << " from " << start << " to " << end;
    
    qDebug() << "Starting task to send asset: " << hexHash << " for messageID " << messageID;
    auto replyPacketList = std::unique_ptr<NLPacketList>(new NLPacketList(PacketType::AssetGetReply, QByteArray(), true, true));

    replyPacketList->write(assetHash);

    replyPacketList->writePrimitive(messageID);

    if (end <= start) {
        writeError(replyPacketList.get(), AssetServerError::InvalidByteRange);
    } else {
        QString filePath = _resourcesDir.filePath(QString(hexHash) + "." + QString(extension));
        
        QFile file { filePath };

        if (file.open(QIODevice::ReadOnly)) {
            if (file.size() < end) {
                writeError(replyPacketList.get(), AssetServerError::InvalidByteRange);
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
            writeError(replyPacketList.get(), AssetServerError::AssetNotFound);
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacketList(std::move(replyPacketList), *_senderNode);
}
