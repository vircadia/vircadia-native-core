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

#include <cmath>

#include <QFile>

#include <DependencyManager.h>
#include <NetworkLogging.h>
#include <NLPacket.h>
#include <NLPacketList.h>
#include <NodeList.h>
#include <udt/Packet.h>

#include "AssetUtils.h"
#include "ByteRange.h"
#include "ClientServerUtils.h"

SendAssetTask::SendAssetTask(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& sendToNode, const QDir& resourcesDir) :
    QRunnable(),
    _message(message),
    _senderNode(sendToNode),
    _resourcesDir(resourcesDir)
{
    
}

void SendAssetTask::run() {
    MessageID messageID;
    ByteRange byteRange;

    _message->readPrimitive(&messageID);
    QByteArray assetHash = _message->read(SHA256_HASH_LENGTH);

    // `start` and `end` indicate the range of data to retrieve for the asset identified by `assetHash`.
    // `start` is inclusive, `end` is exclusive. Requesting `start` = 1, `end` = 10 will retrieve 9 bytes of data,
    // starting at index 1.
    _message->readPrimitive(&byteRange.fromInclusive);
    _message->readPrimitive(&byteRange.toExclusive);
    
    QString hexHash = assetHash.toHex();
    
    qDebug() << "Received a request for the file (" << messageID << "): " << hexHash << " from "
        << byteRange.fromInclusive << " to " << byteRange.toExclusive;
    
    qDebug() << "Starting task to send asset: " << hexHash << " for messageID " << messageID;
    auto replyPacketList = NLPacketList::create(PacketType::AssetGetReply, QByteArray(), true, true);

    replyPacketList->write(assetHash);

    replyPacketList->writePrimitive(messageID);

    if (!byteRange.isValid()) {
        replyPacketList->writePrimitive(AssetServerError::InvalidByteRange);
    } else {
        QString filePath = _resourcesDir.filePath(QString(hexHash));
        
        QFile file { filePath };

        if (file.open(QIODevice::ReadOnly)) {

            // first fixup the range based on the now known file size
            byteRange.fixupRange(file.size());

            // check if we're being asked to read data that we just don't have
            // because of the file size
            if (file.size() < byteRange.fromInclusive || file.size() < byteRange.toExclusive) {
                replyPacketList->writePrimitive(AssetServerError::InvalidByteRange);
                qCDebug(networking) << "Bad byte range: " << hexHash << " "
                    << byteRange.fromInclusive << ":" << byteRange.toExclusive;
            } else {
                // we have a valid byte range, handle it and send the asset
                auto size = byteRange.size();

                if (byteRange.fromInclusive >= 0) {

                    // this range is positive, meaning we just need to seek into the file and then read from there
                    file.seek(byteRange.fromInclusive);
                    replyPacketList->writePrimitive(AssetServerError::NoError);
                    replyPacketList->writePrimitive(size);
                    replyPacketList->write(file.read(size));
                } else {
                    // this range is negative, at least the first part of the read will be back into the end of the file

                    // seek to the part of the file where the negative range begins
                    file.seek(file.size() + byteRange.fromInclusive);

                    replyPacketList->writePrimitive(AssetServerError::NoError);
                    replyPacketList->writePrimitive(size);

                    // first write everything from the negative range to the end of the file
                    replyPacketList->write(file.read(size));
                }

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
