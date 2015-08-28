//
//  SendAssetTask.h
//  assignment-client/src/assets
//
//  Created by Ryan Huffman on 2015/08/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SendAssetTask_h
#define hifi_SendAssetTask_h

#include <QByteArray>
#include <QString>
#include <QRunnable>

#include "AssetUtils.h"
#include "AssetServer.h"
#include "Node.h"

class SendAssetTask : public QRunnable {
public:
    SendAssetTask(MessageID messageID, const QByteArray& assetHash, QString filePath, DataOffset start, DataOffset end,
                  const SharedNodePointer& sendToNode);

    void run();

private:
    MessageID _messageID;
    QByteArray _assetHash;
    QString _filePath;
    DataOffset _start;
    DataOffset _end;
    SharedNodePointer _sendToNode;
};

#endif
