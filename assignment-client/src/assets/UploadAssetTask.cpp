//
//  UploadAssetTask.cpp
//  assignment-client/src/assets
//
//  Created by Stephen Birarda on 2015-08-28.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UploadAssetTask.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>

#include <AssetUtils.h>
#include <NodeList.h>
#include <NLPacketList.h>


UploadAssetTask::UploadAssetTask(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode,
                                 const QDir& resourcesDir) :
    _packetList(packetList),
    _senderNode(senderNode),
    _resourcesDir(resourcesDir)
{
    
}

void UploadAssetTask::run() {
    auto data = _packetList->getMessage();
    
    QBuffer buffer { &data };
    buffer.open(QIODevice::ReadOnly);
    
    MessageID messageID;
    buffer.read(reinterpret_cast<char*>(&messageID), sizeof(messageID));
    
    uint8_t extensionLength;
    buffer.read(reinterpret_cast<char*>(&extensionLength), sizeof(extensionLength));
    
    QByteArray extension = buffer.read(extensionLength);
    
    uint64_t fileSize;
    buffer.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));
    
    qDebug() << "UploadAssetTask reading a file of " << fileSize << "bytes and extension" << extension << "from"
        << uuidStringWithoutCurlyBraces(_senderNode->getUUID());
    
    auto replyPacket = NLPacket::create(PacketType::AssetUploadReply);
    replyPacket->writePrimitive(messageID);
    
    if (fileSize > MAX_UPLOAD_SIZE) {
        replyPacket->writePrimitive(AssetServerError::AssetTooLarge);
    } else {
        QByteArray fileData = buffer.read(fileSize);
        
        auto hash = hashData(fileData);
        auto hexHash = hash.toHex();
        
        qDebug() << "Hash for uploaded file from" << uuidStringWithoutCurlyBraces(_senderNode->getUUID())
            << "is: (" << hexHash << ") ";
        
        QFile file { _resourcesDir.filePath(QString(hexHash)) + "." + QString(extension) };
        
        if (file.exists()) {
            qDebug() << "[WARNING] This file already exists: " << hexHash;
        } else {
            file.open(QIODevice::WriteOnly);
            file.write(fileData);
            file.close();
        }
        replyPacket->writePrimitive(AssetServerError::NoError);
        replyPacket->write(hash);
    }
    
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *_senderNode);
}
