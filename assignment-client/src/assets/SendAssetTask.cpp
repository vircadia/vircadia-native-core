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

SendAssetTask::SendAssetTask(QSharedPointer<udt::Packet> packet, const SharedNodePointer& sendToNode, const QDir& resourcesDir) :
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
    
    QString hexHash = _assetHash.toHex();
    
    qDebug() << "Received a request for the file (" << messageID << "): " << hexHash << " from " << start << " to " << end;
    
    qDebug() << "Starting task to send asset: " << hexHash << " for messageID " << _messageID;
    auto replyPacketList = std::unique_ptr<NLPacketList>(new NLPacketList(PacketType::AssetGetReply, QByteArray(), true, true));

    replyPacketList->write(_assetHash);

    replyPacketList->writePrimitive(_messageID);

    if (_end <= _start) {
        writeError(replyPacketList.get(), AssetServerError::INVALID_BYTE_RANGE);
    } else {
        QString filePath = _resourcesDir.filePath(QString(hexHash) + "." + QString(extension));
        
        QFile file { _filePath };

        if (file.open(QIODevice::ReadOnly)) {
            if (file.size() < _end) {
                writeError(replyPacketList.get(), AssetServerError::INVALID_BYTE_RANGE);
                qCDebug(networking) << "Bad byte range: " << hexHash << " " << _start << ":" << _end;
            } else {
                auto size = _end - _start;
                file.seek(_start);
                replyPacketList->writePrimitive(AssetServerError::NO_ERROR);
                replyPacketList->writePrimitive(size);
                replyPacketList->write(file.read(size));
                qCDebug(networking) << "Sending asset: " << hexHash;
            }
            file.close();
        } else {
            qCDebug(networking) << "Asset not found: " << _filePath << "(" << hexHash << ")";
            writeError(replyPacketList.get(), AssetServerError::ASSET_NOT_FOUND);
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacketList(std::move(replyPacketList), *_senderNode);
}
