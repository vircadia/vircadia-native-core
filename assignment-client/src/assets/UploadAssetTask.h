//
//  UploadAssetTask.h
//  assignment-client/src/assets
//
//  Created by Stephen Birarda on 2015-08-28.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_UploadAssetTask_h
#define hifi_UploadAssetTask_h

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QSharedPointer>

#include "ReceivedMessage.h"

class NLPacketList;
class Node;

class UploadAssetTask : public QRunnable {
public:
    UploadAssetTask(QSharedPointer<ReceivedMessage> message, QSharedPointer<Node> senderNode, const QDir& resourcesDir);

    void run() override;

private:
    QSharedPointer<ReceivedMessage> _receivedMessage;
    QSharedPointer<Node> _senderNode;
    QDir _resourcesDir;
};

#endif // hifi_UploadAssetTask_h
