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

#include "ClientServerUtils.h"

UploadAssetTask::UploadAssetTask(QSharedPointer<ReceivedMessage> receivedMessage, SharedNodePointer senderNode,
                                 const QDir& resourcesDir, uint64_t filesizeLimit) :
    _receivedMessage(receivedMessage),
    _senderNode(senderNode),
    _resourcesDir(resourcesDir),
    _filesizeLimit(filesizeLimit)
{
    
}

void UploadAssetTask::run() {
    auto data = _receivedMessage->getMessage();
    
    QBuffer buffer { &data };
    buffer.open(QIODevice::ReadOnly);
    
    MessageID messageID;
    buffer.read(reinterpret_cast<char*>(&messageID), sizeof(messageID));
    
    uint64_t fileSize;
    buffer.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

    if (_senderNode) {
        qDebug() << "UploadAssetTask reading a file of " << fileSize << "bytes from" << uuidStringWithoutCurlyBraces(_senderNode->getUUID());
    } else {
        qDebug() << "UploadAssetTask reading a file of " << fileSize << "bytes from" << _receivedMessage->getSenderSockAddr();
    }
    
    auto replyPacket = NLPacket::create(PacketType::AssetUploadReply, -1, true);
    replyPacket->writePrimitive(messageID);
    
    if (fileSize > _filesizeLimit) {
        replyPacket->writePrimitive(AssetUtils::AssetServerError::AssetTooLarge);
    } else {
        QByteArray fileData = buffer.read(fileSize);
        
        auto hash = AssetUtils::hashData(fileData);
        auto hexHash = hash.toHex();

        if (_senderNode) {
            qDebug() << "Hash for uploaded file from" << uuidStringWithoutCurlyBraces(_senderNode->getUUID()) << "is: (" << hexHash << ")";
        } else {
            qDebug() << "Hash for uploaded file from" << _receivedMessage->getSenderSockAddr() << "is: (" << hexHash << ")";
        }
        
        QFile file { _resourcesDir.filePath(QString(hexHash)) };

        bool existingCorrectFile = false;
        
        if (file.exists()) {
            // check if the local file has the correct contents, otherwise we overwrite
            if (file.open(QIODevice::ReadOnly) && AssetUtils::hashData(file.readAll()) == hash) {
                qDebug() << "Not overwriting existing verified file: " << hexHash;

                existingCorrectFile = true;

                replyPacket->writePrimitive(AssetUtils::AssetServerError::NoError);
                replyPacket->write(hash);
            } else {
                qDebug() << "Overwriting an existing file whose contents did not match the expected hash: " << hexHash;
                file.close();
            }
        }

        if (!existingCorrectFile) {
            if (file.open(QIODevice::WriteOnly) && file.write(fileData) == qint64(fileSize)) {
                qDebug() << "Wrote file" << hexHash << "to disk. Upload complete";
                file.close();

                replyPacket->writePrimitive(AssetUtils::AssetServerError::NoError);
                replyPacket->write(hash);
            } else {
                qWarning() << "Failed to upload or write to file" << hexHash << " - upload failed.";

                // upload has failed - remove the file and return an error
                auto removed = file.remove();

                if (!removed) {
                    qWarning() << "Removal of failed upload file" << hexHash << "failed.";
                }
                
                replyPacket->writePrimitive(AssetUtils::AssetServerError::FileOperationFailed);
            }
        }


    }
    
    auto nodeList = DependencyManager::get<NodeList>();
    if (_senderNode) {
        nodeList->sendPacket(std::move(replyPacket), *_senderNode);
    } else {
        nodeList->sendPacket(std::move(replyPacket), _receivedMessage->getSenderSockAddr());
    }
}
